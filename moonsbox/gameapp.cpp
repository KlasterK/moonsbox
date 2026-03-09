#include "gameapp.hpp"
#include "mainwindowui.hpp"
#include "simulationengine/materials/allmaterials.hpp"
#include "soundsystem.hpp"
#include "renderer.hpp"
#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL_messagebox.h>
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
    constexpr uint32_t MapInnerColor{0x000000FF};
    constexpr uint32_t MapOuterColor{0x113311FF};
    constexpr std::string_view WindowTitle{"moonsbox"};
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
    , m_window(
        WindowTitle.data(), 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,
        ScreenSize[0],
        ScreenSize[1],
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    )
    , m_sdl_renderer(
        m_window, 
        -1, 
        SDL_RENDERER_ACCELERATED | (VSync ? SDL_RENDERER_PRESENTVSYNC : 0)
    )
    , m_font("assets/NotoSans-Regular.ttf", 16)
    , m_sim(m_map, m_registry)
    , m_renderer(m_map, m_sdl_renderer, MapInnerColor)
    , m_camera(VisibleArea, ScreenSize)
    , m_palette(m_sdl_renderer, m_registry, m_font)
    , m_ui{
        m_sdl_renderer, 
        m_font, 
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
    m_window.SetIcon(SDL2pp::Surface{"assets/window_icon"});
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

        m_sdl_renderer.SetDrawColor(
            MapOuterColor >> 24 & 0xFF,
            MapOuterColor >> 16 & 0xFF,
            MapOuterColor >> 8 & 0xFF
        );
        m_sdl_renderer.Clear();
        m_renderer.render(
            m_camera.get_map_area(
                {m_window.GetWidth(), m_window.GetHeight()}
            )
        );

        m_ui.render();
        m_palette.render();
        m_sdl_renderer.Present();
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
            m_window.Get()
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
            m_window.Get()
        );
    }

    if(!container.close())
    {
        throw std::runtime_error(
            "GameApp::store_save: unable to close MinizipWriteSaveContainer"
        );
    }
}
