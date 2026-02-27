#include "materialpalette.hpp"
#include "SDL2/SDL_blendmode.h"
#include "SDL2/SDL_pixels.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include <SDL2pp/Optional.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Surface.hh>
#include <iterator>
#include <optional>
#include <stdexcept>

namespace
{
    constexpr int IconWidth = 64;
    constexpr int IconHeight = 64;
    constexpr int Margin = 20;
    constexpr SDL2pp::Color BgColor{0x00, 0x00, 0x00, 0xCC};
    constexpr SDL2pp::Color FgColor{0xFF, 0xFF, 0xFF, 0xFF};
    constexpr SDL2pp::Color SelectionInnerColor = {0x00, 0x20, 0x40};
    constexpr SDL2pp::Color SelectionOuterColor = {0x00, 0x60, 0x80};

    template<std::integral T>
    T ceildiv(T a, T b)
    {
        return (a + b - 1) / b;
    }
}

MaterialPalette::MaterialPalette(SDL2pp::Renderer &rnd, MaterialRegistry &registry, 
                                 SDL2pp::Font &font)
    : m_registry(registry)
    , m_rnd(rnd)
{
    TTF_SetFontWrappedAlign(font.Get(), TTF_WRAPPED_ALIGN_CENTER);
    m_slot_texs.reserve(std::distance(registry.begin(), registry.end()));
    for(auto it = registry.begin(); it != registry.end(); std::advance(it, 1))
    {
        auto *label_surf_raw = TTF_RenderText_Blended_Wrapped(
            font.Get(), it->first.c_str(), FgColor, IconWidth
        );
        if(label_surf_raw == nullptr)
            throw SDL2pp::Exception("TTF_RenderText_Blended_Wrapped");

        SDL2pp::Surface label_surf{label_surf_raw};
        SDL2pp::Surface icon_surf{std::format("assets/materials/{}", it->first)};

        auto *merged_surf_raw = SDL_CreateRGBSurfaceWithFormat(
            0, IconWidth, IconHeight + label_surf.GetHeight(), 32, SDL_PIXELFORMAT_RGBA8888
        );
        if(merged_surf_raw == nullptr)
            throw SDL2pp::Exception("SDL_CreateRGBSurfaceWithFormat");
        
        SDL2pp::Surface merged_surf{merged_surf_raw};
        icon_surf.Blit(SDL2pp::NullOpt, merged_surf, {0, 0, IconWidth, IconHeight});

        int label_x = IconWidth / 2 - label_surf.GetWidth() / 2;
        label_surf.Blit(SDL2pp::NullOpt, merged_surf, {{label_x, IconHeight}, label_surf.GetSize()});

        m_slot_texs.emplace_back(rnd, merged_surf);
    }
}

MaterialController &MaterialPalette::get_selected_material() const
{
    return *std::next(m_registry.begin(), m_selection_idx)->second;
}

size_t MaterialPalette::get_current_selection_idx() const
{
    return m_selection_idx;
}

void MaterialPalette::set_current_selection_idx(size_t idx)
{
    size_t nitems = std::distance(m_registry.begin(), m_registry.end());
    if(idx >= nitems)
        throw std::logic_error("MaterialPalette::set_current_selection: index out of bounds");

    m_selection_idx = idx;
}

bool MaterialPalette::move_selection(int columns_right, int rows_down)
{
    auto screen_size = m_rnd.GetOutputSize();
    int columns_count = screen_size.x / (IconWidth + Margin);
    int current_column = m_selection_idx % columns_count;

    int new_column = current_column + columns_right;
    if(new_column < 0 || new_column >= columns_count)
        return false;

    int new_idx = m_selection_idx + columns_right + rows_down * columns_count;
    if(new_idx < 0 || new_idx >= std::distance(m_registry.begin(), m_registry.end()))
        return false;

    m_selection_idx = new_idx;
    return true;
}

std::optional<size_t> MaterialPalette::find_material_idx(MaterialController &ctl) const
{
    for(auto it = m_registry.begin(); it != m_registry.end(); ++it)
    {
        if(it->second == &ctl)
            return std::distance(m_registry.begin(), it);
    }
    return std::nullopt;
}

std::optional<size_t> MaterialPalette::match_click_to_idx(int mx, int my) const
{
    if(mx < 0 || my < 0)
        return std::nullopt;

    auto screen_size = m_rnd.GetOutputSize();
    if(mx >= screen_size.x || my >= screen_size.y)
        return std::nullopt;

    auto highest_tex = std::ranges::max_element(m_slot_texs, {}, &SDL2pp::Texture::GetHeight);
    int max_slot_height = highest_tex->GetHeight();

    size_t columns_count = static_cast<size_t>(screen_size.x / (IconWidth + Margin));
    
    int half_icon_widths  = IconWidth * columns_count / 2;
    int half_hor_margins  = Margin * (columns_count - 1) / 2;

    int initial_x = (screen_size.x / 2) - half_icon_widths - half_hor_margins;
    SDL2pp::Rect slot_rect{initial_x, initial_x, IconWidth, max_slot_height};

    for(size_t i{}; i < m_slot_texs.size(); ++i)
    {
        if(slot_rect.Contains(mx, my))
        {
            return i;
        }

        if(i % columns_count == (columns_count - 1))
        {
            slot_rect.x = initial_x;
            slot_rect.y += Margin + max_slot_height;
        }
        else
        {
            slot_rect.x += Margin + IconWidth;
        }
    }
    return std::nullopt;
}

void MaterialPalette::scroll(int rows_down)
{
    (void)rows_down;
    return;
}

void MaterialPalette::show()
{
    m_is_visible = true;
}

void MaterialPalette::hide()
{
    m_is_visible = false;
}

bool MaterialPalette::is_visible() const
{
    return m_is_visible;
}

static void _draw_rounded_rect_fill_outline_solid(
    SDL2pp::Renderer &renderer,
    const SDL2pp::Rect &rect,
    int width,
    int radius,
    SDL2pp::Color fill_color,
    SDL2pp::Color outline_color
)
{
    for(auto [cx, cy] : std::initializer_list<std::array<int, 2>>{
        // Top left arc
        {rect.x - width + radius, rect.y - width + radius}, 
        // Top right arc
        {rect.x + rect.w + width - radius - 1, rect.y - width + radius},
        // Bottom left arc
        {rect.x - width + radius, rect.y + rect.h + width - radius - 1},
        // Bottom right arc
        {rect.x + rect.w + width - radius - 1, rect.y + rect.h + width - radius - 1},
    })
    {
        filledCircleRGBA(
            renderer.Get(),
            cx,
            cy,
            radius,
            outline_color.r,
            outline_color.g,
            outline_color.b,
            0xFF
        );
    }

    roundedBoxRGBA(
        renderer.Get(), 
        rect.x,
        rect.y,
        rect.x + rect.w,
        rect.y + rect.h,
        radius,
        fill_color.r,
        fill_color.g,
        fill_color.b,
        0xFF
    );

    auto line_rects = std::to_array<SDL2pp::Rect>({
        // Left line
        {rect.x - width, rect.y + radius, width, rect.h - radius * 2},
        // Top line
        {rect.x + radius, rect.y - width, rect.w - radius * 2, width},
        // Right line
        {rect.x + rect.w, rect.y + radius, width, rect.h - radius * 2},
        // Bottom line
        {rect.x + radius, rect.y + rect.h, rect.w - radius * 2, width},
    });
    renderer.SetDrawColor(outline_color);
    renderer.SetDrawBlendMode(SDL_BLENDMODE_NONE);
    renderer.FillRects(line_rects.data(), line_rects.size());
};

void MaterialPalette::render()
{
    if(!m_is_visible)
        return;

    auto screen_size = m_rnd.GetOutputSize();
    m_rnd.SetDrawColor(BgColor);
    m_rnd.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    m_rnd.FillRect({0, 0}, screen_size);

    auto highest_tex = std::ranges::max_element(m_slot_texs, {}, &SDL2pp::Texture::GetHeight);
    int max_slot_height = highest_tex->GetHeight();

    size_t columns_count = static_cast<size_t>(screen_size.x / (IconWidth + Margin));
    
    int half_icon_widths  = IconWidth * columns_count / 2;
    int half_hor_margins  = Margin * (columns_count - 1) / 2;

    int initial_x = (screen_size.x / 2) - half_icon_widths - half_hor_margins;
    SDL2pp::Point dst_point{initial_x, initial_x};

    for(size_t i{}; i < m_slot_texs.size(); ++i)
    {
        if(i == m_selection_idx)
        {
            SDL2pp::Rect sel_rect{
                dst_point.x - Margin / 2,
                dst_point.y - Margin / 2,
                IconWidth + Margin,
                m_slot_texs[i].GetHeight() + Margin,
            };
            _draw_rounded_rect_fill_outline_solid(
                m_rnd, sel_rect, 3, 8, SelectionInnerColor, SelectionOuterColor
            );
        }

        m_rnd.Copy(m_slot_texs[i], SDL2pp::NullOpt, dst_point);

        if(i % columns_count == (columns_count - 1))
        {
            dst_point.x = initial_x;
            dst_point.y += Margin + max_slot_height;
        }
        else
        {
            dst_point.x += Margin + IconWidth;
        }
    }
}