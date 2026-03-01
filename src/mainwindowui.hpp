#ifndef MOOX_MAINWINDOWUI_HPP
#define MOOX_MAINWINDOWUI_HPP

#include "fpscounter.hpp"
#include "windowevents.hpp"
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Font.hh>
#include <SDL2pp/Texture.hh>
#include <SDL2pp/Point.hh>
#include <SDL2pp/Rect.hh>
#include <SDL2/SDL_events.h>
#include <filesystem>
#include <functional>
#include <array>

class MaterialPalette;

class MainWindowUI
{
public:
    using FileCallback = std::move_only_function<void(std::filesystem::path)>;

public:
    MainWindowUI(
        SDL2pp::Renderer &renderer, SDL2pp::Font &font, MaterialPalette &palette,
        FileCallback load_save_cb, FileCallback store_save_cb
    );
    bool process_event(const SDL_Event &e);
    void render();

private:
    struct Button 
    { 
        SDL2pp::Texture content_tex;
        SDL2pp::Point content_point{-1, -1};
        SDL2pp::Rect total_rect{-1, -1, 0, 0};
        bool is_holded{false};

        static Button expanded_from_texture(SDL2pp::Texture &&tex, SDL2pp::Point topleft);
        void render(SDL2pp::Renderer &renderer, int mx, int my);
        void move(SDL2pp::Point rel);
        bool do_act_on_release(int mx, int my) const;
    };

    struct SavingSubwindow
    {
        Button title_btn, x_btn, load_btn, save_btn;
        SDL2pp::Rect inner_rect{-1, -1, 0, 0}, total_rect{-1, -1, 0, 0};

        void render(SDL2pp::Renderer &renderer, int mx, int my);
        void move(SDL2pp::Point rel);
        std::array<Button *, 4> buttons();
    };

    void center_saving_subwindow();

    SDL2pp::Renderer &m_renderer;
    SDL2pp::Font &m_font;
    MaterialPalette &m_palette;

    Button m_palette_btn, m_saving_btn;
    SDL2pp::Point m_fps_point;
    SavingSubwindow m_saving_subwindow;

    FileCallback m_load_save_cb, m_store_save_cb;

    FPSCounter m_fps_counter{};
    bool m_is_saving_subwindow_visible{false};
};
static_assert(EventHandler<MainWindowUI>);

#endif // MOOX_MAINWINDOWUI_HPP
