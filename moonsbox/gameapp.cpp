#include "gameapp.hpp"
#include "mainwindowui.hpp"
#include "simulationengine/materials/allmaterials.hpp"
#include "soundsystem.hpp"
#include "renderer.hpp"
#include <stdexcept>
#include <array>
#include <cstddef>
#include <cstdint>
#include <simulationengine/algorithms/drawing.hpp>
#include <simulationengine/core/materialregistry.hpp>
#include <simulationengine/serialization/minizipsavecontainer.hpp>
#include <simulationengine/serialization/saving.hpp>
#include <string_view>

namespace
{
    constexpr std::array<int, 2> ScreenSize{1520, 890};
    constexpr std::array<size_t, 2> MapSize{720, 405};
    constexpr std::array<int, 4> VisibleArea{-20, -20, 760, 445}; // x,y,w,h
    constexpr SDL_Color MapInnerColor{0x00, 0x00, 0x00, 0xFF};
    constexpr SDL_Color MapOuterColor{0x11, 0x33, 0x11, 0xFF};
    constexpr std::string_view WindowTitleCStr{"moonsbox\0"};
    constexpr bool VSync{true};
}

GameApp::GameApp()
    : m_materials_tuple{}
    , m_registry([this]
    {
        MaterialRegistry registry;
        register_all_materials(m_materials_tuple, registry);
        for(auto &pair : registry)
            pair.second->set_play_sound_callback(
                [](std::string_view name, std::optional<std::string_view> category, 
                   bool do_override)
                {
                    sfx::play_sound(name, category, do_override);
                }
            );
        return registry;
    }())
    , m_map([this]
    {
        GameMap map(MapSize[0], MapSize[1]);
        auto factory = drawing::make_controller_init_point_factory(
            map, std::get<Space>(m_materials_tuple)
        );
        drawing::fill(map, std::move(factory));
        return map;
    }())
    , m_window(nullptr, SDL_DestroyWindow)
    , m_sdl_renderer([this] -> decltype(m_sdl_renderer)
    {
        SDL_Window *w{};
        SDL_Renderer *r{};
        if(!SDL_CreateWindowAndRenderer(WindowTitleCStr.data(),
                                        ScreenSize[0],
                                        ScreenSize[1],
                                        SDL_WINDOW_RESIZABLE,
                                        &w,
                                        &r))
            throw std::runtime_error("GameApp::GameApp: SDL_CreateWindowAndRenderer failed");

        m_window.reset(w);
        return {r, SDL_DestroyRenderer};
    }())
    , m_font([] -> decltype(m_font)
    {
        auto *f = TTF_OpenFont("assets/NotoSans-Regular.ttf", 16);
        if(!f)
            throw std::runtime_error("GameApp::GameApp: TTF_OpenFont returned nullptr");
        return {f, TTF_CloseFont};
    }())
    , m_sim(m_map, m_registry)
    , m_renderer(m_map, m_sdl_renderer.get(), MapInnerColor)
    , m_camera(VisibleArea, ScreenSize)
    , m_palette(m_sdl_renderer.get(), m_registry, m_font.get())
    , m_ui{
        m_sdl_renderer.get(),
        m_font.get(),
        m_palette,
        [this](std::filesystem::path path) { load_save(path); },
        [this](std::filesystem::path path) { store_save(path); },
    }
    , m_drawing_material(std::get<Sand>(m_materials_tuple))
    , m_event_handlers_tuple{
        MaterialPaletteEventHandler{m_palette, m_registry, m_drawing_material},
        m_ui,
        GameAppEventHandler{*this},
        SimulationEventHandler{m_map, m_sim, m_registry},
        CameraEventHandler{m_camera},
        DrawingEventHandler{m_camera, m_map, m_drawing_material},
        RendererEventHandler{m_renderer},
    }
{
    if(VSync)
        SDL_SetRenderVSync(m_sdl_renderer.get(), 1);

    auto *s = SDL_LoadSurface("assets/window_icon");
    if(!s)
        throw std::runtime_error("GameApp::GameApp: failed to load window icon");
    SDL_SetWindowIcon(m_window.get(), s);
    SDL_DestroySurface(s);
}

void GameApp::run()
{
    while (m_is_running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            std::apply([&e](auto &...handlers)
            {
                bool consumed{};
                auto process = [&](auto& handler) 
                {
                    if (!consumed)
                        consumed = handler.process_event(e);
                };
                (process(handlers), ...);
            }, m_event_handlers_tuple);
        }

        std::get<DrawingEventHandler>(m_event_handlers_tuple).update_drawing();
        m_sim.tick();

        SDL_SetRenderDrawColor(
            m_sdl_renderer.get(),
            MapOuterColor.r,
            MapOuterColor.g,
            MapOuterColor.b,
            MapOuterColor.a
        );
        SDL_RenderClear(m_sdl_renderer.get());
        int screen_w{}, screen_h{};
        SDL_GetRenderOutputSize(m_sdl_renderer.get(), &screen_w, &screen_h);
        m_renderer.render(m_camera.get_map_area({screen_w, screen_h}));

        m_ui.render();
        m_palette.render();
        SDL_RenderPresent(m_sdl_renderer.get());
    }
}

void GameApp::stop()
{
    m_is_running = false;
}

void GameApp::load_save(std::filesystem::path path)
{
    MinizipReadSaveContainer container(path);
    auto map_result = saving::deserialize(container, m_registry);
    if(!map_result.has_value())
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_WARNING,
            "Save Loading Error",
            map_result.error().c_str(),
            m_window.get()
        );
    }
    else
    {
        m_map = std::move(*map_result);
    }
}

void GameApp::store_save(std::filesystem::path path)
{
    MinizipWriteSaveContainer container(path);
    auto error_result = saving::serialize(container, m_map, m_registry);
    if(!error_result.has_value())
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_WARNING,
            "Save Storing Error",
            error_result.error().c_str(),
            m_window.get()
        );
    }

    if(!container.close())
    {
        throw std::runtime_error(
            "GameApp::store_save: unable to close MinizipWriteSaveContainer"
        );
    }
}
