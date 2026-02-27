#ifndef MOOX_MATERIALPALETTE_HPP
#define MOOX_MATERIALPALETTE_HPP

#include <SDL2pp/SDL2pp.hh>
#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include "materialregistry.hpp"
#include <array>

class MaterialPalette
{
public:
    MaterialPalette(SDL2pp::Renderer &rnd, MaterialRegistry &registry, SDL2pp::Font &font);

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
    SDL2pp::Renderer &m_rnd;
    std::vector<SDL2pp::Texture> m_slot_texs;

    size_t m_selection_idx{0};
    bool m_is_visible{false};
};

#endif // MOOX_MATERIALPALETTE_HPP
