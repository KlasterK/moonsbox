#ifndef MOOX_RENDERER_HPP
#define MOOX_RENDERER_HPP

#include <SDL3/SDL_render.h>
#include <cstdint>
#include <memory>
#include <array>

class GameMap;

class Renderer
{
public:
    enum class Mode {Normal, Thermal};

public:
    Renderer(GameMap &map, SDL_Renderer *nonown_sdl_renderer, const SDL_Color &bg_color);

    void render(std::array<int, 4> map_visible_area);
    Mode get_mode();
    void set_mode(Mode v);

private:
    GameMap &m_map;
    SDL_Renderer *m_sdl_renderer;
    SDL_Color m_bg_color{};
    Mode m_mode{Mode::Normal};

    std::unique_ptr<SDL_Texture, void (&)(SDL_Texture *)>
        m_map_tex_opt{nullptr, SDL_DestroyTexture};

    std::unique_ptr<SDL_Surface, void (&)(SDL_Surface *)>
        m_buffer_surf_opt{nullptr, SDL_DestroySurface};
};

#endif // MOOX_RENDERER_HPP
