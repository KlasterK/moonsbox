#include "materialpalette.hpp"
#include <cmath>
#include <simulationengine/core/materialregistry.hpp>
#include <concepts>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace
{
    constexpr int IconWidth{64};
    constexpr int IconHeight{64};
    constexpr int Margin{20};
    constexpr SDL_Color BgColor{0x00, 0x00, 0x00, 0xCC};
    constexpr SDL_Color FgColor{0xFF, 0xFF, 0xFF, 0xFF};
    constexpr SDL_Color SelectionInnerColor{0x00, 0x8C, 0xFF, 0x20};
    constexpr SDL_Color SelectionOuterColor{0x00, 0xC3, 0xFF, 0x40};
    constexpr int OutlineWidth{3};
    constexpr int OutlineRadius{8};

    template<std::integral T>
    T ceildiv(T a, T b)
    {
        return (a + b - 1) / b;
    }

    struct Arc
    {
        SDL_FPoint centre{};
        SDL_FPoint topleft{};

        Arc(float origin_x, float origin_y, float x_offset_to_centre, float y_offset_to_centre)
            : centre{origin_x + OutlineRadius * x_offset_to_centre,
                     origin_y + OutlineRadius * y_offset_to_centre}
            , topleft{x_offset_to_centre > 0.f ? origin_x : origin_x - OutlineRadius,
                      y_offset_to_centre > 0.f ? origin_y : origin_y - OutlineRadius}
        {}
    };

    constexpr size_t ArcFillPixelsApproximateCount = static_cast<size_t>(
        6.2831f * (OutlineRadius - OutlineWidth) * (OutlineRadius - OutlineWidth) / 8.f + 4.f
    );

    constexpr size_t ArcOutlinePixelsApproximateCount = static_cast<size_t>(
        6.2831f * OutlineWidth * (2 * OutlineRadius - OutlineWidth) / 8.f + 4.f
    );

    void draw_selection(SDL_Renderer *renderer, const SDL_FRect *full_rect)
    {
        const std::array<Arc, 4> arcs({
            {full_rect->x,                full_rect->y,                 1.f,  1.f},
            {full_rect->x + full_rect->w, full_rect->y,                -1.f,  1.f},
            {full_rect->x,                full_rect->y + full_rect->h,  1.f, -1.f},
            {full_rect->x + full_rect->w, full_rect->y + full_rect->h, -1.f, -1.f},
        });

        std::vector<SDL_FPoint> fill_points, outline_points;
        fill_points.reserve(ArcFillPixelsApproximateCount);
        outline_points.reserve(ArcOutlinePixelsApproximateCount);

        for(const auto &[centre, topleft] : arcs)
        {
            for(float x = topleft.x; x < topleft.x + OutlineRadius; ++x)
            {
                for(float y = topleft.y; y < topleft.y + OutlineRadius; ++y)
                {
                    float left_cmp = (x - centre.x) * (x - centre.x) + (y - centre.y) * (y - centre.y);
                    if(left_cmp <= (OutlineRadius - OutlineWidth) * (OutlineRadius - OutlineWidth))
                    {
                        fill_points.emplace_back(x, y);
                    }
                    else if(left_cmp <= OutlineRadius * OutlineRadius)
                    {
                        outline_points.emplace_back(x, y);
                    }
                }
            }

            SDL_SetRenderDrawColor(
                renderer, SelectionInnerColor.r, SelectionInnerColor.g,
                SelectionInnerColor.b, SelectionInnerColor.a
            );
            SDL_RenderPoints(renderer, fill_points.data(), fill_points.size());

            SDL_SetRenderDrawColor(
                renderer, SelectionOuterColor.r, SelectionOuterColor.g,
                SelectionOuterColor.b, SelectionOuterColor.a
            );
            SDL_RenderPoints(renderer, outline_points.data(), outline_points.size());

            fill_points.clear();
            outline_points.clear();
        }

        std::array<SDL_FRect, 4> outline_rects({
            {
                full_rect->x + OutlineRadius,
                full_rect->y,
                full_rect->w - OutlineRadius*2,
                OutlineWidth
            },
            {
                full_rect->x,
                full_rect->y + OutlineRadius,
                OutlineWidth,
                full_rect->h - OutlineRadius*2
            },
            {
                full_rect->x + OutlineRadius,
                full_rect->y + full_rect->h - OutlineWidth,
                full_rect->w - OutlineRadius*2,
                OutlineWidth
            },
            {
                full_rect->x + full_rect->w - OutlineWidth,
                full_rect->y + OutlineRadius,
                OutlineWidth,
                full_rect->h - OutlineRadius*2
            },
        });
        SDL_SetRenderDrawColor(
            renderer, SelectionOuterColor.r, SelectionOuterColor.g,
            SelectionOuterColor.b, SelectionOuterColor.a
        );
        SDL_RenderFillRects(renderer, outline_rects.data(), outline_rects.size());

        std::array<SDL_FRect, 3> fill_rects({
            {
                full_rect->x + OutlineRadius,
                full_rect->y + OutlineWidth,
                full_rect->w - OutlineRadius*2,
                full_rect->h - OutlineWidth*2,
            },
            {
                full_rect->x + OutlineWidth,
                full_rect->y + OutlineRadius,
                OutlineRadius - OutlineWidth,
                full_rect->h - OutlineRadius*2,
            },
            {
                full_rect->x + full_rect->w - OutlineRadius,
                full_rect->y + OutlineRadius,
                OutlineRadius - OutlineWidth,
                full_rect->h - OutlineRadius*2,
            },
        });
        SDL_SetRenderDrawColor(
            renderer, SelectionInnerColor.r, SelectionInnerColor.g,
            SelectionInnerColor.b, SelectionInnerColor.a
        );
        SDL_RenderFillRects(renderer, fill_rects.data(), fill_rects.size());
    }
}

MaterialPalette::MaterialPalette(SDL_Renderer *nonown_sdl_renderer,
                                 MaterialRegistry &registry,
                                 TTF_Font *nonown_font)
    : m_registry(registry)
    , m_rnd(nonown_sdl_renderer)
{
    TTF_SetFontWrapAlignment(nonown_font, TTF_HORIZONTAL_ALIGN_CENTER);
    m_slot_texs.reserve(std::distance(registry.begin(), registry.end()));
    for(const auto &pair : registry)
    {
        std::unique_ptr<SDL_Surface, void (&)(SDL_Surface *)> label_surf{
            TTF_RenderText_Blended_Wrapped(
                nonown_font,
                pair.first.data(),
                pair.first.size(),
                FgColor,
                IconWidth
            ),
            SDL_DestroySurface
        };
        if(!label_surf)
            throw std::runtime_error("MaterialPalette::MaterialPalette: TTF_RenderText_Blended_Wrapped returned nullptr");

        std::unique_ptr<SDL_Surface, void (&)(SDL_Surface *)> merged_surf{
            SDL_CreateSurface(IconWidth, IconHeight + label_surf->h, SDL_PIXELFORMAT_RGBA8888),
            SDL_DestroySurface
        };
        if(!merged_surf)
            throw std::runtime_error("MaterialPalette::MaterialPalette: SDL_CreateSurface returned nullptr");

        std::string icon_path = "assets/materials/" + pair.first;
        std::unique_ptr<SDL_Surface, void (&)(SDL_Surface *)> icon_surf{
            SDL_LoadSurface(icon_path.c_str()),
            SDL_DestroySurface
        };
        if(icon_surf)
        {
            SDL_Rect dst_rect{0, 0, IconWidth, IconHeight};
            if(icon_surf->w == IconWidth && icon_surf->h == IconHeight)
            {
                SDL_BlitSurface(icon_surf.get(), nullptr, merged_surf.get(), &dst_rect);
            }
            else
            {
                SDL_BlitSurfaceScaled(icon_surf.get(), nullptr, merged_surf.get(), &dst_rect, SDL_SCALEMODE_LINEAR);
            }
        }

        SDL_Rect dst_rect{
            IconWidth / 2 - label_surf->w / 2,
            IconHeight,
            label_surf->w,
            label_surf->h,
        };
        SDL_BlitSurface(label_surf.get(), nullptr, merged_surf.get(), &dst_rect);

        m_slot_texs.emplace_back(
            SDL_CreateTextureFromSurface(m_rnd, merged_surf.get()),
            SDL_DestroyTexture
        );
        if(!m_slot_texs.back())
            throw std::runtime_error("MaterialPalette::MaterialPalette: SDL_CreateTextureFromSurface returned nullptr");
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
    int screen_w{};
    SDL_GetRenderOutputSize(m_rnd, &screen_w, nullptr);
    int columns_count = screen_w / (IconWidth + Margin);
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

    int screen_w{}, screen_h{};
    SDL_GetRenderOutputSize(m_rnd, &screen_w, &screen_h);
    if(mx >= screen_w || my >= screen_h)
        return std::nullopt;

    auto max_slot_it = std::ranges::max_element(m_slot_texs, {}, &SDL_Texture::h);
    int max_slot_height = (*max_slot_it)->h;
    size_t columns_count = static_cast<size_t>(screen_w / (IconWidth + Margin));

    int half_icon_widths  = IconWidth * columns_count / 2;
    int half_hor_margins  = Margin * (columns_count - 1) / 2;

    int initial_x = (screen_w / 2) - half_icon_widths - half_hor_margins;
    SDL_Rect slot_rect{initial_x, initial_x, IconWidth, max_slot_height};
    SDL_Point mouse_point{mx, my};

    for(size_t i{}; i < m_slot_texs.size(); ++i)
    {
        if(SDL_PointInRect(&mouse_point, &slot_rect))
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

void MaterialPalette::render()
{
    if(!m_is_visible)
        return;

    int screen_w{};
    SDL_GetRenderOutputSize(m_rnd, &screen_w, nullptr);
    SDL_SetRenderDrawColor(m_rnd, BgColor.r, BgColor.g, BgColor.b, BgColor.a);
    SDL_SetRenderDrawBlendMode(m_rnd, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(m_rnd, nullptr);

    auto max_slot_it = std::ranges::max_element(m_slot_texs, {}, &SDL_Texture::h);
    int max_slot_height = (*max_slot_it)->h;
    size_t columns_count = static_cast<size_t>(screen_w / (IconWidth + Margin));

    int half_icon_widths  = IconWidth * columns_count / 2;
    int half_hor_margins  = Margin * (columns_count - 1) / 2;

    int initial_x = (screen_w / 2) - half_icon_widths - half_hor_margins;
    SDL_FRect dst_rect{float(initial_x), float(initial_x), 0.f, 0.f};

    for(size_t i{}; i < m_slot_texs.size(); ++i)
    {
        if(i == m_selection_idx)
        {
            SDL_FRect full_rect{
                dst_rect.x - float(Margin) / 4 - OutlineWidth,
                dst_rect.y - float(Margin) / 4 - OutlineWidth,
                m_slot_texs[i]->w + OutlineWidth*2 + float(Margin) / 2,
                m_slot_texs[i]->h + OutlineWidth*2 + float(Margin) / 2,
            };
            draw_selection(m_rnd, &full_rect);
        }

        dst_rect.w = m_slot_texs[i]->w;
        dst_rect.h = m_slot_texs[i]->h;
        SDL_RenderTexture(m_rnd, m_slot_texs[i].get(), nullptr, &dst_rect);

        if(i % columns_count == columns_count - 1)
        {
            dst_rect.x = initial_x;
            dst_rect.y += Margin + max_slot_height;
        }
        else
        {
            dst_rect.x += Margin + IconWidth;
        }
    }
}
