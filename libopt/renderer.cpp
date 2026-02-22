#include "renderer.hpp"
#include <SDL2/SDL.h>
#include <SDL2pp/Exception.hh>
#include <algorithm>
#include <cstring>
#include <utility>
#include <variant>

Renderer::Renderer(GameMap &map, SDL2pp::Surface &dst_surface, uint32_t bg_color)
    : m_map(map)
    , m_dst_surface(dst_surface)
    , m_bg_color(bg_color)
{}

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
    int screen_w = m_dst_surface.GetWidth();
    int screen_h = m_dst_surface.GetHeight();

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
    if (!m_scale_buffer.has_value()
        || m_scale_buffer->GetWidth()  != blit_w
        || m_scale_buffer->GetHeight() != blit_h)
    {
        auto *surf = SDL_CreateRGBSurfaceWithFormat(
            0, blit_w, blit_h, 32,
            m_dst_surface.GetFormat()
        );
        if(surf == nullptr)
            throw SDL2pp::Exception("SDL_CreateRGBSurfaceWithFormat");
        m_scale_buffer.emplace(surf);
    }

    SDL2pp::Rect src_rect{x_begin, (int)m_map.height() - y_map_end, inner_w, inner_h};
    SDL2pp::Surface &src_surface = [this] -> auto &
    {
        if(m_mode == Mode::Normal)
            return m_map.colors.surface();
        if(m_mode == Mode::Thermal)
            return *m_thermal_buffer;
        std::unreachable();
    }();

    if(m_mode == Mode::Thermal)
    {
        // TODO: implement faster algorithm of thermal vision
        for(size_t y = src_rect.y; y < src_rect.y + src_rect.h; ++y)
        {
            for(size_t x = src_rect.x; x < src_rect.x + src_rect.w; ++x)
            {
                uint32_t rgba = m_map.colors(x, y);
                // 50 % of grayscale
                int32_t darkscale = (
                    ((rgba >> 24) & 0xFF)
                    + ((rgba >> 16) & 0xFF)
                    + ((rgba >> 8) & 0xFF)
                ) / 6;
                float temp_factor = m_map.temps(x, y) / 500.f;
                auto result_color = uint32_t(
                    (std::min(0xFF, darkscale + static_cast<int32_t>(temp_factor * 0xBF)) << 24)
                    | (std::clamp(darkscale + static_cast<int32_t>((temp_factor - 1) * 0x3F),
                                  0x00, 0xFF) << 16)
                    | (darkscale << 8)
                    | 0xFF
                );
                auto addr = (uint32_t *)src_surface.Get()->pixels;
                addr[y * src_surface.Get()->pitch / 4 + x] = result_color;
            }
        }
    }

    // Fill buffer surface with bg color and blit scaled
    uint32_t bg_color_native = SDL_MapRGBA(
        m_dst_surface.Get()->format,
        (m_bg_color >> 24) & 0xFF,
        (m_bg_color >> 16) & 0xFF,
        (m_bg_color >> 8) & 0xFF,
        0xFF
    );
    m_scale_buffer->FillRect(std::nullopt, bg_color_native);
    src_surface.BlitScaled(src_rect, *m_scale_buffer, SDL2pp::NullOpt);

    // Y-flip buffer surface
    if((int)m_row_buffer_size != blit_w * 4)
    {
        m_row_buffer = std::make_unique_for_overwrite<uint8_t[]>(blit_w * 4);
        m_row_buffer_size = blit_w * 4;
    }

    int scale_pitch = m_scale_buffer->Get()->pitch;
    auto lower_addr = (uint8_t *)m_scale_buffer->Get()->pixels;
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

    // Copy buffer surface onto destination
    int dst_pitch = m_dst_surface.Get()->pitch;

    for (int src_y = 0; src_y < blit_h; ++src_y) 
    {
        int dst_y = (int)inner_top + src_y;
        
        if (dst_y >= 0 && dst_y < screen_h) 
        {
            uint8_t *scale_row = (uint8_t *)m_scale_buffer->Get()->pixels + src_y * scale_pitch;
            uint8_t *dst_row = (uint8_t *)m_dst_surface.Get()->pixels + dst_y * dst_pitch;
            dst_row += (int)inner_left * 4;
            
            memcpy(dst_row, scale_row, blit_w * 4);
        }
    }
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
        m_thermal_buffer.reset();
    }
    else if(v == Mode::Thermal)
    {
        auto *surf = SDL_CreateRGBSurfaceWithFormat(
            0, m_map.width(), m_map.height(), 32, SDL_PIXELFORMAT_RGBX8888
        );
        if(surf == nullptr)
            throw SDL2pp::Exception("SDL_CreateRGBSurfaceWithFormat");
        m_thermal_buffer.emplace(surf);
    }
    else throw std::logic_error("Renderer::set_mode: invalid Mode value");
    m_mode = v;
}
