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
    struct TableSize { size_t rows{}, columns{}; };

public:
    MaterialPalette(SDL2pp::Renderer &rnd, MaterialRegistry &registry, SDL2pp::Font &font);

    MaterialController &get_material_at(size_t x, size_t y) const;
    MaterialController &get_selected_material() const;

    TableSize whole_grid_size() const;
    TableSize visible_grid_size() const;

    std::array<size_t, 2> get_current_selection() const;
    void set_current_selection(size_t x, size_t y);

    std::optional<std::array<size_t, 2>> find_material_slot(MaterialController &ctl) const;
    std::optional<std::array<size_t, 2>> match_click_to_slot(int x, int y) const;
    void scroll(int lines_down);

    void show();
    void hide();
    bool is_visible() const;
    void render() const;

private:
    MaterialRegistry &m_registry;
    SDL2pp::Renderer &m_rnd;
    SDL2pp::Font &m_font;
    std::array<size_t, 2> m_selection_pos{0, 0};
    bool m_is_visible{false};
};

#endif // MOOX_MATERIALPALETTE_HPP
