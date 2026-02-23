#include "renderer.hpp"
#include "SDL_blendmode.h"
#include "SDL_render.h"
#include <SDL2/SDL.h>
#include <SDL2pp/Exception.hh>
#include <SDL2pp/Optional.hh>
#include <SDL2pp/Texture.hh>
#include <algorithm>
#include <cstring>
#include <utility>
#include <variant>

Renderer::Renderer(GameMap &map, SDL2pp::Renderer &sdl_renderer, uint32_t bg_color)
    : m_map(map)
    , m_sdl_renderer(sdl_renderer)
    , m_bg_color(bg_color)
    , m_map_tex(
        sdl_renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING,
        int(map.width()),
        int(map.height())
    )
{
    m_map_tex.SetBlendMode(SDL_BLENDMODE_BLEND);
}

void Renderer::render(std::array<int, 4> visible_area)
{
    // Boundaries of visible area (clipped visible area and map rects)
    int x_begin = std::max(visible_area[0], 0);
    int x_end = std::min(visible_area[0] + visible_area[2], (int)m_map.width());
    int y_map_begin = std::max(visible_area[1], 0);
    int y_map_end = std::min(visible_area[1] + visible_area[3], (int)m_map.height());

    int inner_w = x_end - x_begin;
    int inner_h = y_map_end - y_map_begin;

    if (inner_w <= 0 || inner_h <= 0) 
        return;

    // Screen size
    auto [screen_w, screen_h] = m_sdl_renderer.GetOutputSize();

    // Scale
    double scale_x = double(screen_w) / double(visible_area[2]);
    double scale_y = double(screen_h) / double(visible_area[3]);

    // The position of the inner area in the visible area (in pixels)
    double inner_left   = (x_begin - visible_area[0]) * scale_x;
    double inner_top    = (y_map_begin - visible_area[1]) * scale_y;
    double inner_right  = (x_end - visible_area[0]) * scale_x;
    double inner_bottom = (y_map_end - visible_area[1]) * scale_y;

    int blit_w = int(inner_right - inner_left);
    int blit_h = int(inner_bottom - inner_top);

    SDL2pp::Rect src_rect{
        x_begin, int(m_map.height()) - y_map_end, inner_w, inner_h
    };

    SDL2pp::Rect dst_rect{
        int(inner_left), int(inner_top), blit_w, blit_h
    };

    if(m_mode == Mode::Normal)
    {
        m_sdl_renderer.SetDrawColor(
            m_bg_color >> 24 & 0xFF,
            m_bg_color >> 16 & 0xFF,
            m_bg_color >> 8 & 0xFF
        );
        m_sdl_renderer.FillRect(dst_rect);
        m_map_tex.Update(SDL2pp::NullOpt, m_map.colors.surface());
    }
    else 
    {
        auto pixels = (uint32_t *)m_thermal_buffer->Get()->pixels;
        auto pitch = m_thermal_buffer->Get()->pitch;

        for(int y = src_rect.y; y < src_rect.y + src_rect.h; ++y)
        {
            for(int x = src_rect.x; x < src_rect.x + src_rect.w; ++x)
            {
                uint32_t rgba = m_map.colors(size_t(x), size_t(y));
                // 50 % of grayscale
                int32_t darkscale = (
                    ((rgba >> 24) & 0xFF)
                    + ((rgba >> 16) & 0xFF)
                    + ((rgba >> 8) & 0xFF)
                ) / 6;
                float temp_factor = m_map.temps(size_t(x), size_t(y)) / 500.f;
                auto result_color = uint32_t(
                    (std::min(0xFF, darkscale + int32_t(temp_factor * 0xBF)) << 24)
                    | (std::clamp(darkscale + int32_t((temp_factor - 1) * 0x3F),
                                  0x00, 0xFF) << 16)
                    | (darkscale << 8)
                    | 0xFF
                );
                pixels[y * pitch / 4 + x] = result_color;
            }
        }

        m_map_tex.Update(SDL2pp::NullOpt, *m_thermal_buffer);
    }

    m_sdl_renderer.Copy(
        m_map_tex, 
        src_rect, 
        dst_rect, 
        0.0, 
        SDL2pp::NullOpt, 
        SDL_FLIP_VERTICAL
    );
}

Renderer::Mode Renderer::get_mode()
{
    return m_mode;
}

void Renderer::set_mode(Mode v)
{
    if(v == m_mode)
        return;

    if(v == Mode::Normal)
    {
        m_sdl_renderer.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
        m_thermal_buffer.reset();
    }
    else if(v == Mode::Thermal)
    {
        m_sdl_renderer.SetDrawBlendMode(SDL_BLENDMODE_NONE);
        auto *surf = SDL_CreateRGBSurfaceWithFormat(
            0,
            int(m_map.width()),
            int(m_map.height()),
            32,
            SDL_PIXELFORMAT_RGBX8888
        );
        if(surf == nullptr)
            throw SDL2pp::Exception("SDL_CreateRGBSurfaceWithFormat");
        m_thermal_buffer.emplace(surf);
    }
    else throw std::logic_error("Renderer::set_mode: invalid Mode value");
    m_mode = v;
}
