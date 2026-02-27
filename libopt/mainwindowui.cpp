#include "mainwindowui.hpp"
#include "materialpalette.hpp"
#include "soundsystem.hpp"
#include <SDL2pp/Color.hh>
#include <SDL2pp/Optional.hh>
#include <SDL2pp/Point.hh>
#include <SDL2pp/Texture.hh>
#include <SDL_events.h>
#include <SDL_mouse.h>
#include <utility>

namespace
{
    constexpr SDL2pp::Color ButtonDefaultColor{0x7C, 0x64, 0x23};
    constexpr SDL2pp::Color ButtonHoverColor{0x63, 0x51, 0x1F};
    constexpr SDL2pp::Color ButtonPressedColor{0xA9, 0x89, 0x30};
    constexpr int Spacing = 10;
}

namespace Subwindow
{
    constexpr SDL2pp::Color BorderColor{0x61, 0x4D, 0x16};
    constexpr SDL2pp::Color FgColor{0xFF, 0xFF, 0xFF};
    constexpr SDL2pp::Color BgColor{0x4E, 0x3F, 0x15};
    constexpr int BorderThickness = 2;
    constexpr int Width = 300;
    constexpr int InnerHeight = 100;
    constexpr int ButtonWidth = (Width - BorderThickness*2 - Spacing*3) / 2;
    constexpr int ButtonHeight = InnerHeight - Spacing*2;
}

MainWindowUI::Button MainWindowUI::Button::expanded_from_texture(
    SDL2pp::Texture &&tex, SDL2pp::Point topleft
)
{
    SDL2pp::Point size = tex.GetSize();
    return Button{
        std::move(tex),
        topleft + SDL2pp::Point{Spacing, Spacing},
        {topleft, size + SDL2pp::Point{Spacing*2, Spacing*2}},
    };
}

void MainWindowUI::Button::render(SDL2pp::Renderer &renderer, int mx, int my)
{
    if(is_holded)
        renderer.SetDrawColor(ButtonPressedColor);
    else if(total_rect.Contains(mx, my))
        renderer.SetDrawColor(ButtonHoverColor);
    else 
        renderer.SetDrawColor(ButtonDefaultColor);

    renderer.FillRect(total_rect);
    renderer.Copy(content_tex, SDL2pp::NullOpt, content_point);
}

void MainWindowUI::Button::move(SDL2pp::Point rel)
{
    total_rect += rel;
    content_point += rel;
}

bool MainWindowUI::Button::do_act_on_release(int mx, int my) const
{
    return is_holded && total_rect.Contains(mx, my);
}

void MainWindowUI::SavingSubwindow::render(SDL2pp::Renderer &renderer, int mx, int my)
{
    renderer.SetDrawColor(Subwindow::BorderColor);
    renderer.FillRect(total_rect);

    renderer.SetDrawColor(Subwindow::BgColor);
    renderer.FillRect(inner_rect);

    for(Button *btn : {&load_btn, &save_btn})
        btn->render(renderer, mx, my);

    renderer.Copy(title_btn.content_tex, SDL2pp::NullOpt, title_btn.content_point);
    renderer.Copy(x_btn.content_tex, SDL2pp::NullOpt, x_btn.content_point);
}

void MainWindowUI::SavingSubwindow::move(SDL2pp::Point rel)
{
    for(Button *btn : buttons())
        btn->move(rel);

    inner_rect += rel;
    total_rect += rel;
}

std::array<MainWindowUI::Button *, 4> MainWindowUI::SavingSubwindow::buttons()
{
    return {&x_btn, &title_btn, &load_btn, &save_btn};
}

void MainWindowUI::center_saving_subwindow()
{
    auto [title_w, title_h] = m_saving_subwindow.title_btn.content_tex.GetSize();
    auto [x_w, x_h] = m_saving_subwindow.x_btn.content_tex.GetSize();
    auto [load_w, load_h] = m_saving_subwindow.load_btn.content_tex.GetSize();
    auto [save_w, save_h] = m_saving_subwindow.save_btn.content_tex.GetSize();

    SDL2pp::Point screen_size = m_renderer.GetOutputSize();
    SDL2pp::Point topleft{
        screen_size.x / 2 - Subwindow::Width / 2,
        screen_size.y / 2 - (
            Subwindow::BorderThickness*3 + title_h + Subwindow::InnerHeight
        ) / 2,
    };

    m_saving_subwindow.title_btn.total_rect = {
        topleft, {Subwindow::Width, Subwindow::BorderThickness*2 + title_h}
    };
    m_saving_subwindow.title_btn.content_point = {
        topleft.x + Subwindow::Width / 2 - title_w / 2,
        topleft.y + Subwindow::BorderThickness,
    };

    m_saving_subwindow.x_btn.total_rect = {
        topleft.x + Subwindow::Width - Spacing - x_w,
        topleft.y + Subwindow::BorderThickness + title_h / 2 - x_h / 2,
        x_w,
        x_h,
    };
    m_saving_subwindow.x_btn.content_point = m_saving_subwindow.x_btn.total_rect.GetTopLeft();

    m_saving_subwindow.load_btn.content_point = {
        topleft.x + Subwindow::BorderThickness + Spacing
            + Subwindow::ButtonWidth / 2 - load_w / 2,
        topleft.y + Subwindow::BorderThickness*2 + title_h + Spacing
            + Subwindow::ButtonHeight / 2 - load_h / 2,
    };
    m_saving_subwindow.load_btn.total_rect = {
        topleft.x + Subwindow::BorderThickness + Spacing,
        topleft.y + Subwindow::BorderThickness*2 + title_h + Spacing,
        Subwindow::ButtonWidth,
        Subwindow::ButtonHeight,
    };

    m_saving_subwindow.save_btn.content_point = {
        topleft.x + Subwindow::BorderThickness + Spacing*2 + Subwindow::ButtonWidth
            + Subwindow::ButtonWidth / 2 - save_w / 2,
        topleft.y + Subwindow::BorderThickness*2 + title_h + Spacing
            + Subwindow::ButtonHeight / 2 - save_h / 2,
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

MainWindowUI::MainWindowUI(SDL2pp::Renderer &renderer, SDL2pp::Font &font, MaterialPalette &palette)
    : m_renderer(renderer)
    , m_font(font)
    , m_palette(palette)
    , m_palette_btn{Button::expanded_from_texture(
        {renderer, "assets/materials_palette_btn_icon"}, 
        {Spacing, Spacing}
    )}
    , m_saving_btn{Button::expanded_from_texture(
        {renderer, "assets/saving_btn_icon"}, 
        {Spacing*2 + m_palette_btn.total_rect.w, Spacing}
    )}
    , m_fps_point(Spacing, Spacing*2 + m_palette_btn.total_rect.h)
    , m_saving_subwindow{
        .title_btn{
            .content_tex{renderer, m_font.RenderText_Shaded(
                "Saving", Subwindow::FgColor, Subwindow::BorderColor
            )},
        },
        .x_btn{
            .content_tex{renderer, m_font.RenderUNICODE_Shaded(
                u"\u00D7", Subwindow::FgColor, Subwindow::BorderColor
            )},
        },
        .load_btn{
            .content_tex{renderer, m_font.RenderText_Blended("Load", Subwindow::FgColor)},
        },
        .save_btn{
            .content_tex{renderer, m_font.RenderText_Blended("Save", Subwindow::FgColor)},
        },
    }
{}

bool MainWindowUI::process_event(const SDL_Event &event)
{
    if(event.type == SDL_MOUSEBUTTONDOWN)
    {
        for(Button *btn : {&m_palette_btn, &m_saving_btn})
        {
            if(btn->total_rect.Contains(event.button.x, event.button.y))
            {
                btn->is_holded = true;
                return true;
            }
        }

        if(!m_is_saving_subwindow_visible)
            return false;
        
        for(Button *btn : m_saving_subwindow.buttons())
        {
            if(btn->total_rect.Contains(event.button.x, event.button.y))
            {
                btn->is_holded = true;
                return true;
            }
        }
        return false;
    }

    if(event.type == SDL_MOUSEBUTTONUP)
    {
        if(m_palette_btn.do_act_on_release(event.button.x, event.button.y))
        {
            sfx::play_sound("palette.show", "ui", true);
            m_palette.show();
        }
        else if(m_saving_btn.do_act_on_release(event.button.x, event.button.y))
        {
            m_is_saving_subwindow_visible = true;
            center_saving_subwindow();
        }
        else if(m_is_saving_subwindow_visible)
        {
            if(m_saving_subwindow.x_btn.do_act_on_release(event.button.x, event.button.y))
            {
                m_is_saving_subwindow_visible = false;
            }
            else if(m_saving_subwindow.load_btn.do_act_on_release(event.button.x, event.button.y))
            {
                sfx::play_sound("material.Sand");
            }
            else if(m_saving_subwindow.save_btn.do_act_on_release(event.button.x, event.button.y))
            {
                sfx::play_sound("material.Water");
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

    if (
        event.type == SDL_MOUSEMOTION 
        && m_is_saving_subwindow_visible 
        && m_saving_subwindow.title_btn.is_holded
    )
        m_saving_subwindow.move({event.motion.xrel, event.motion.yrel});

    return false;
}

void MainWindowUI::render()
{
    int mx{}, my{};
    SDL_GetMouseState(&mx, &my);

    for(auto *btn : {&m_palette_btn, &m_saving_btn})
        btn->render(m_renderer, mx, my);

    if(m_is_saving_subwindow_visible)
        m_saving_subwindow.render(m_renderer, mx, my);

    m_fps_counter.tick();
    SDL2pp::Surface surf = m_font.RenderText_Blended(
        std::format(
            "FPS: Avg {:.2f} Min {:.2f} Max {:.2f}", 
            m_fps_counter.avg_fps(), m_fps_counter.min_fps(), m_fps_counter.max_fps()
        ),
        {0, 255, 0, 255}
    );
    SDL2pp::Texture tex(m_renderer, surf);
    m_renderer.Copy(tex, SDL2pp::NullOpt, m_fps_point);
}
