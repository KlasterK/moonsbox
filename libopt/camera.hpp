#ifndef MOOX_CAMERA_HPP
#define MOOX_CAMERA_HPP

#include <array>

class Camera
{
public:
    Camera(std::array<int, 4> initial_map_visible_area, std::array<int, 2> screen_size);

    void zoom(float factor, std::array<int, 2> mouse_onscreen_pos);
    void move(std::array<int, 2> relative_onscreen_pos);

    std::array<int, 4> get_map_area(std::array<int, 2> screen_size) const;
    std::array<int, 2> to_map_pos(std::array<int, 2> onscreen_pos) const;

private:
    std::array<float, 2> m_map_topleft{}, m_scale_map_per_onscreen{};
};

#endif // MOOX_CAMERA_HPP
