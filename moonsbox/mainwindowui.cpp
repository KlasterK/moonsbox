#include "mainwindowui.hpp"
#include "materialpalette.hpp"
#include "soundsystem.hpp"
#include <utility>
#include "ext/portable-file-dialogs.h"

namespace
{
    constexpr SDL_Color ButtonDefaultColor{0x7C, 0x64, 0x23, 0xFF};
    constexpr SDL_Color ButtonHoverColor{0x63, 0x51, 0x1F, 0xFF};
    constexpr SDL_Color ButtonPressedColor{0xA9, 0x89, 0x30, 0xFF};
    constexpr int Spacing = 10;
}

namespace Subwindow
{
    constexpr SDL_Color BorderColor{0x61, 0x4D, 0x16, 0xFF};
    constexpr SDL_Color FgColor{0xFF, 0xFF, 0xFF, 0xFF};
    constexpr SDL_Color BgColor{0x4E, 0x3F, 0x15, 0xFF};
    constexpr int BorderThickness = 2;
    constexpr int Width = 300;
    constexpr int InnerHeight = 100;
    constexpr int ButtonWidth = (Width - BorderThickness*2 - Spacing*3) / 2;
    constexpr int ButtonHeight = InnerHeight - Spacing*2;
}

MainWindowUI::Button MainWindowUI::Button::default_from_surface(
    SDL_Renderer *nonown_renderer,
    SDL_Surface *takeown_surf,
    std::string error_ctx_if_nullptr)
{
    if(!takeown_surf)
        throw std::runtime_error(std::move(error_ctx_if_nullptr));

    auto *tex = SDL_CreateTextureFromSurface(nonown_renderer, takeown_surf);
    SDL_DestroySurface(takeown_surf);
    if(!tex)
        throw std::runtime_error("MainWindowUI::Button::default_from_surface: SDL_CreateTextureFromSurface returned nullptr");

    return Button{
        {tex, SDL_DestroyTexture},
        {0.f, 0.f, float(tex->w), float(tex->h)},
        {0.f, 0.f, float(tex->w), float(tex->h)},
    };
}

MainWindowUI::Button MainWindowUI::Button::expanded_from_surface(
    SDL_Renderer *nonown_renderer,
    SDL_Surface *takeown_surf,
    const SDL_FPoint &topleft,
    std::string error_ctx_if_nullptr)
{
    auto btn = Button::default_from_surface(nonown_renderer, takeown_surf, error_ctx_if_nullptr);
    btn.content_rect.x = topleft.x + Spacing;
    btn.content_rect.y = topleft.y + Spacing;
    btn.total_rect.x = topleft.x;
    btn.total_rect.y = topleft.y;
    btn.total_rect.w += Spacing*2;
    btn.total_rect.h += Spacing*2;
    return btn;
}

void MainWindowUI::Button::render(SDL_Renderer *nonown_renderer, std::optional<SDL_FPoint> mouse_point)
{
    if(mouse_point)
    {
        const SDL_Color *color{};
        if(is_holded)
            color = &ButtonPressedColor;
        else if(SDL_PointInRectFloat(&*mouse_point, &total_rect))
            color = &ButtonHoverColor;
        else 
            color = &ButtonDefaultColor;

        SDL_SetRenderDrawColor(nonown_renderer, color->r, color->g, color->b, color->a);
        SDL_RenderFillRect(nonown_renderer, &total_rect);
    }
    SDL_RenderTexture(nonown_renderer, content_tex.get(), nullptr, &content_rect);
}

void MainWindowUI::Button::move(const SDL_FPoint &rel)
{
    total_rect.x += rel.x;
    total_rect.y += rel.y;
    content_rect.x += rel.x;
    content_rect.y += rel.y;
}

bool MainWindowUI::Button::do_act_on_release(const SDL_FPoint &mouse_point) const
{
    return is_holded && SDL_PointInRectFloat(&mouse_point, &total_rect);
}

void MainWindowUI::SavingSubwindow::render(SDL_Renderer *nonown_renderer, const SDL_FPoint &mouse_point)
{
    SDL_SetRenderDrawColor(
        nonown_renderer,
        Subwindow::BorderColor.r,
        Subwindow::BorderColor.g,
        Subwindow::BorderColor.b,
        Subwindow::BorderColor.a
    );
    SDL_RenderFillRect(nonown_renderer, &total_rect);

    SDL_SetRenderDrawColor(
        nonown_renderer,
        Subwindow::BgColor.r,
        Subwindow::BgColor.g,
        Subwindow::BgColor.b,
        Subwindow::BgColor.a
    );
    SDL_RenderFillRect(nonown_renderer, &inner_rect);

    load_btn.render(nonown_renderer, mouse_point);
    save_btn.render(nonown_renderer, mouse_point);
    title_btn.render(nonown_renderer, std::nullopt);
    x_btn.render(nonown_renderer, std::nullopt);
}

void MainWindowUI::SavingSubwindow::move(const SDL_FPoint &rel)
{
    for(Button *btn : buttons())
        btn->move(rel);

    inner_rect.x += rel.x;
    inner_rect.y += rel.y;
    total_rect.x += rel.x;
    total_rect.y += rel.y;
}

std::array<MainWindowUI::Button *, 4> MainWindowUI::SavingSubwindow::buttons()
{
    return {&x_btn, &title_btn, &load_btn, &save_btn};
}

void MainWindowUI::center_saving_subwindow()
{
    float title_w = m_saving_subwindow.title_btn.content_tex->w;
    float title_h = m_saving_subwindow.title_btn.content_tex->h;
    float x_w = m_saving_subwindow.x_btn.content_tex->w;
    float x_h = m_saving_subwindow.x_btn.content_tex->h;
    float load_w = m_saving_subwindow.load_btn.content_tex->w;
    float load_h = m_saving_subwindow.load_btn.content_tex->h;
    float save_w = m_saving_subwindow.save_btn.content_tex->w;
    float save_h = m_saving_subwindow.save_btn.content_tex->h;

    int screen_w{}, screen_h{};
    SDL_GetRenderOutputSize(m_renderer, &screen_w, &screen_h);
    SDL_FPoint topleft{
        float(screen_w) / 2 - Subwindow::Width / 2,
        float(screen_h) / 2 - (
            Subwindow::BorderThickness*3 + title_h + Subwindow::InnerHeight
        ) / 2,
    };

    m_saving_subwindow.title_btn.total_rect = {
        topleft.x, topleft.y, Subwindow::Width, Subwindow::BorderThickness*2 + title_h,
    };
    m_saving_subwindow.title_btn.content_rect = {
        topleft.x + Subwindow::Width / 2 - title_w / 2,
        topleft.y + Subwindow::BorderThickness,
        title_w,
        title_h,
    };

    m_saving_subwindow.x_btn.total_rect = {
        topleft.x + Subwindow::Width - Spacing - x_w,
        topleft.y + Subwindow::BorderThickness + title_h / 2 - x_h / 2,
        x_w,
        x_h,
    };
    m_saving_subwindow.x_btn.content_rect = m_saving_subwindow.x_btn.total_rect;

    m_saving_subwindow.load_btn.content_rect = {
        topleft.x + Subwindow::BorderThickness + Spacing
            + Subwindow::ButtonWidth / 2 - load_w / 2,
        topleft.y + Subwindow::BorderThickness*2 + title_h + Spacing
            + Subwindow::ButtonHeight / 2 - load_h / 2,
        load_w,
        load_h,
    };
    m_saving_subwindow.load_btn.total_rect = {
        topleft.x + Subwindow::BorderThickness + Spacing,
        topleft.y + Subwindow::BorderThickness*2 + title_h + Spacing,
        Subwindow::ButtonWidth,
        Subwindow::ButtonHeight,
    };

    m_saving_subwindow.save_btn.content_rect = {
        topleft.x + Subwindow::BorderThickness + Spacing*2 + Subwindow::ButtonWidth
            + Subwindow::ButtonWidth / 2 - save_w / 2,
        topleft.y + Subwindow::BorderThickness*2 + title_h + Spacing
            + Subwindow::ButtonHeight / 2 - save_h / 2,
        load_w,
        load_h,
    };
    m_saving_subwindow.save_btn.total_rect = {
        topleft.x + Subwindow::BorderThickness + Spacing*2 + Subwindow::ButtonWidth,
        topleft.y + Subwindow::BorderThickness*2 + title_h + Spacing,
        Subwindow::ButtonWidth,
        Subwindow::ButtonHeight,
    };

    m_saving_subwindow.inner_rect = {
        topleft.x + Subwindow::BorderThickness,
        topleft.y + Subwindow::BorderThickness*2 + title_h,
        Subwindow::Width - Subwindow::BorderThickness*2,
        Subwindow::InnerHeight,
    };

    m_saving_subwindow.total_rect = {
        topleft.x,
        topleft.y,
        Subwindow::Width,
        Subwindow::BorderThickness*3 + title_h + Subwindow::InnerHeight
    };
}

MainWindowUI::MainWindowUI(SDL_Renderer *nonown_renderer,
                           TTF_Font *nonown_font,
                           MaterialPalette &palette,
                           FileCallback load_save_cb,
                           FileCallback store_save_cb)
    : m_renderer(nonown_renderer)
    , m_font(nonown_font)
    , m_palette(palette)
    , m_palette_btn{Button::expanded_from_surface(
        nonown_renderer,
        SDL_LoadSurface("assets/materials_palette_btn_icon"),
        {Spacing, Spacing},
        "MainWindowUI::MainWindowUI: SDL_LoadSurface returned nullptr"
    )}
    , m_saving_btn{Button::expanded_from_surface(
        nonown_renderer,
        SDL_LoadSurface("assets/saving_btn_icon"),
        {Spacing*2 + m_palette_btn.total_rect.w, Spacing},
        "MainWindowUI::MainWindowUI: SDL_LoadSurface returned nullptr"
    )}
    , m_saving_subwindow{
        .title_btn{Button::default_from_surface(
            nonown_renderer,
            TTF_RenderText_Shaded(nonown_font, "Saving", 0, Subwindow::FgColor, Subwindow::BorderColor),
            "MainWindowUI::MainWindowUI: TTF_RenderText_Shaded returned nullptr"
        )},
        .x_btn{Button::default_from_surface(
            nonown_renderer,
            TTF_RenderGlyph_Shaded(nonown_font, 0xD7, Subwindow::FgColor, Subwindow::BorderColor),
            "MainWindowUI::MainWindowUI: TTF_RenderGlyph_Shaded returned nullptr"
        )},
        .load_btn{Button::default_from_surface(
            nonown_renderer,
            TTF_RenderText_Blended(nonown_font, "Load", 0, Subwindow::FgColor),
            "MainWindowUI::MainWindowUI: TTF_RenderText_Blended returned nullptr"
        )},
        .save_btn{Button::default_from_surface(
            nonown_renderer,
            TTF_RenderText_Blended(nonown_font, "Save", 0, Subwindow::FgColor),
            "MainWindowUI::MainWindowUI: TTF_RenderText_Blended returned nullptr"
        )},
    }
    , m_load_save_cb(std::move(load_save_cb))
    , m_store_save_cb(std::move(store_save_cb))
{}

bool MainWindowUI::process_event(const SDL_Event &event)
{
    if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        SDL_FPoint mouse_point{event.button.x, event.button.y};

        for(Button *btn : {&m_palette_btn, &m_saving_btn})
        {
            if(SDL_PointInRectFloat(&mouse_point, &btn->total_rect))
            {
                btn->is_holded = true;
                return true;
            }
        }

        if(!m_is_saving_subwindow_visible)
            return false;

        for(Button *btn : m_saving_subwindow.buttons())
        {
            if(SDL_PointInRectFloat(&mouse_point, &btn->total_rect))
            {
                btn->is_holded = true;
                return true;
            }
        }
        return false;
    }

    if(event.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        SDL_FPoint mouse_point{event.button.x, event.button.y};

        if(m_palette_btn.do_act_on_release(mouse_point))
        {
            sfx::play_sound("palette.show", "ui", true);
            m_palette.show();
        }
        else if(m_saving_btn.do_act_on_release(mouse_point))
        {
            m_is_saving_subwindow_visible = true;
            center_saving_subwindow();
        }
        else if(m_is_saving_subwindow_visible)
        {
            if(m_saving_subwindow.x_btn.do_act_on_release(mouse_point))
            {
                m_is_saving_subwindow_visible = false;
            }
            else if(m_saving_subwindow.load_btn.do_act_on_release(mouse_point))
            {
                std::vector<std::string> filters{
                    "Moonsbox Save", "*.kk-save", "All Files", "*",
                };
                pfd::open_file open_file("Load Save", "user", std::move(filters));
                if(auto result_vec = open_file.result(); !result_vec.empty())
                {
                    m_load_save_cb(result_vec.at(0));
                }
                m_is_saving_subwindow_visible = false;
            }
            else if(m_saving_subwindow.save_btn.do_act_on_release(mouse_point))
            {
                std::vector<std::string> filters{
                    "Moonsbox Save", "*.kk-save", "All Files", "*",
                };
                pfd::save_file save_file("Store Save", "user", std::move(filters));
                if(auto result_str = save_file.result(); !result_str.empty())
                {
                    m_store_save_cb(result_str);
                }
                m_is_saving_subwindow_visible = false;
            }
        }

        for(Button *btn : {&m_palette_btn, &m_saving_btn})
            btn->is_holded = false;

        if(m_is_saving_subwindow_visible)
        {
            for(Button *btn : m_saving_subwindow.buttons())
                btn->is_holded = false;
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION
        && m_is_saving_subwindow_visible
        && m_saving_subwindow.title_btn.is_holded)
    {
        m_saving_subwindow.move({event.motion.xrel, event.motion.yrel});
    }

    return false;
}

void MainWindowUI::render()
{
    SDL_FPoint mouse_point{};
    SDL_GetMouseState(&mouse_point.x, &mouse_point.y);

    for(auto *btn : {&m_palette_btn, &m_saving_btn})
        btn->render(m_renderer, mouse_point);

    if(m_is_saving_subwindow_visible)
        m_saving_subwindow.render(m_renderer, mouse_point);

    m_fps_counter.tick();
    auto fps_text = std::format(
        "FPS: Avg {:.2f} Min {:.2f} Max {:.2f}",
        m_fps_counter.avg_fps(),m_fps_counter.min_fps(), m_fps_counter.max_fps()
    );

    auto fake_fps_btn = Button::default_from_surface(
        m_renderer,
        TTF_RenderText_Blended(m_font, fps_text.data(), fps_text.size(), {0, 255, 0, 255}),
        "MainWindowUI::render: TTF_RenderText_Blended returned nullptr"
    );
    fake_fps_btn.content_rect.x = Spacing;
    fake_fps_btn.content_rect.y = Spacing*2 + m_palette_btn.total_rect.h;
    fake_fps_btn.render(m_renderer, std::nullopt);
}
