#include "renderer.hpp"
#include <pybind11/pybind11.h>
#include <SDL2/SDL.h>
#include <iostream>

#define RAISE(x, y) (PyErr_SetString((x), (y)), nullptr)
extern "C"
{
#include "include/pygame-ce/include/_pygame.h"
}

Renderer::Renderer(GameMap &map, py::object &dst_surface, uint32_t bg_color)
    : m_map(map)
    , m_dst_pgsurf(dst_surface)
    , m_scale_buffer(nullptr)
    , m_bg_color(bg_color)
{}

Renderer::~Renderer()
{
    if(m_scale_buffer)
        SDL_FreeSurface(m_scale_buffer);
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
    int screen_w = pgSurface_AsSurface(m_dst_pgsurf.ptr())->w;
    int screen_h = pgSurface_AsSurface(m_dst_pgsurf.ptr())->h;

    // Scale
    double scale_x = (double)screen_w / (double)visible_area[2];
    double scale_y = (double)screen_h / (double)visible_area[3];

    // The position of the inner area in the visible area (in pixels)
    double inner_left = (x_begin - visible_area[0]) * scale_x;
    double inner_top = (y_map_begin - visible_area[1]) * scale_y;
    double inner_right = (x_end - visible_area[0]) * scale_x;
    double inner_bottom = (y_map_end - visible_area[1]) * scale_y;

    int blit_w = (int)(inner_right - inner_left);
    int blit_h = (int)(inner_bottom - inner_top);

    // Create or resize buffer surface for inner area
    if (m_scale_buffer == nullptr
        || m_scale_buffer->w != blit_w
        || m_scale_buffer->h != blit_h)
    {
        if (m_scale_buffer) 
            SDL_FreeSurface(m_scale_buffer);

        m_scale_buffer = SDL_CreateRGBSurfaceWithFormat(
            0, blit_w, blit_h, 32,
            pgSurface_AsSurface(m_dst_pgsurf.ptr())->format->format
        );
    }

    // Fill buffer surface with bg color and blit scaled
    uint32_t bg_color_native = SDL_MapRGBA(
        pgSurface_AsSurface(m_dst_pgsurf.ptr())->format,
        // RGBA8888
        (m_bg_color >> 24) & 0xFF,
        (m_bg_color >> 16) & 0xFF,
        (m_bg_color >> 8) & 0xFF,
        0xFF
    );
    SDL_FillRect(m_scale_buffer, nullptr, bg_color_native);
    SDL_Rect src_rect{x_begin, (int)m_map.height() - y_map_end, inner_w, inner_h};
    SDL_BlitScaled(&m_map.colors.surface(), &src_rect, m_scale_buffer, nullptr);

    // Y-flip buffer surface
    if((int)m_row_buffer_size != blit_w * 4)
    {
        m_row_buffer = std::make_unique_for_overwrite<uint8_t[]>(blit_w * 4);
        m_row_buffer_size = blit_w * 4;
    }

    int scale_pitch = m_scale_buffer->pitch;
    auto lower_addr = (uint8_t *)m_scale_buffer->pixels;
    auto upper_addr = lower_addr + scale_pitch * (blit_h - 1);
    auto max_lower_addr = lower_addr + scale_pitch * (blit_h / 2);

    while(lower_addr < max_lower_addr)
    {
        memcpy(m_row_buffer.get(), lower_addr, blit_w * 4);
        memcpy(lower_addr, upper_addr, blit_w * 4);
        memcpy(upper_addr, m_row_buffer.get(), blit_w * 4);
        lower_addr += scale_pitch;
        upper_addr -= scale_pitch;
    }

    // Copy buffer surface onto pygame surface
    SDL_Surface *dst_surface = pgSurface_AsSurface(m_dst_pgsurf.ptr());
    int dst_pitch = dst_surface->pitch;

    for (int src_y = 0; src_y < blit_h; ++src_y) 
    {
        int dst_y = (int)inner_top + src_y;
        
        if (dst_y >= 0 && dst_y < screen_h) 
        {
            uint8_t *scale_row = (uint8_t *)m_scale_buffer->pixels + src_y * scale_pitch;
            uint8_t *dst_row = (uint8_t *)dst_surface->pixels + dst_y * dst_pitch;
            dst_row += (int)inner_left * 4;
            
            memcpy(dst_row, scale_row, blit_w * 4);
        }
    }
}
