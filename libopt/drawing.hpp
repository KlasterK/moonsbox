#ifndef DRAWING_HPP
#define DRAWING_HPP

#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include <algorithm>

namespace drawing
{
    using SignedPoint = std::array<int, 2>;
    using Rect = std::array<int, 4>;

    enum class LineEnds {Square, Round};

    void swap(GameMap &map, size_t ax, size_t ay, size_t bx, size_t by);

    template<typename T>
    void fill(GameMap &map, T material_factory);

    template<typename T>
    void rect(GameMap &map, Rect area, T material_factory);

    template<typename T>
    void ellipse(GameMap &map, Rect area, T material_factory);

    template<typename T>
    void line(GameMap &map, SignedPoint begin, SignedPoint end, 
              int width, T material_factory, LineEnds ends);
}

// Implementation

inline void drawing::swap(GameMap &map, size_t ax, size_t ay, size_t bx, size_t by)
{
    auto a_temp = map.temps(ax, ay);
    auto a_heat_capacity = map.heat_capacities(ax, ay);
    auto a_thermal_conductivity = map.thermal_conductivities(ax, ay);
    auto a_color = map.colors(ax, ay);
    auto a_tag = map.tags(ax, ay);
    auto a_physical_behavior = map.physical_behaviors(ax, ay);
    auto a_aux = map.auxs(ax, ay);
    auto a_material_id = map.material_ids(ax, ay);

    map.temps(ax, ay) = map.temps(bx, by);
    map.heat_capacities(ax, ay) = map.heat_capacities(bx, by);
    map.thermal_conductivities(ax, ay) = map.thermal_conductivities(bx, by);
    map.colors(ax, ay) = map.colors(bx, by);
    map.tags(ax, ay) = map.tags(bx, by);
    map.physical_behaviors(ax, ay) = map.physical_behaviors(bx, by);
    map.auxs(ax, ay) = map.auxs(bx, by);
    map.material_ids(ax, ay) = map.material_ids(bx, by);

    map.temps(bx, by) = a_temp;
    map.heat_capacities(bx, by) = a_heat_capacity;
    map.thermal_conductivities(bx, by) = a_thermal_conductivity;
    map.colors(bx, by) = a_color;
    map.tags(bx, by) = a_tag;
    map.physical_behaviors(bx, by) = a_physical_behavior;
    map.auxs(bx, by) = a_aux;
    map.material_ids(bx, by) = a_material_id;
}

template<typename T>
void drawing::fill(GameMap &map, T material_factory)
{
    for(size_t y{}; y < map.height(); ++y)
    {
        for(size_t x{}; x < map.width(); ++x)
        {
            material_factory(map, x, y);
        }
    }
}

template<typename T>
void drawing::rect(GameMap &map, Rect area, T material_factory)
{
    for(size_t y = area[1]; y < std::min(map.height(), (size_t)area[1] + area[3]); ++y)
    {
        for(size_t x = area[0]; x < std::min(map.width(), (size_t)area[0] + area[2]); ++x)
        {
            material_factory(map, x, y);
        }
    }
}

#endif // DRAWING_HPP
