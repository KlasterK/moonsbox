#include "renderer.hpp"
#include <simulationengine/core/gamemap.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <array>

Renderer::Renderer(GameMap &map, SDL_Renderer *nonown_sdl_renderer, const SDL_Color &bg_color)
    : m_map(map)
    , m_sdl_renderer(nonown_sdl_renderer)
    , m_bg_color(bg_color)
{}

void Renderer::render(std::array<int, 4> visible_area)
{
    if (!m_map_tex_opt
        || m_map_tex_opt->w != int(m_map.width())
        || m_map_tex_opt->h != int(m_map.height()))
    {
        m_map_tex_opt.reset(SDL_CreateTexture(
            m_sdl_renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            int(m_map.width()),
            int(m_map.height())
        ));
        if(!m_map_tex_opt)
            throw std::runtime_error("Renderer::render: SDL_CreateTexture returned nullptr");
        SDL_SetTextureBlendMode(m_map_tex_opt.get(), SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(m_map_tex_opt.get(), SDL_SCALEMODE_PIXELART);
    }

    // Boundaries of visible area (clipped visible area and map rects)
    int x_begin = std::max(visible_area[0], 0);
    int x_end = std::min(visible_area[0] + visible_area[2], int(m_map.width()));
    int y_map_begin = std::max(visible_area[1], 0);
    int y_map_end = std::min(visible_area[1] + visible_area[3], int(m_map.height()));

    int inner_w = x_end - x_begin;
    int inner_h = y_map_end - y_map_begin;

    if (inner_w <= 0 || inner_h <= 0) 
        return;

    // Screen size
    int screen_w{}, screen_h{};
    SDL_GetRenderOutputSize(m_sdl_renderer, &screen_w, &screen_h);

    // Scale
    float scale_x = float(screen_w) / float(visible_area[2]);
    float scale_y = float(screen_h) / float(visible_area[3]);

    // The position of the inner area in the visible area (in pixels)
    float inner_left   = (x_begin - visible_area[0]) * scale_x;
    float inner_top    = (y_map_begin - visible_area[1]) * scale_y;
    float inner_right  = (x_end - visible_area[0]) * scale_x;
    float inner_bottom = (y_map_end - visible_area[1]) * scale_y;

    float blit_w = inner_right - inner_left;
    float blit_h = inner_bottom - inner_top;

    SDL_FRect src_rect{
        float(x_begin), float(m_map.height()) - float(y_map_end), float(inner_w), float(inner_h)
    };
    SDL_FRect dst_rect{
        inner_left, inner_top, blit_w, blit_h
    };

    if(m_mode == Mode::Normal)
    {
        if (!m_buffer_surf_opt
            || m_buffer_surf_opt->w != int(m_map.width())
            || m_buffer_surf_opt->h != int(m_map.height())
            || m_buffer_surf_opt->pixels != m_map.colors.span().data())
        {
            m_buffer_surf_opt.reset(SDL_CreateSurfaceFrom(
                int(m_map.width()),
                int(m_map.height()),
                SDL_PIXELFORMAT_RGBA8888,
                m_map.colors.span().data(),
                int(m_map.width() * sizeof(m_map.colors(0, 0)))
            ));
            if(!m_buffer_surf_opt)
                throw std::runtime_error("Renderer::render: SDL_CreateSurfaceFrom returned nullptr");
        }

        SDL_SetRenderDrawColor(
            m_sdl_renderer,
            m_bg_color.r,
            m_bg_color.g,
            m_bg_color.b,
            m_bg_color.a
        );
        SDL_RenderFillRect(m_sdl_renderer, &dst_rect);
    }
    else
    {
        if (!m_buffer_surf_opt
            || m_buffer_surf_opt->w != int(m_map.width())
            || m_buffer_surf_opt->h != int(m_map.height()))
        {
            m_buffer_surf_opt.reset(SDL_CreateSurface(
                int(m_map.width()),
                int(m_map.height()),
                SDL_PIXELFORMAT_RGBX8888
            ));
            if(!m_buffer_surf_opt)
                throw std::runtime_error("Renderer::render: SDL_CreateSurface returned nullptr");
        }

        uint32_t *pixels = static_cast<uint32_t *>(m_buffer_surf_opt->pixels);
        int pitch = m_buffer_surf_opt->pitch;

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

    }

    SDL_UpdateTexture(
        m_map_tex_opt.get(),
        nullptr,
        m_buffer_surf_opt->pixels,
        m_buffer_surf_opt->pitch
    );
    SDL_RenderTextureRotated(
        m_sdl_renderer,
        m_map_tex_opt.get(),
        &src_rect,
        &dst_rect,
        0.0,
        nullptr,
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
        SDL_SetRenderDrawBlendMode(m_sdl_renderer, SDL_BLENDMODE_BLEND);
        m_buffer_surf_opt.reset();
    }
    else if(v == Mode::Thermal)
    {
        SDL_SetRenderDrawBlendMode(m_sdl_renderer, SDL_BLENDMODE_NONE);
        m_buffer_surf_opt.reset();
    }
    else
    {
        throw std::logic_error("Renderer::set_mode: invalid Mode value");
    }
    m_mode = v;
}
