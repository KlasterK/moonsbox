#include "camera.hpp"
#include <SDL2/SDL.h>
#include <array>

Camera::Camera(std::array<int, 4> initial_map_visible_area, std::array<int, 2> screen_size)
    : m_map_topleft{
        float(initial_map_visible_area[0]), 
        float(initial_map_visible_area[1]),
    }
    , m_scale_map_per_onscreen{
        float(initial_map_visible_area[2]) / screen_size[0],
        float(initial_map_visible_area[3]) / screen_size[1],
    }
{}

void Camera::zoom(float factor, std::array<int, 2> mouse_onscreen_pos)
{
    float multiplier = factor > 0
                     ? factor + 1.f
                     : 1.f / (-factor + 1.f);

    std::array<float, 2> new_scale{
        m_scale_map_per_onscreen[0] * multiplier,
        m_scale_map_per_onscreen[1] * multiplier
    };
    std::array<float, 2> map_pos{
        mouse_onscreen_pos[0] * m_scale_map_per_onscreen[0] + m_map_topleft[0],
        mouse_onscreen_pos[1] * m_scale_map_per_onscreen[1] + m_map_topleft[1],
    };
    m_map_topleft[0] = map_pos[0] - float(mouse_onscreen_pos[0]) * new_scale[0];
    m_map_topleft[1] = map_pos[1] - float(mouse_onscreen_pos[1]) * new_scale[1];
    m_scale_map_per_onscreen = new_scale;
}

void Camera::move(std::array<int, 2> relative_onscreen_pos)
{
    m_map_topleft[0] += float(relative_onscreen_pos[0]) * m_scale_map_per_onscreen[0];
    m_map_topleft[1] += float(relative_onscreen_pos[1]) * m_scale_map_per_onscreen[1];
}

std::array<int, 4> Camera::get_map_area(std::array<int, 2> screen_size) const
{
    return {
        int(m_map_topleft[0]),
        int(m_map_topleft[1]),
        int(screen_size[0] * m_scale_map_per_onscreen[0]),
        int(screen_size[1] * m_scale_map_per_onscreen[1]),
    };
}

std::array<int, 2> Camera::to_map_pos(std::array<int, 2> onscreen_pos) const
{
    return {
        int(onscreen_pos[0] * m_scale_map_per_onscreen[0] + m_map_topleft[0]),
        int(onscreen_pos[1] * m_scale_map_per_onscreen[1] + m_map_topleft[1]),
    };
}
