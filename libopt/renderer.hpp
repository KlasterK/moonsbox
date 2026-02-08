#ifndef MOOX_RENDERER_HPP
#define MOOX_RENDERER_HPP

#include "gamemap.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

class Renderer
{
public:
    enum class Mode {Normal, Thermal};

public:
    Renderer(GameMap &map, py::object &dst_surface, uint32_t bg_color);
    Renderer(Renderer &&) = default;
    Renderer(const Renderer &) = delete;
    Renderer &operator=(Renderer &&) = delete;
    Renderer &operator=(const Renderer &) = delete;
    ~Renderer();

    void render(std::array<int, 4> visible_area);
    Mode get_mode();
    void set_mode(Mode v);

private:
    GameMap &m_map;
    py::object m_dst_pgsurf;
    struct SDL_Surface *m_scale_buffer{}, *m_thermal_buffer{};
    uint32_t m_bg_color{};
    std::unique_ptr<uint8_t[]> m_row_buffer{};
    size_t m_row_buffer_size{};
    Mode m_mode = Mode::Normal;
};

#endif // MOOX_RENDERER_HPP
