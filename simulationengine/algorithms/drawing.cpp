#include <simulationengine/algorithms/drawing.hpp>
#include <simulationengine/core/gamemap.hpp>
#include <algorithm>
#include <cmath>

void drawing::swap(GameMap &map, size_t ax, size_t ay, size_t bx, size_t by)
{
    auto a_temp = map.temps(ax, ay);
    auto a_heat_capacity = map.heat_capacities(ax, ay);
    auto a_thermal_conductivity = map.thermal_conductivities(ax, ay);
    auto a_color = map.colors(ax, ay);
    auto a_tag = map.tags(ax, ay);
    auto a_physical_behavior = map.physical_behaviors(ax, ay);
    auto a_aux = map.auxs(ax, ay);
    auto a_material_ctl = map.material_ctls(ax, ay);

    map.temps(ax, ay) = map.temps(bx, by);
    map.heat_capacities(ax, ay) = map.heat_capacities(bx, by);
    map.thermal_conductivities(ax, ay) = map.thermal_conductivities(bx, by);
    map.colors(ax, ay) = map.colors(bx, by);
    map.tags(ax, ay) = map.tags(bx, by);
    map.physical_behaviors(ax, ay) = map.physical_behaviors(bx, by);
    map.auxs(ax, ay) = std::move(map.auxs(bx, by));
    map.material_ctls(ax, ay) = map.material_ctls(bx, by);

    map.temps(bx, by) = a_temp;
    map.heat_capacities(bx, by) = a_heat_capacity;
    map.thermal_conductivities(bx, by) = a_thermal_conductivity;
    map.colors(bx, by) = a_color;
    map.tags(bx, by) = a_tag;
    map.physical_behaviors(bx, by) = a_physical_behavior;
    map.auxs(bx, by) = std::move(a_aux);
    map.material_ctls(bx, by) = a_material_ctl;
}

void drawing::copy(GameMap &map, size_t src_x, size_t src_y, size_t dst_x, size_t dst_y)
{
    map.temps(dst_x, dst_y)                     = map.temps(src_x, src_y);
    map.heat_capacities(dst_x, dst_y)           = map.heat_capacities(src_x, src_y);
    map.thermal_conductivities(dst_x, dst_y)    = map.thermal_conductivities(src_x, src_y);
    map.colors(dst_x, dst_y)                    = map.colors(src_x, src_y);
    map.tags(dst_x, dst_y)                      = map.tags(src_x, src_y);
    map.physical_behaviors(dst_x, dst_y)        = map.physical_behaviors(src_x, src_y);
    map.auxs(dst_x, dst_y)                      = map.auxs(src_x, src_y);
    map.material_ctls(dst_x, dst_y)             = map.material_ctls(src_x, src_y);
}

void drawing::fill(GameMap &map, MaterialFactory &material_factory)
{
    for(size_t y{}; y < map.height(); ++y)
    {
        for(size_t x{}; x < map.width(); ++x)
        {
            material_factory(x, y);
        }
    }
}

void drawing::rect(GameMap &map, Rect area, MaterialFactory &material_factory)
{
    for(int y = std::max(0, area[1]); 
        y < std::min(area[1] + area[3], int(map.height())); 
        ++y)
    {
        for(int x = std::max(0, area[0]); 
            x < std::min(area[0] + area[2], int(map.width())); 
            ++x)
        {
            material_factory(x, y);
        }
    }
}

void drawing::ellipse(GameMap &map, Rect area, MaterialFactory &material_factory)
{
    if(area[2] < 3 || area[3] < 3)
        return drawing::rect(map, area, material_factory);

    // Solving ellipse equation:
    // ((x - x0) / a) ** 2 + ((y - y0) / b) ** 2 <= 1
    // This formula with only integral logic:
    // b ** 2 * (x - x0) ** 2 + a ** 2 * (y - y0) ** 2 <= a ** 2 * b ** 2

    long long x0, y0, a_sq, b_sq, right_cmp;
    x0 = area[0] + area[2] / 2;
    y0 = area[1] + area[3] / 2;
    a_sq = area[2] * area[2] / 4;
    b_sq = area[3] * area[3] / 4;
    right_cmp = a_sq * b_sq;

    for(long long y = std::max(0, area[1]); 
        y < std::min(area[1] + area[3], int(map.height())); 
        ++y)
    {
        for(long long x = std::max(0, area[0]); 
            x < std::min(area[0] + area[2], int(map.width())); 
            ++x)
        {
            long long x_x0_sq = (x - x0) * (x - x0);
            long long y_y0_sq = (y - y0) * (y - y0);
            long long left_cmp = b_sq * x_x0_sq + a_sq * y_y0_sq;
            if(left_cmp <= right_cmp)
                material_factory(x, y);
        }
    }
}

void drawing::line(GameMap &map, SignedPoint begin, SignedPoint end, int width, 
                   MaterialFactory &material_factory, LineEnds ends)
{
    if(width == 1)
    {
        int delta_x = std::abs(begin[0] - end[0]);
        int delta_y = std::abs(begin[1] - end[1]);

        int current_x = begin[0];
        int current_y = begin[1];

        int step_x = begin[0] < end[0] ? 1 : -1;
        int step_y = begin[1] < end[1] ? 1 : -1;

        int error{};

        // 4-connected line algorithm
        for(int i{}; i < delta_x + delta_y; ++i)
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

            if(!map.in_bounds(current_x, current_y))
                continue;
            material_factory(current_x, current_y);
        }
    }
    else
    {
        float half_width = static_cast<float>(width) / 2.0f;
        
        // Calculate direction vector
        float dir_x = static_cast<float>(end[0] - begin[0]);
        float dir_y = static_cast<float>(end[1] - begin[1]);
        float length = std::sqrt(dir_x * dir_x + dir_y * dir_y);
        
        if (length < 1.f) 
        {
            if(ends == LineEnds::Round)
                drawing::ellipse(map, {begin[0] - width/2, begin[1] - width/2, width, width}, 
                                 material_factory);
            else if (ends == LineEnds::Square)
                drawing::rect(map, {begin[0] - width/2, begin[1] - width/2, width, width}, 
                              material_factory);
            else if(map.in_bounds(begin[0], begin[1]))
                material_factory(begin[0], begin[1]);
            return;
        }
        
        // Normalise the vector
        dir_x /= length;
        dir_y /= length;
        
        // Perpendicular vector
        float per_x = -dir_y * half_width;
        float per_y = dir_x * half_width;
        
        // Calculating 4 corners of polygon describing this line
        std::array<SignedPoint, 4> corners{
            SignedPoint{
                static_cast<int>(std::round(begin[0] + per_x)),
                static_cast<int>(std::round(begin[1] + per_y))
            },
            SignedPoint{
                static_cast<int>(std::round(begin[0] - per_x)),
                static_cast<int>(std::round(begin[1] - per_y))
            },
            SignedPoint{
                static_cast<int>(std::round(end[0] - per_x)),
                static_cast<int>(std::round(end[1] - per_y))
            },
            SignedPoint{
                static_cast<int>(std::round(end[0] + per_x)),
                static_cast<int>(std::round(end[1] + per_y))
            },
        };

        int minX = std::min({corners[0][0], corners[1][0], corners[2][0], corners[3][0]});
        int maxX = std::max({corners[0][0], corners[1][0], corners[2][0], corners[3][0]});
        int minY = std::min({corners[0][1], corners[1][1], corners[2][1], corners[3][1]});
        int maxY = std::max({corners[0][1], corners[1][1], corners[2][1], corners[3][1]});
        
        // Fill the polygon
        for (int y = minY; y <= maxY; ++y) 
        {
            for (int x = minX; x <= maxX; ++x) 
            {
                // Check if the dot is inside the parallelogramm
                // Using cross product to check from every side
                bool inside = true;
                for (int i = 0; i < 4; ++i) 
                {
                    int j = (i + 1) % 4;
                    float cross = (corners[j][0] - corners[i][0]) * (y - corners[i][1]);
                    cross -= (corners[j][1] - corners[i][1]) * (x - corners[i][0]);
                    if (cross < 0) 
                    {
                        inside = false;
                        break;
                    }
                }

                if (inside && map.in_bounds(x, y))
                    material_factory(x, y);
            }
        }
        
        if(ends == LineEnds::Square) 
        {
            drawing::rect(map, {begin[0] - width/2, begin[1] - width/2, width, width}, 
                          material_factory);
            drawing::rect(map, {end[0] - width/2, end[1] - width/2, width, width}, 
                          material_factory);
        } 
        else if(ends == LineEnds::Round) 
        {
            drawing::ellipse(map, {begin[0] - width/2, begin[1] - width/2, width, width}, 
                             material_factory);
            drawing::ellipse(map, {end[0] - width/2, end[1] - width/2, width, width}, 
                             material_factory);
        }
    }
}