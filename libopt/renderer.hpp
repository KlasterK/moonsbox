#ifndef MOOX_RENDERER_HPP
#define MOOX_RENDERER_HPP

#include "gamemap.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <SDL2pp/SDL2pp.hh>

class Renderer
{
public:
    enum class Mode {Normal, Thermal};

public:
    Renderer(GameMap &map, SDL2pp::Surface &dst_surface, uint32_t bg_color);

    void render(std::array<int, 4> map_visible_area);
    Mode get_mode();
    void set_mode(Mode v);

private:
    GameMap &m_map;
    SDL2pp::Surface &m_dst_surface;
    std::optional<SDL2pp::Surface> m_scale_buffer{}, m_thermal_buffer{};
    uint32_t m_bg_color{};
    std::unique_ptr<uint8_t[]> m_row_buffer{};
    size_t m_row_buffer_size{};
    Mode m_mode{Mode::Normal};
};

#endif // MOOX_RENDERER_HPP
