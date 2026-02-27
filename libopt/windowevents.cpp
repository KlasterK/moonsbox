#include "windowevents.hpp"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "drawing.hpp"
#include "gameapp.hpp"
#include "gamemap.hpp"
#include "soundsystem.hpp"
#include <stdexcept>

namespace
{
    constexpr float ZoomFactor = 0.1f;
    constexpr bool DrawingIsCircular = true;
    constexpr bool DrawingIsDestructive = false;
    constexpr int DeltaDrawingWidth = 1;
}

GameAppEventHandler::GameAppEventHandler(GameApp &app)
    : m_app(app)
{}

bool GameAppEventHandler::process_event(const SDL_Event &e)
{
    if (e.type == SDL_QUIT)
    {
        m_app.stop();
        return true;
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
    {
        m_app.stop();
        return true;
    }
    return false;
}

SimulationEventHandler::SimulationEventHandler(GameMap &map, SimulationManager &sim, MaterialRegistry &registry)
    : m_map(map), m_sim(sim), m_registry(registry)
{}

bool SimulationEventHandler::process_event(const SDL_Event &e)
{
    if (e.type != SDL_KEYDOWN)
        return false;
    
    if(e.key.keysym.sym == SDLK_F5)
    {
        bool was_paused = m_sim.is_paused();
        m_sim.set_paused(false);
        m_sim.tick();
        m_sim.set_paused(was_paused);
    }
    else if(e.key.keysym.sym == SDLK_F6)
    {
        MaterialController *space = m_registry.find_controller_by_name("Space");
        assert(space);
        drawing::fill(m_map, drawing::make_controller_init_point_factory(m_map, *space));
    }
    else if(e.key.keysym.sym == SDLK_SPACE)
    {
        m_sim.set_paused(!m_sim.is_paused());
    }
    return false;
}

CameraEventHandler::CameraEventHandler(Camera &camera) 
    : m_camera(camera) 
{}

bool CameraEventHandler::process_event(const SDL_Event &e)
{
    if (e.type == SDL_MOUSEMOTION)
    {
        if (SDL_GetModState() & KMOD_SHIFT || (e.motion.state & SDL_BUTTON_RMASK))
        
            m_camera.move({-e.motion.xrel, -e.motion.yrel});
        return false;
    }
    if (e.type == SDL_MOUSEWHEEL)
    {
        if (SDL_GetModState() & KMOD_ALT)
            return false;

        int mx{}, my{};
        SDL_GetMouseState(&mx, &my);
        m_camera.zoom(-e.wheel.preciseY * ZoomFactor, {mx, my});
        return false;
    }
    return false;
}

DrawingEventHandler::DrawingEventHandler(
    Camera &camera, 
    GameMap &map, 
    const std::reference_wrapper<MaterialController> &drawing_material
)
    : m_camera(camera)
    , m_map(map)
    , m_drawing_material(drawing_material)
{}

bool DrawingEventHandler::process_event(const SDL_Event &e)
{
    if (
        (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_LCTRL 
                                   || e.key.keysym.sym == SDLK_RCTRL))
        || (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    )
    {
        int mx{}, my{};
        SDL_GetMouseState(&mx, &my);
        auto mouse_map_pos = m_camera.to_map_pos({mx, my});
        mouse_map_pos[1] = int(m_map.height()) - 1 - mouse_map_pos[1];

        drawing::Rect area{
            mouse_map_pos[0] - m_brush_width / 2,
            mouse_map_pos[1] - m_brush_width / 2,
            m_brush_width,
            m_brush_width
        };

        if(DrawingIsCircular)
            drawing::ellipse(m_map, area, material_factory());
        else
            drawing::rect(m_map, area, material_factory());

        m_drawing_material.get().play_place_sound(m_map, mouse_map_pos[0], mouse_map_pos[1]);
        
        m_previous_pos = mouse_map_pos;
        m_is_holded = true;
        return false;
    }

    if (e.type == SDL_MOUSEMOTION && m_is_holded)
    {
        auto mouse_map_pos = m_camera.to_map_pos({e.motion.x, e.motion.y});
        mouse_map_pos[1] = int(m_map.height()) - 1 - mouse_map_pos[1];
        
        drawing::line(
            m_map, 
            m_previous_pos, 
            mouse_map_pos, 
            m_brush_width, 
            material_factory(), 
            DrawingIsCircular ? drawing::LineEnds::Round : drawing::LineEnds::Square
        );

        m_drawing_material.get().play_place_sound(m_map, mouse_map_pos[0], mouse_map_pos[1]);

        m_previous_pos = mouse_map_pos;
        return false;
    }

    if (
        (e.type == SDL_KEYUP && (e.key.keysym.sym == SDLK_LCTRL 
                                 || e.key.keysym.sym == SDLK_RCTRL))
        || (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    )
    {
        m_previous_pos = {-1, -1};
        m_is_holded = false;
        return false;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_EQUALS)
    {
        m_brush_width += DeltaDrawingWidth;
        return false;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_MINUS)
    {
        m_brush_width = std::max(1, m_brush_width - DeltaDrawingWidth);
        return false;
    }

    if (e.type == SDL_MOUSEWHEEL && SDL_GetModState() & KMOD_ALT)
    {
        m_brush_width = std::max(1, m_brush_width + e.wheel.y);
        return false;
    }

    // if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_k)
    // {
    //     int mx{}, my{};
    //     SDL_GetMouseState(&mx, &my);
    //     auto mouse_map_pos = m_camera.to_map_pos({mx, my});
    //     mouse_map_pos[1] = int(m_map.height()) - 1 - mouse_map_pos[1];

    //     if(mouse_map_pos[0] < 0 || mouse_map_pos[1] < 0)
    //         return false;
    //     std::array<size_t, 2> mouse_map_pos_uz{size_t(mouse_map_pos[0]), size_t(mouse_map_pos[1])};

    //     if (!m_map.in_bounds(mouse_map_pos_uz[0], mouse_map_pos_uz[1]))
    //         return false;

    //     MaterialController *ctl = m_map.material_ctls(mouse_map_pos_uz[0], mouse_map_pos_uz[1]);
    //     assert(ctl);

        
    //         m_palette->set_selected_material(*ctl);
    //     return false;
    // }

    return false;
}

void DrawingEventHandler::update()
{
    if(!m_is_holded)
        return;

    drawing::Rect area{
        m_previous_pos[0] - m_brush_width / 2,
        m_previous_pos[1] - m_brush_width / 2,
        m_brush_width,
        m_brush_width
    };

    if(DrawingIsCircular)
        drawing::ellipse(m_map, area, material_factory());
    else
        drawing::rect(m_map, area, material_factory());

    m_drawing_material.get().play_place_sound(m_map, m_previous_pos[0], m_previous_pos[1]);
}

drawing::MaterialFactory DrawingEventHandler::material_factory()
{
    return [this](size_t x, size_t y)
    {
        if (!DrawingIsDestructive && !m_drawing_material.get().is_placeable_on(m_map, x, y))
            return;

        if (!m_map.in_bounds(x, y))
            return;

        m_drawing_material.get().init_point(m_map, x, y);
    };
}

MaterialPaletteEventHandler::MaterialPaletteEventHandler(
    MaterialPalette &palette, 
    MaterialRegistry &registry,
    std::reference_wrapper<MaterialController> &drawing_material
)
    : m_palette(palette)
    , m_registry(registry)
    , m_drawing_material(drawing_material)
    , m_previous_idx(m_palette.get_current_selection_idx())
{}

bool MaterialPaletteEventHandler::process_event(const SDL_Event &e)
{
    if (m_palette.is_visible())
    {
        if (e.type == SDL_KEYDOWN)
        {
            switch (e.key.keysym.sym)
            {
            case SDLK_LEFT:
            case SDLK_a:
                m_palette.move_selection(-1, 0);
                sfx::play_sound("palette.move_selection", "ui", true);
                return true;
            case SDLK_RIGHT:
            case SDLK_d:
                m_palette.move_selection(1, 0);
                sfx::play_sound("palette.move_selection", "ui", true);
                return true;
            case SDLK_UP:
            case SDLK_w:
                m_palette.move_selection(0, -1);
                sfx::play_sound("palette.move_selection", "ui", true);
                return true;
            case SDLK_DOWN:
            case SDLK_s:
                m_palette.move_selection(0, 1);
                sfx::play_sound("palette.move_selection", "ui", true);
                return true;
            case SDLK_RETURN:
            case SDLK_SPACE:
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                m_previous_idx = m_palette.get_current_selection_idx();
                m_drawing_material = m_palette.get_selected_material();
                m_palette.hide();
                sfx::play_sound("palette.hide_confirmation", "ui", true);
                return true;
            case SDLK_ESCAPE:
                m_palette.set_current_selection_idx(m_previous_idx);
                m_palette.hide();
                sfx::play_sound("palette.hide_no_confirmation", "ui", true);
                return true;
            default:
                return true; // consume all keys while palette visible
            }
        }

        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            if(e.button.button == SDL_BUTTON_LEFT)
            {
                auto idx_opt = m_palette.match_click_to_idx(e.button.x, e.button.y);
                if(idx_opt)
                {
                    m_palette.set_current_selection_idx(*idx_opt);
                    m_previous_idx = *idx_opt;
                    m_drawing_material = m_palette.get_selected_material();
                    m_palette.hide();
                    sfx::play_sound("palette.hide_confirmation", "ui", true);
                }
                else
                {
                    m_palette.set_current_selection_idx(m_previous_idx);
                    m_palette.hide();
                    sfx::play_sound("palette.hide_no_confirmation", "ui", true);
                }
            }
            else if(e.button.button == SDL_BUTTON_RIGHT)
            {
                m_palette.set_current_selection_idx(m_previous_idx);
                m_palette.hide();
                sfx::play_sound("palette.hide_no_confirmation", "ui", true);
            }
            return true;
        }
        if (e.type == SDL_MOUSEWHEEL)
        {
            m_palette.scroll(-e.wheel.y);
            return true;
        }
    }
    else
    {
        if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_TAB)
            || (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_MIDDLE))
        {
            m_palette.show();
            sfx::play_sound("palette.show", "ui", true);
            return false;
        }

        if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_u)
            || (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_X2))
        {
            m_material_stack.push_back(m_drawing_material);
            return false;
        }
        
        if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_i)
            || (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_X1))
        {
            m_drawing_material = [this] -> auto &
            {
                if (m_material_stack.empty())
                {
                    auto *p_ctl = m_registry.find_controller_by_name("Space");
                    assert(p_ctl);
                    return *p_ctl;
                }
                auto ctl_refwr = m_material_stack.back();
                m_material_stack.pop_back();
                return ctl_refwr.get();
            }();

            auto idx_opt = m_palette.find_material_idx(m_drawing_material.get());
            if(idx_opt)
                m_palette.set_current_selection_idx(*idx_opt);
            return false;
        }
    }
    return false;
}

RendererEventHandler::RendererEventHandler(Renderer &renderer) 
    : m_renderer(renderer) 
{}

bool RendererEventHandler::process_event(const SDL_Event &e)
{
    if (e.type != SDL_KEYDOWN)
        return false;

    if (e.key.keysym.sym == SDLK_v)
    {
        switch(m_renderer.get_mode())
        {
        case Renderer::Mode::Normal:
            m_renderer.set_mode(Renderer::Mode::Thermal);
            return false;
        case Renderer::Mode::Thermal:
            m_renderer.set_mode(Renderer::Mode::Normal);
            return false;
        default:
            return false;
        }
    }

    return false;
}
