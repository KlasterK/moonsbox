#ifndef MOOX_RENDERER_HPP
#define MOOX_RENDERER_HPP

#include <SDL2pp/Texture.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Surface.hh>
#include <cstdint>
#include <optional>
#include <array>
#include <variant>

class GameMap;

class Renderer
{
public:
    enum class Mode {Normal, Thermal};

public:
    Renderer(GameMap &map, SDL2pp::Renderer &sdl_renderer, uint32_t bg_color);

    void render(std::array<int, 4> map_visible_area);
    Mode get_mode();
    void set_mode(Mode v);

private:
    GameMap &m_map;
    SDL2pp::Renderer &m_sdl_renderer;
    uint32_t m_bg_color{};
    Mode m_mode{Mode::Normal};

    std::optional<SDL2pp::Texture> m_map_tex_opt;
    std::optional<SDL2pp::Surface> m_buffer_surf_opt;
};

#endif // MOOX_RENDERER_HPP
