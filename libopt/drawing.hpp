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

template<typename T>
void drawing::ellipse(GameMap &map, Rect area, T material_factory)
{
    // Solving ellipse equation:
    // ((x - x0) / a) ** 2 + ((y - y0) / b) ** 2 <= 1
    // This formula with only integral logic:
    // b ** 2 * (x - x0) ** 2 + a ** 2 * (y - y0) ** 2 <= a ** 2 * b ** 2

    long long x0, y0, a_sq, b_sq, right_cmp, rows = map.height(), cols = map.width();
    x0 = area[0] + area[2] / 2;
    y0 = area[1] + area[3] / 2;
    a_sq = area[2] * area[2] / 4;
    b_sq = area[3] * area[3] / 4;
    right_cmp = a_sq * b_sq;

    for (long long y = 0; y < rows; ++y) 
    {
        for (long long x = 0; x < cols; ++x) 
        {
            long long x_x0_sq = (x - x0) * (x - x0);
            long long y_y0_sq = (y - y0) * (y - y0);
            long long left_cmp = b_sq * x_x0_sq + a_sq * y_y0_sq;
            if(left_cmp < right_cmp) 
                material_factory(map, x, y);
        }
    }
}

// TODO: implement more efficient line drawing (this one is just translation from Python)
template<typename T>
void drawing::line(GameMap &map, SignedPoint begin, SignedPoint end, 
                   int width, T material_factory, LineEnds ends)
{
    int delta_x = std::abs(begin[0] - end[0]);
    int delta_y = std::abs(begin[1] - end[1]);

    int current_x = begin[0];
    int current_y = begin[1];

    int step_x = begin[0] < end[0] ? 1 : -1;
    int step_y = begin[1] < end[1] ? 1 : -1;

    int error{};
    int radius = std::max(0, width / 2);

    // 4-connected line algorithm
    for(size_t i{}; i < delta_x + delta_y; ++i)
    {
        int e1 = error + delta_y;
        int e2 = error - delta_x;
        if(std::abs(e1) < std::abs(e2))
        {
            current_x += step_x;
            error = e1;
        }
        else
        {
            current_y += step_y;
            error = e2;
        }

        for(int radius_delta_x = -radius; radius_delta_x <= radius; ++radius_delta_x)
        {
            for(int radius_delta_y = -radius; radius_delta_y <= radius; ++radius_delta_y)
            {
                int tx = current_x + radius_delta_x;
                int ty = current_y + radius_delta_y;
                if(!map.in_bounds(tx, ty))
                    continue;
                if(ends == LineEnds::Round 
                        && (radius_delta_x * radius_delta_x + radius_delta_y * radius_delta_y) 
                        > radius * radius)
                    continue;
                material_factory(map, tx, ty);
            }
        }
    }

    //     for rad_dx in range(-radius, radius + 1):
    //         for rad_dy in range(-radius, radius + 1):
    //             tx, ty = x + rad_dx, y + rad_dy
    //             if not self.bounds((tx, ty)):
    //                 continue
    //             if ends == 'round':
    //                 # Only fill points inside the circle
    //                 if rad_dx**2 + rad_dy**2 > radius**2:
    //                     continue
    //             self._array[tx, ty] = material_factory(self, tx, ty)
}

#endif // DRAWING_HPP
