#include "materialpalette.hpp"
#include "SDL_blendmode.h"
#include <optional>
#include <stdexcept>
#include <SDL2pp/SDL2pp.hh>

namespace
{
    constexpr size_t ColumnsCount = 5;
    constexpr int SlotWidth = 100;
    constexpr int SlotHeight = 20;

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
    , m_font(font)
{}

MaterialController &MaterialPalette::get_material_at(size_t x, size_t y) const
{
    if(x >= ColumnsCount)
        throw std::logic_error("MaterialPalette::get_material_at: slot out of bounds");

    size_t idx = y * ColumnsCount + x;
    if(idx >= std::distance(m_registry.begin(), m_registry.end()))
        throw std::logic_error("MaterialPalette::get_material_at: slot out of bounds");

    return *std::next(m_registry.begin(), idx)->second;
}

MaterialController &MaterialPalette::get_selected_material() const
{
    size_t idx = m_selection_pos[1] * ColumnsCount + m_selection_pos[0];
    return *std::next(m_registry.begin(), idx)->second;
}

MaterialPalette::TableSize MaterialPalette::whole_grid_size() const
{
    size_t total_items = std::distance(m_registry.begin(), m_registry.end());
    return {.rows=ceildiv(total_items, ColumnsCount), .columns=ColumnsCount};
}

MaterialPalette::TableSize MaterialPalette::visible_grid_size() const
{
    return whole_grid_size();
}

std::array<size_t, 2> MaterialPalette::get_current_selection() const
{
    return m_selection_pos;
}

void MaterialPalette::set_current_selection(size_t x, size_t y)
{
    size_t nitems = std::distance(m_registry.begin(), m_registry.end());
    if(x >= ColumnsCount || x + y * ColumnsCount >= nitems)
        throw std::logic_error("MaterialPalette::set_current_selection: slot out of bounds");

    m_selection_pos = {x, y};
}

std::optional<std::array<size_t, 2>> MaterialPalette::find_material_slot(
    MaterialController &ctl
) const
{
    size_t x{}, y{};
    for(auto it = m_registry.begin(); it != m_registry.end(); ++it)
    {
        if(++x >= ColumnsCount)
        {
            x = 0;
            ++y;
        }

        if(it->second == &ctl)
            return std::array{x, y};
    }
    return std::nullopt;
}

std::optional<std::array<size_t, 2>> MaterialPalette::match_click_to_slot(int x, int y) const
{
    if(x < 0 || y < 0)
        return std::nullopt;

    std::array<size_t, 2> slot{size_t(x) / SlotWidth, size_t(y) / SlotHeight};

    size_t nitems = std::distance(m_registry.begin(), m_registry.end());
    if(slot[0] >= ColumnsCount || slot[0] + slot[1] * ColumnsCount >= nitems)
        return std::nullopt;

    return slot;
}

void MaterialPalette::scroll(int lines_down)
{
    (void)lines_down;
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

void MaterialPalette::render() const
{
    if(!m_is_visible)
        return;

    auto screen_size = m_rnd.GetOutputSize();
    m_rnd.SetDrawColor({0x00, 0x00, 0x00, 0xCC});
    m_rnd.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    m_rnd.FillRect({0, 0}, screen_size);

    int x{}, y{};
    for(auto it = m_registry.begin(); it != m_registry.end(); ++it)
    {
        SDL_Color label_color = {0xFF, 0xFF, 0xFF, 0xFF};
        if(m_selection_pos[0] == size_t(x) && m_selection_pos[1] == size_t(y))
            label_color = {0xFF, 0x00, 0x00, 0xFF};

        auto label_surf = m_font.RenderText_Blended(it->first, label_color);
        SDL2pp::Texture label_tex{m_rnd, label_surf};
        m_rnd.Copy(
            label_tex,
            SDL2pp::NullOpt, 
            SDL2pp::Rect{
                x * SlotWidth, 
                y * SlotHeight, 
                label_surf.GetWidth(), 
                label_surf.GetHeight()
            }
        );

        if(++x >= ColumnsCount)
        {
            x = 0;
            ++y;
        }
    }
}


// #include "materialpalette.hpp"
// #include <algorithm>
// #include <cctype>
// #include <iostream>
// #include <string>
// #include <vector>

// namespace
// {
//     bool starts_with_ignore_case(std::string_view hay, std::string_view needle)
//     {
//         if (needle.size() > hay.size())
//             return false;
//         for (size_t i = 0; i < needle.size(); ++i)
//             if (std::tolower(static_cast<unsigned char>(hay[i])) != std::tolower(static_cast<unsigned char>(needle[i])))
//                 return false;
//         return true;
//     }
// }

// MaterialPaletteStub::MaterialPaletteStub(MaterialRegistry &registry)
//     : m_registry(registry)
// {
//     MaterialController *sand = registry.find_controller_by_name("Sand");
//     if(sand)
//     {
//         m_selected = sand;
//         sync_index_from_selected();
//     }
//     else
//     {
//         m_selected_index = 0;
//         m_selected = m_registry.begin()->second;
//     }
// }

// void MaterialPaletteStub::sync_index_from_selected()
// {
//     for (size_t i = 0; i < m_registry.size(); ++i)
//         if (m_registry[i].second == m_selected)
//         {
//             m_selected_index = static_cast<int>(i);
//             return;
//         }
//     if (!m_registry.empty())
//     {
//         m_selected_index = 0;
//         m_selected = m_registry[0].second;
//     }
// }

// void MaterialPaletteStub::show()
// {
//     m_visible = true;
//     prompt_material_stdin();
// }

// void MaterialPaletteStub::hide(bool confirmation)
// {
//     m_visible = false;
//     if (confirmation && m_selected_index >= 0 && m_selected_index < static_cast<int>(m_registry.size()))
//         m_selected = m_registry[m_selected_index].second;

//         m_registry.end() - m_registry.begin();
// }

// void MaterialPaletteStub::go_to_starting_with(std::string_view text)
// {
//     for (size_t i = 0; i < m_registry.size(); ++i)
//         if (starts_with_ignore_case(m_registry[i].first, text))
//         {
//             m_selected_index = static_cast<int>(i);
//             m_selected = m_registry[i].second;
//             return;
//         }
// }

// std::array<int, 2> MaterialPaletteStub::grid_size() const
// {
//     int n = static_cast<int>(m_registry.size());
//     int cols = std::max(1, n);
//     return { cols, 1 };
// }

// std::array<int, 2> MaterialPaletteStub::whole_grid_size() const
// {
//     return grid_size();
// }

// std::array<int, 2> MaterialPaletteStub::selection_slot() const
// {
//     int cols = std::max(1, static_cast<int>(m_registry.size()));
//     return { m_selected_index % cols, m_selected_index / cols };
// }

// void MaterialPaletteStub::set_selection_slot(int col, int row)
// {
//     int cols = std::max(1, static_cast<int>(m_registry.size()));
//     int idx = row * cols + col;
//     if (idx >= 0 && idx < static_cast<int>(m_registry.size()))
//     {
//         m_selected_index = idx;
//         m_selected = m_registry[idx].second;
//     }
// }

// MaterialController *MaterialPaletteStub::get_material_at(int col, int row) const
// {
//     int cols = std::max(1, static_cast<int>(m_registry.size()));
//     int idx = row * cols + col;
//     if (idx < 0 || idx >= static_cast<int>(m_registry.size()))
//         return nullptr;
//     return m_registry[idx].second;
// }

// bool MaterialPaletteStub::set_selected_by_name(std::string_view name)
// {
//     MaterialController *ctl = m_registry->find_controller_by_name(name);
//     if (ctl)
//     {
//         m_selected = ctl;
//         sync_index_from_selected();
//         return true;
//     }
//     for (size_t i = 0; i < m_registry.size(); ++i)
//         if (starts_with_ignore_case(m_registry[i].first, name))
//         {
//             m_selected = m_registry[i].second;
//             m_selected_index = static_cast<int>(i);
//             return true;
//         }
//     return false;
// }

// void MaterialPaletteStub::prompt_material_stdin()
// {
//     std::cout << "Enter material name: " << std::flush;
//     std::string line;
//     if (std::getline(std::cin, line))
//     {
//         while (!line.empty() && (line.back() == ' ' || line.back() == '\t'))
//             line.pop_back();
//         if (!line.empty())
//         {
//             if (set_selected_by_name(line))
//             {
//                 if (m_selected_index >= 0 && m_selected_index < static_cast<int>(m_registry.size()))
//                     std::cout << "Selected: " << m_registry[m_selected_index].first << "\n";
//                 else
//                     std::cout << "Selected.\n";
//             }
//             else
//                 std::cerr << "Unknown material: " << line << "\n";
//         }
//     }
// }
