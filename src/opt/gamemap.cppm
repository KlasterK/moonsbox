module;
#include <functional>
export module gamemap;

import util;

class GameMap;

export enum class MaterialTags : long long
{
    Null        = 0,
    Solid       = 1,
    Bulk        = 2,
    Liquid      = 4,
    Gas         = 8,
    Space       = 16,

    Sparseness  = Gas | Space,
    Movable     = Bulk | Liquid | Gas,
};

export struct MaterialData 
{
    float temp, heat_capacity, thermal_conductivity;
    uint32_t color_rgba;
    std::function<void(GameMap&, Point)> update;
    uintptr_t aux;
    MaterialTags tags;

    // Default constructor, represents Space
    MaterialData()
        : temp(20.0_cels)
        , heat_capacity(0.3f)
        , thermal_conductivity(1.0f)
        , color_rgba(0000)
        , update(nullptr)
        , tags(MaterialTags::Space)
    {}
};

using material_factory_t = std::function<MaterialData(GameMap&, Point)>;
enum class LineEnds { Square, Round };

export class GameMap
{
public:
    GameMap(Point size, material_factory_t filler)
    {
        resize(size);
    }

    Point get_size() noexcept
    {
        return m_size;
    }

    opt_ref<const MaterialData> at(Point pos) const noexcept
    {
        return bounds(pos) ? m_data[_index(pos)] : std::nullopt;
    }

    opt_ref<MaterialData> at(Point pos) noexcept
    {
        return bounds(pos) ? m_data[_index(pos)] : std::nullopt;
    }

    decltype(m_data)::iterator begin() { return m_data.begin(); }
    decltype(m_data)::iterator end()   { return m_data.end(); }
    Point strides() { return m_size[1], 1; }

    int invy(int y) const noexcept
    {
        return m_size[1] - 1 - y;
    }

    Point invy_pos(Point pos) const noexcept
    {
        return {pos[0], m_size[1] - 1 - pos[1]};
    }

    bool bounds(Point pos) const noexcept
    {
        return pos[0] >= 0 && pos[0] < m_size[0] && pos[1] >= 0 && pos[1] < m_size[1];
    }

    void resize(Point new_size)
    {
        m_data = std::make_unique<MaterialData[]>(m_size[0] * m_size[1], py::none());
        m_size = new_size;
        fill(m_space_class);
    }

    void fill(material_factory_t material_factory) 
    {
        for (int y = 0; y < m_size[1]; ++y)
            for (int x = 0; x < m_size[0]; ++x)
                m_data[_index({x, y})] = material_factory(*this, x, y);
    }

    void draw_rect(Rect area, material_factory_t material_factory) 
    {
        int x_start = std::max(area.x0(), 0);
        int x_end   = std::min(area.x1(), m_size[0]);
        int y_start = std::max(area.y0(), 0);
        int y_end   = std::min(area.y1(), m_size[1]);

        for (int y = y_start; y < y_end; ++y)
            for (int x = x_start; x < x_end; ++x)
                m_data[_index({x, y})] = material_factory(*this, x, y);
    }

    void draw_ellipse(Rect area, material_factory_t material_factory) 
    {
        // Solving ellipse equation:
        // ((x - x0) / a) ** 2 + ((y - y0) / b) ** 2 <= 1
        // This formula with only integral logic:
        // b ** 2 * (x - x0) ** 2 + a ** 2 * (y - y0) ** 2 <= a ** 2 * b ** 2

        long long x0 = area.center_x();
        long long y0 = area.center_y();
        long long a_sq = area.w() * area.w() / 4;
        long long b_sq = area.h() * area.h() / 4;
        long long right_cmp = a_sq * b_sq;
        long long rows = m_size[0], cols = m_size[1];

        for (long long y = 0; y < rows; ++y) 
        {
            for (long long x = 0; x < cols; ++x) 
            {
                long long x_x0_sq = (x - x0) * (x - x0);
                long long y_y0_sq = (y - y0) * (y - y0);
                long long left_cmp = b_sq * x_x0_sq + a_sq * y_y0_sq;
                if(left_cmp <= right_cmp) material_factory(*this, x, y);
            }
        }
    }

    // TODO: implement more efficient line drawing (this one is just translation from Python)
    void draw_line(Point start, Point end, int width, material_factory_t material_factory, 
                   LineEnds ends) 
    {
        int delta_x = std::abs(start[0] - end[0]);
        int delta_y = std::abs(start[1] - end[1]);

        int current_x = start[0];
        int current_y = end[0];

        int step_x = start[0] < end[0] ? 1 : -1;
        int step_y = start[1] < end[1] ? 1 : -1;
    
        std::vector<Point> points;

        if (delta_x > delta_y) 
        {
            double error = delta_x / 2.0;
            while (current_x != end[0]) 
            {
                points.push_back({current_x, current_y});
                current_x += step_x;
                error -= delta_y;
                if (error < 0) 
                {
                    current_y += step_y;
                    error += delta_x;
                }
            }
            points.push_back({current_x, current_y});
        } 
        else 
        {
            double error = delta_y / 2.0;
            while (current_y != end[1]) 
            {
                points.push_back({current_x, current_y});
                current_y += step_y;
                error -= delta_x;
                if (error < 0) 
                {
                    current_x += step_x;
                    error += delta_y;
                }
            }
            points.push_back({current_x, current_y});
        }

        int radius = std::max(0, width / 2);

        for (const Point& p : points) 
        {
            int px = p[0];
            int py = p[1];

            for (int dy = -radius; dy <= radius; ++dy) 
            {
                for (int dx = -radius; dx <= radius; ++dx) 
                {
                    int tx = px + dx;
                    int ty = py + dy;
                    if (!bounds({tx, ty})) continue;
                    if (ends == "round" && (dx * dx + dy * dy) > (radius * radius))
                        continue;
                    m_data[_index({tx, ty})] = material_factory(*this, tx, ty);
                }
            }
        }
    }

public:
    void dump(std::ostream file)
    {}

    void load(std::istream file)
    {}
    
private:
    std::vector<MaterialData> m_data;
    Point m_size;

    int _index(Point pos) const 
    {
        // Using columns-first method
        return pos[1] + pos[0] * m_size[1];
    }
};
