#include "gameapp.hpp"
#include "drawing.hpp"
#include "fpscounter.hpp"
#include "materialregistry.hpp"
#include "materials.hpp"
#include "renderer.hpp"
#include <SDL2pp/SDL2pp.hh>

namespace
{
    std::array<int, 2> ScreenSize{1520, 890};
    std::array<size_t, 2> MapSize{720, 405};
    std::array<int, 4> VisibleArea{-20, -20, 760, 445}; // x,y,w,h
    uint32_t MapInnerColor{0x000000FF};
    uint32_t MapOuterColor{0x113311FF};
    const char *WindowTitle = "moonsbox";
    bool VSync = true;
    bool DoEnableFPSCounter = true;
}

template<typename T>
static void _register_materials_from_tuple(T &tuple, MaterialRegistry &registry)
{
    registry.register_controller(std::get<Space>(tuple),            "Space");
    registry.register_controller(std::get<Sand>(tuple),             "Sand");
    registry.register_controller(std::get<Plus100K>(tuple),         "+100 K");
    registry.register_controller(std::get<Minus100K>(tuple),        "-100 K");
    registry.register_controller(std::get<Water>(tuple),            "Water");
    registry.register_controller(std::get<Ice>(tuple),              "Ice");
    registry.register_controller(std::get<Steam>(tuple),            "Steam");
    registry.register_controller(std::get<Tap>(tuple),              "Tap");
    registry.register_controller(std::get<UnbreakableWall>(tuple),  "Unbreakable Wall");
    registry.register_controller(std::get<BlackHole>(tuple),        "Black Hole");
    registry.register_controller(std::get<Propane>(tuple),          "Propane");
    registry.register_controller(std::get<Fire>(tuple),             "Fire");
    registry.register_controller(std::get<PureGlass>(tuple),        "Glass");
    registry.register_controller(std::get<Lava>(tuple),             "Lava");
    registry.register_controller(std::get<Absorbent>(tuple),        "Absorbent");
    registry.register_controller(std::get<Aerogel>(tuple),          "Aerogel");
    registry.register_controller(std::get<DryIce>(tuple),           "Dry Ice");
}

GameApp::GameApp()
    : m_materials_tuple{}
    , m_registry([this]
    {
        MaterialRegistry registry;
        _register_materials_from_tuple(m_materials_tuple, registry);
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
        WindowTitle, 
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
    , m_drawing_material(std::get<Sand>(m_materials_tuple))
    , m_event_handlers_tuple{
        MaterialPaletteEventHandler{m_palette, m_registry, m_drawing_material},
        GameAppEventHandler{*this},
        SimulationEventHandler{m_map, m_sim, m_registry},
        CameraEventHandler{m_camera},
        DrawingEventHandler{m_camera, m_map, m_drawing_material},
        RendererEventHandler{m_renderer},
    }
{}

void GameApp::run()
{
    FPSCounter fps_counter;

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

        std::apply([](auto &...handlers)
        {
            (handlers.update(), ...);
        }, m_event_handlers_tuple);

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

        if(DoEnableFPSCounter)
        {
            fps_counter.tick();
            auto surf = m_font.RenderText_Blended(
                std::format(
                    "FPS: Avg {:.2f} Min {:.2f} Max {:.2f}", 
                    fps_counter.avg_fps(), fps_counter.min_fps(), fps_counter.max_fps()
                ),
                {0, 255, 0, 255}
            );
            SDL2pp::Texture tex(m_sdl_renderer, surf);
            SDL2pp::Rect dst_rect{10, 10, surf.GetWidth(), surf.GetHeight()};
            m_sdl_renderer.Copy(tex, SDL2pp::NullOpt, dst_rect);
        }

        m_palette.render();
        m_sdl_renderer.Present();
    }
}

void GameApp::stop()
{
    m_is_running = false;
}
