#ifndef MOOX_GAMEAPP_HPP
#define MOOX_GAMEAPP_HPP

#include <simulationengine/materials/allmaterials.hpp>
#include <simulationengine/core/materialregistry.hpp>
#include <simulationengine/core/gamemap.hpp>
#include <simulationengine/simulation/simulationmanager.hpp>
#include "renderer.hpp"
#include "camera.hpp"
#include "materialpalette.hpp"
#include "mainwindowui.hpp"
#include <simulationengine/core/materialcontroller.hpp>
#include "windowevents.hpp"
#include <SDL3/SDL_render.h>
#include <functional>
#include <filesystem>
#include <tuple>

class GameApp
{
public:
    GameApp();

    void run();
    void stop();
    inline bool is_running() const { return m_is_running; }

private:
    bool m_is_running{true};

    AllMaterialsT<std::tuple> m_materials_tuple;

    MaterialRegistry m_registry;
    GameMap m_map;

    std::unique_ptr<SDL_Window, void (&)(SDL_Window *)> m_window;
    std::unique_ptr<SDL_Renderer, void (&)(SDL_Renderer *)> m_sdl_renderer;
    std::unique_ptr<TTF_Font, void (&)(TTF_Font *)> m_font;

    SimulationManager m_sim;
    Renderer m_renderer;
    Camera m_camera;
    MaterialPalette m_palette;
    MainWindowUI m_ui;

    std::reference_wrapper<MaterialController> m_drawing_material;
    std::tuple<
        MaterialPaletteEventHandler,
        MainWindowUI &,
        GameAppEventHandler,
        SimulationEventHandler,
        CameraEventHandler,
        DrawingEventHandler,
        RendererEventHandler
    > m_event_handlers_tuple;

    void load_save(std::filesystem::path path);
    void store_save(std::filesystem::path path);
};

#endif // MOOX_GAMEAPP_HPP
