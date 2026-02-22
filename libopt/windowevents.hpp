#ifndef MOOX_WINDOWEVENTS_HPP
#define MOOX_WINDOWEVENTS_HPP

#include "camera.hpp"
#include "drawing.hpp"
#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include "materialpalette.hpp"
#include "renderer.hpp"
#include "simulation.hpp"
#include <SDL2pp/SDL2pp.hh>

/// Event handler concept
///
/// Types satisfying this concept must provide:
/// - `bool process_event(const SDL_Event &)` -- processes event,
///   returns false if event should be propagated to next handler, true otherwise
/// - `void update()` -- called every frame
template<typename T>
concept EventHandler = requires(T h, const SDL_Event &e)
{
    { h.process_event(e) } -> std::same_as<bool>;
    { h.update() } -> std::same_as<void>;
};

class GameAppEventHandler
{
public:
    explicit GameAppEventHandler(class GameApp &app);
    bool process_event(const SDL_Event &e);
    inline void update() {}

private:
    GameApp &m_app;
};
static_assert(EventHandler<GameAppEventHandler>);

class SimulationEventHandler
{
public:
    SimulationEventHandler(GameMap &map, SimulationManager &sim, MaterialRegistry &registry);
    bool process_event(const SDL_Event &e);
    inline void update() {}

private:
    GameMap &m_map;
    SimulationManager &m_sim;
    MaterialRegistry &m_registry;
};
static_assert(EventHandler<SimulationEventHandler>);

class CameraEventHandler
{
public:
    explicit CameraEventHandler(Camera &camera);
    bool process_event(const SDL_Event &e);
    inline void update() {}

private:
    Camera &m_camera;
};
static_assert(EventHandler<CameraEventHandler>);

class DrawingEventHandler
{
public:
    DrawingEventHandler(Camera &camera, GameMap &map, 
                        const std::reference_wrapper<MaterialController> &drawing_material);
    bool process_event(const SDL_Event &e);
    void update();

private:
    Camera &m_camera;
    GameMap &m_map;

    std::array<int, 2> m_previous_pos{0, 0};
    bool m_is_holded = false;
    int m_brush_width = 1;
    const std::reference_wrapper<MaterialController> &m_drawing_material;

    drawing::MaterialFactory material_factory();
};
static_assert(EventHandler<DrawingEventHandler>);

class MaterialPaletteEventHandler
{
public:
    MaterialPaletteEventHandler(MaterialPalette &palette, MaterialRegistry &registry,
                                std::reference_wrapper<MaterialController> &drawing_material);
    bool process_event(const SDL_Event &e);
    inline void update() {}

private:
    MaterialPalette &m_palette;
    MaterialRegistry &m_registry;
    std::reference_wrapper<MaterialController> &m_drawing_material;

    std::array<size_t, 2> m_previous_slot{0, 0};
    std::vector<std::reference_wrapper<MaterialController>> m_material_stack;
};
static_assert(EventHandler<MaterialPaletteEventHandler>);

class RendererEventHandler
{
public:
    explicit RendererEventHandler(Renderer &renderer);
    bool process_event(const SDL_Event &e);
    inline void update() {}

private:
    Renderer &m_renderer;
};
static_assert(EventHandler<RendererEventHandler>);

#endif // MOOX_WINDOWEVENTS_HPP
