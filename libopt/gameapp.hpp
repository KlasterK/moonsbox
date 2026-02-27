#ifndef MOOX_GAMEAPP_HPP
#define MOOX_GAMEAPP_HPP

#include "camera.hpp"
#include "gamemap.hpp"
#include "mainwindowui.hpp"
#include "materialpalette.hpp"
#include "renderer.hpp"
#include "simulation.hpp"
#include "materialregistry.hpp"
#include "windowevents.hpp"
#include "materials.hpp"
#include <SDL2pp/SDL2pp.hh>

class GameApp
{
public:
    GameApp();

    void run();
    void stop();
    inline bool is_running() const { return m_is_running; }

private:
    bool m_is_running{true};

    std::tuple<
        Space, Sand, Plus100K, Minus100K, Water, Ice, Steam,
        Tap, UnbreakableWall, BlackHole, Propane, Fire, PureGlass,
        Lava, Absorbent, Aerogel, DryIce
    > m_materials_tuple;

    MaterialRegistry m_registry;
    GameMap m_map;

    SDL2pp::Window m_window;
    SDL2pp::Renderer m_sdl_renderer;
    SDL2pp::Font m_font;

    SimulationManager m_sim;
    Renderer m_renderer;
    Camera m_camera;
    MaterialPalette m_palette;

    std::reference_wrapper<MaterialController> m_drawing_material;
    std::tuple<
        MaterialPaletteEventHandler,
        MainWindowUI,
        GameAppEventHandler, 
        SimulationEventHandler, 
        CameraEventHandler,
        DrawingEventHandler, 
        RendererEventHandler
    > m_event_handlers_tuple; 
};

#endif // MOOX_GAMEAPP_HPP
