#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "gamemap.hpp"
#include "drawing.hpp"
#include "simulation.hpp"
#include "materials.hpp"
#include <optional>
#include "renderer.hpp"

#define RAISE(x, y) (PyErr_SetString((x), (y)), nullptr)
extern "C"
{
#include "include/pygame-ce/include/pygame.h"
}

namespace py = pybind11;

struct DotProxy
{
    GameMap &map;
    size_t x, y;
};

void _assign_dot(GameMap &map, DotProxy proxy, int x, int y)
{
    map.temps(x, y) = proxy.map.temps(proxy.x, proxy.y);
    map.heat_capacities(x, y) = proxy.map.heat_capacities(proxy.x, proxy.y);
    map.thermal_conductivities(x, y) = proxy.map.thermal_conductivities(proxy.x, proxy.y);
    map.colors(x, y) = proxy.map.colors(proxy.x, proxy.y);
    map.tags(x, y) = proxy.map.tags(proxy.x, proxy.y);
    map.physical_behaviors(x, y) = proxy.map.physical_behaviors(proxy.x, proxy.y);
    map.auxs(x, y) = proxy.map.auxs(proxy.x, proxy.y);
    map.material_ids(x, y) = proxy.map.material_ids(proxy.x, proxy.y);
}

Space g_space;
Sand  g_sand;

SimulationManager _make_simulation_manager(GameMap &map)
{
    SimulationManager sim(map);
    sim.register_controller(g_space, "Space");
    sim.register_controller(g_sand, "Sand");
    return sim;
}

PYBIND11_MODULE(libopt, m)
{
    import_pygame_surface();

    py::class_<DotProxy>(m, "DotProxy")
        .def_property("temp",
                      [](DotProxy self) 
                      { return self.map.temps(self.x, self.y); },
                      [](DotProxy self, float value) 
                      { self.map.temps(self.x, self.y) = value; })
        .def_property("heat_capacity",
                      [](DotProxy self) 
                      { return self.map.heat_capacities(self.x, self.y); },
                      [](DotProxy self, float value) 
                      { self.map.heat_capacities(self.x, self.y) = value; })
        .def_property("thermal_conductivity",
                      [](DotProxy self) 
                      { return self.map.thermal_conductivities(self.x, self.y); },
                      [](DotProxy self, float value) 
                      { self.map.thermal_conductivities(self.x, self.y) = value; })
        .def_property("color",
                      [](DotProxy self) 
                      { static auto pygame_color = py::module_::import("pygame").attr("Color");
                        return pygame_color(self.map.colors(self.x, self.y)); },
                      [](DotProxy self, py::object value) 
                      { self.map.colors(self.x, self.y) = py::int_(value); })
        .def_property("tags",
                      [](DotProxy self) 
                      { return self.map.tags(self.x, self.y).to_ullong(); },
                      [](DotProxy self, unsigned long long value) 
                      { self.map.tags(self.x, self.y) = value; })
        .def_property("physical_entity",
                      [](DotProxy self) 
                      { return static_cast<unsigned long long>(
                        self.map.physical_behaviors(self.x, self.y)); },
                      [](DotProxy self, unsigned long long value) 
                      { self.map.physical_behaviors(self.x, self.y) 
                        = static_cast<MaterialPhysicalBehavior>(value); })
    ;

    py::class_<GameMap>(m, "GameMap")
        .def(py::init([](std::array<size_t, 2> size) 
        { 
            GameMap map(size[0], size[1]);
            drawing::fill(map, [&](GameMap &map, size_t x, size_t y)
                               { g_space.init_point(map, x, y); });
            return map;
        }))
        .def_property_readonly("size", [](GameMap &map) 
        {
            return py::make_tuple(map.width(), map.height());
        })
        .def("__getitem__", [](GameMap &map, std::array<int, 2> pos) -> py::object
        {
            if(!map.in_bounds(pos[0], pos[1]))
                return py::none();
            return py::cast(DotProxy{map, (size_t)pos[0], (size_t)pos[1]});
        })
        .def("__setitem__", [](GameMap &map, std::array<int, 2> pos, DotProxy proxy)
        {
            if(!map.in_bounds(pos[0], pos[1]))
                return;

            _assign_dot(map, proxy, pos[0], pos[1]);
        })
        .def("invy", [](GameMap &map, int y) { return map.height() - 1 - y; })
        .def("invy_pos", [](GameMap &map, std::array<int, 2> pos)
        {
            return std::array{pos[0], int(map.height() - 1 - pos[1])};
        })
        .def("bounds", [](GameMap &map, std::array<int, 2> pos) 
        { 
            return map.in_bounds(pos[0], pos[1]); 
        })
        .def("fill", [](GameMap &map, py::function material_factory) 
        {
            drawing::fill(map, [&](GameMap &, size_t x, size_t y)
            {
                auto obj = material_factory(map, x, y);
                _assign_dot(map, py::cast<DotProxy>(obj), x, y);
            });
        })
        .def("draw_rect", [](GameMap &map, drawing::Rect area, py::function material_factory) 
        {
            drawing::rect(map, area, [&](GameMap &, size_t x, size_t y)
            {
                auto obj = material_factory(map, x, y);
                _assign_dot(map, py::cast<DotProxy>(obj), x, y);
            });
        })
        .def("draw_ellipse", [](GameMap &map, drawing::Rect area, py::function material_factory) 
        {
            drawing::ellipse(map, area, [&](GameMap &, size_t x, size_t y)
            {
                auto obj = material_factory(map, x, y);
                _assign_dot(map, py::cast<DotProxy>(obj), x, y);
            });
        })
        .def("draw_line", [](GameMap &map, std::array<int, 2> begin, std::array<int, 2> end, 
                             int width, py::function material_factory, std::string_view ends) 
        {
            drawing::LineEnds ends_value;
            if(ends == "square")
                ends_value = drawing::LineEnds::Square;
            else if(ends == "round")
                ends_value = drawing::LineEnds::Round;
            else // ends == "none" or anything else
                ends_value = drawing::LineEnds::None;

            drawing::line(map, begin, end, width, [&](GameMap &, size_t x, size_t y)
            {
                auto obj = material_factory(map, x, y);
                _assign_dot(map, py::cast<DotProxy>(obj), x, y);
            }, ends_value);
        })
        .def("dump", [](GameMap &, py::object) {})
        .def("load", [](GameMap &, py::object) {})
    ;

    py::class_<SimulationManager>(m, "SimulationManager")
        .def(py::init([](GameMap &map) { return _make_simulation_manager(map); }))
        .def("tick", [](SimulationManager &sim) { sim.tick(); })
        .def("tick", [](SimulationManager &sim, py::object) { sim.tick(); })
        .def("get_tps", []{ return 0.0; })
    ;

    py::class_<MaterialController>(m, "MaterialController")
        .def("__call__", [](MaterialController &ctl, GameMap &map, size_t x, size_t y)
        { 
            ctl.init_point(map, x, y);
            return DotProxy(map, x, y);
        })
    ;

    m.def("ls_materials", []
    {
        py::dict d;
        d["Space"] = py::cast(static_cast<MaterialController *>(&g_space), 
                              py::return_value_policy::reference);
        d["Sand"]  = py::cast(static_cast<MaterialController *>(&g_sand), 
                              py::return_value_policy::reference);
        return d;
    });

    py::class_<Renderer>(m, "Renderer")
        .def(py::init([](GameMap &map, py::object dst_surf, py::object bg_color)
        {
            return Renderer(map, dst_surf, py::int_(bg_color));
        }))
        .def("render", [](Renderer &rnd, std::array<int, 4> area) { rnd.render(area); })
        .def("render", [](Renderer &rnd, std::array<int, 4> area, float) { rnd.render(area); })
        .def("get_fps", [](Renderer &) { return 0.0; })
        .def("begin_capturing", [](Renderer &) {})
        .def("end_capturing", [](Renderer &) {})
        .def("is_capturing", [](Renderer &) { return false; })
        .def("take_screenshot", [](Renderer &) {})
        .def("next_render_mask", [](Renderer &) {})
    ;
}
