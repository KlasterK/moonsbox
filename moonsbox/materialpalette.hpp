#ifndef MOOX_MATERIALPALETTE_HPP
#define MOOX_MATERIALPALETTE_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

class MaterialRegistry;
class MaterialController;

class MaterialPalette
{
public:
    MaterialPalette(SDL_Renderer *nonown_sdl_renderer, MaterialRegistry &registry, TTF_Font *nonown_font);

    MaterialController &get_selected_material() const;
    size_t get_current_selection_idx() const;
    void set_current_selection_idx(size_t idx);
    bool move_selection(int columns_right, int rows_down);

    std::optional<size_t> find_material_idx(MaterialController &ctl) const;
    std::optional<size_t> match_click_to_idx(int x, int y) const;
    void scroll(int rows_down);

    void show();
    void hide();
    bool is_visible() const;
    void render();

private:
    MaterialRegistry &m_registry;
    SDL_Renderer *m_rnd;
    std::vector<std::unique_ptr<SDL_Texture, void (&)(SDL_Texture *)>> m_slot_texs;

    size_t m_selection_idx{0};
    bool m_is_visible{false};
};

#endif // MOOX_MATERIALPALETTE_HPP
