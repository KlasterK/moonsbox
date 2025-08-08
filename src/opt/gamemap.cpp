#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <functional>

namespace py = pybind11;
using namespace py::literals;

using Point = std::array<int, 2>;

std::array<int, 4> pygame_rect_to_xywh(py::object rect)
{
    return {
        rect.attr("x").cast<int>(),
        rect.attr("y").cast<int>(),
        rect.attr("w").cast<int>(),
        rect.attr("h").cast<int>(),
    };
}

class GameMap {
public:
    GameMap(Point size) 
    {
        auto materials = py::module_::import(".materials");
        m_space_class = materials.attr("Space");
        resize(size);
    }

    py::object get(Point pos) const
    {
        return bounds(pos) ? m_data[_index(pos)] : py::none();
    }

    void set(Point pos, py::object value) 
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
                m_data[_index(x, y)] = material_factory(*this, x, y);
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
                if(left_cmp < right_cmp) material_factory(*this, x, y);
            }
        }
    }

    // TODO: implement more efficient line drawing (this one is just translation from Python)
    void draw_line(Point start, Point end, int width, py::object material_factory, std::string ends) 
    {
        int delta_x = std::abs(start[0], end[0]);
        int delta_y = std::abs(start[1], end[1]);

        int current_x = start[0];
        int current_y = end[0];

        int step_x = start[0] < end[0] ? 1 : -1;
        int step_y = start[1] < end[1] ? 1 : -1;
    
        std::vector<Point> points;

        if (delta_x > delta_y) 
        {
            double error = delta_x / 2.0;
            while (current_x != x1) 
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
            while (current_y != y1) 
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

    py::object dump() 
    {
        py::dict info;
        info["application"] = "moonsbox";
        info["version"] = py::make_tuple("1.0.1-alpha");
        // Convert m_data to numpy array
        py::module np = py::module_::import("numpy");
        py::tuple shape = py::make_tuple(m_size[0], m_size[1]);
        py::object arr = np.attr("empty")(shape, "object");
        for (int x = 0; x < m_size[0]; ++x) {
            for (int y = 0; y < m_size[1]; ++y) {
                arr[x][y] = m_data[_index(x, y)];
            }
        }
        info["array"] = arr;
        py::module pickle = py::module_::import("pickle");
        return pickle.attr("dumps")(info);
    }

    void load(py::object bytes_obj) {
        py::module pickle = py::module_::import("pickle");
        py::object info = pickle.attr("loads")(bytes_obj);
        if (!info.contains("application") || info["application"].cast<std::string>() != "moonsbox") {
            throw std::runtime_error("save is not a moonsbox save");
        }
        if (!info.contains("version") || info["version"].cast<std::string>().find("1.1") == std::string::npos) {
            throw std::runtime_error("save is incompatible with this version");
        }
        if (!info.contains("array")) {
            throw std::runtime_error("save is invalid");
        }
        py::object arr = info["array"];
        py::tuple shape = arr.attr("shape");
        if (shape.size() != 2) {
            throw std::runtime_error("save is invalid");
        }
        int sx = shape[0].cast<int>();
        int sy = shape[1].cast<int>();
        m_size = {sx, sy};
        m_data.resize(sx * sy, py::none());
        for (int x = 0; x < sx; ++x) {
            for (int y = 0; y < sy; ++y) {
                m_data[_index(x, y)] = arr[x][y];
            }
        }
    }

private:
    std::vector<py::object> m_data;
    Point m_size;
    py::object m_space_class;

    int _index(Point pos) const 
    {
        // Using columns-first method
        return pos[1] + pos[0] * m_size[1];
    }
};

PYBIND11_MODULE(opt_gamemap, m) 
{
    py::class_<GameMap>(m, "GameMap")
        .def(py::init<Point>())
        .def("get", &GameMap::get)
        .def("set", &GameMap::set)
        .def("invy", &GameMap::invy)
        .def("invy_pos", &GameMap::invy_pos)
        .def("bounds", &GameMap::bounds)
        .def("resize", &GameMap::resize)
        .def("fill", &GameMap::fill)
        .def("draw_rect", &GameMap::draw_rect)
        .def("draw_ellipse", &GameMap::draw_ellipse)
        .def("draw_line", &GameMap::draw_line)
        .def("dump", &GameMap::dump)
        .def("load", &GameMap::load);
}
