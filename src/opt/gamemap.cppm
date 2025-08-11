module;
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
export module gamemap;

namespace py = pybind11;
using namespace py::literals;

import util;

class GameMap 
{
public:
    GameMap(Point size) 
    {
        auto materials = py::module_::import("src.materials");
        m_space_class = materials.attr("Space");
        m_base_material_class = materials.attr("BaseMaterial");
        resize(size);
    }

    Point get_size()
    {
        return m_size;
    }

    py::object get_at(Point pos) const
    {
        return bounds(pos) ? m_data[_index(pos)] : py::none();
    }

    void get_view() const
    {
        throw std::runtime_error("get_view() not implemented in opt_gamemap");
    }

    void set_at(Point pos, py::object value) 
    {
        if (bounds(pos)) 
            m_data[_index(pos)] = value;
    }

    int invy(int y) const 
    {
        return m_size[1] - 1 - y;
    }

    Point invy_pos(Point pos) const 
    {
        return {pos[0], m_size[1] - 1 - pos[1]};
    }

    bool bounds(Point pos) const 
    {
        return pos[0] >= 0 && pos[0] < m_size[0] && pos[1] >= 0 && pos[1] < m_size[1];
    }

    void resize(Point new_size) 
    {
        m_size = new_size;
        m_data.resize(m_size[0] * m_size[1], py::none());
        fill(m_space_class);
    }

    void fill(py::object material_factory) 
    {
        for (int y = 0; y < m_size[1]; ++y)
            for (int x = 0; x < m_size[0]; ++x)
                m_data[_index({x, y})] = material_factory(*this, x, y);
    }

    void draw_rect(py::object area, py::object material_factory) 
    {
        auto [rx, ry, rw, rh] = pygame_rect_to_xywh(area);

        int x_start = std::max(rx, 0);
        int x_end = std::min(ry, m_size[0]);
        int y_start = std::max(rx+rw, 0);
        int y_end = std::min(ry+rh, m_size[1]);

        for (int y = y_start; y < y_end; ++y)
            for (int x = x_start; x < x_end; ++x)
                m_data[_index({x, y})] = material_factory(*this, x, y);
    }

    void draw_ellipse(py::object area, py::object material_factory) 
    {
        // Solving ellipse equation:
        // ((x - x0) / a) ** 2 + ((y - y0) / b) ** 2 <= 1
        // This formula with only integral logic:
        // b ** 2 * (x - x0) ** 2 + a ** 2 * (y - y0) ** 2 <= a ** 2 * b ** 2
        // Does this formula need long long? I calculated sqrt of MAX_INT and it's ~44k

        long long x0, y0, a_sq, b_sq, right_cmp, rows = m_size[0], cols = m_size[1];
        {
            auto [x, y, w, h] = pygame_rect_to_xywh(area);
            x0 = x + w / 2;
            y0 = x + h / 2;
            a_sq = w * w / 4;
            b_sq = h * h / 4;
            right_cmp = a_sq * b_sq;
        }

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
    void draw_line(Point start, Point end, int width, py::object material_factory, std::string ends) 
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

    py::object dump(py::object file) 
    {
        // NOTE: internal format is columns-first, but external is rows-first
        py::list lists;
        for(int x{}; x < m_size[0]; ++x)
        {
            py::list sublist;
            for(int y{}; y < m_size[1]; ++y)
            {
                sublist.append(m_data[_index({x, y})]);
            }
            lists.append(sublist);
        }

        py::dict info;
        info["application"] = "moonsbox";
        info["version"] = py::make_tuple("1.0.1-alpha");
        info["lists"] = lists;
        py::module pickle = py::module_::import("pickle");
        return pickle.attr("dump")(info, file);
    }

    void load(py::object file)  
    {
        // TODO: add more checks
        py::dict info;
        try 
        {
            py::module pickle = py::module_::import("pickle");
            info = pickle.attr("load")(file);
        }
        catch(const py::builtin_exception &e)
        {
            throw py::value_error("failed to unpickle save");
        }

        if (!info.contains("application") 
            || info["application"].cast<std::string>() != "moonsbox"
        ) throw py::value_error("save is not a moonsbox save (wrong application)");

        if (!info.contains("version") 
            || !info["version"].cast<py::sequence>().contains("1.0.1-alpha")
        ) throw py::value_error("save is incompatible with this version");

        if (!info.contains("lists"))
            throw py::value_error("save is invalid (no lists key)");

        py::sequence lists = info["lists"];
        int shape_y = lists.size();
        if(shape_y <= 0)
            throw py::value_error("save is invalid (empty map)");
        int shape_x = lists[0].cast<py::sequence>().size();
        if(shape_x <= 0)
            throw py::value_error("save is invalid (empty map)");

        // Firsly, check everything
        for(auto sublist_uncasted : lists)
        {
            auto subseq = sublist_uncasted.cast<py::sequence>();
            if(subseq.size() != shape_x)
                throw py::value_error("save is invalid (non-straight shape)");
        }

        resize({shape_x, shape_y});
        for(int x{}; x < shape_x; ++x)
        {
            auto subseq = lists[x].cast<py::sequence>();
            for(int y{}; y < shape_y; ++y)
            {
                m_data[_index({x, y})] = subseq[y];
            }
        }
    }

private:
    std::vector<py::object> m_data;
    Point m_size;
    py::object m_space_class, m_base_material_class;

    int _index(Point pos) const 
    {
        // Using columns-first method
        return pos[1] + pos[0] * m_size[1];
    }
};

PYBIND11_MODULE(opt_gamemap, m) 
{
}
