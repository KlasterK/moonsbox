#ifndef MOOX_MAINWINDOWUI_HPP
#define MOOX_MAINWINDOWUI_HPP

#include "fpscounter.hpp"
#include "windowevents.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <filesystem>
#include <array>
#include <simulationengine/core/compat.hpp>

class MaterialPalette;

class MainWindowUI
{
public:
    using FileCallback = MoveOnlyOrOldFunction<void(std::filesystem::path)>;

public:
    MainWindowUI(
        SDL_Renderer *nonown_renderer,
        TTF_Font *nonown_font,
        MaterialPalette &palette,
        FileCallback load_save_cb,
        FileCallback store_save_cb
    );
    bool process_event(const SDL_Event &e);
    void render();

private:
    struct Button
    {
        std::unique_ptr<SDL_Texture, void (&)(SDL_Texture *)> content_tex;
        SDL_FRect content_rect{};
        SDL_FRect total_rect{};
        bool is_holded{false};

        static Button default_from_surface(
            SDL_Renderer *nonown_renderer,
            SDL_Surface *takeown_surf,
            std::string error_ctx_if_nullptr
        );
        static Button expanded_from_surface(
            SDL_Renderer *nonown_renderer,
            SDL_Surface *takeown_surf,
            const SDL_FPoint &topleft,
            std::string error_ctx_if_nullptr
        );
        void render(SDL_Renderer *nonown_renderer, std::optional<SDL_FPoint> mouse_point);
        void move(const SDL_FPoint &rel);
        bool do_act_on_release(const SDL_FPoint &mouse_point) const;
    };

    struct SavingSubwindow
    {
        Button title_btn, x_btn, load_btn, save_btn;
        SDL_FRect inner_rect{};
        SDL_FRect total_rect{};

        void render(SDL_Renderer *nonown_renderer, const SDL_FPoint &mouse_point);
        void move(const SDL_FPoint &rel);
        std::array<Button *, 4> buttons();
    };

    void center_saving_subwindow();

    SDL_Renderer *m_renderer;
    TTF_Font *m_font;
    MaterialPalette &m_palette;

    Button m_palette_btn, m_saving_btn;
    SavingSubwindow m_saving_subwindow;

    FileCallback m_load_save_cb, m_store_save_cb;

    FPSCounter m_fps_counter{};
    bool m_is_saving_subwindow_visible{false};
};
static_assert(EventHandler<MainWindowUI>);

#endif // MOOX_MAINWINDOWUI_HPP
