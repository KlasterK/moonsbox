#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "gamemap.hpp"
#include "drawing.hpp"
#include "simulation.hpp"
#include "materials.hpp"
#include <optional>
#include <map>
#include <tuple>
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

std::tuple<
    Space, Sand, Plus100K, Minus100K, Water, Ice, Steam,
    Tap
> g_materials_tuple{};

auto g_materials_map = std::to_array<std::pair<std::string, MaterialController &>>({
    {"Space",   std::get<Space>(g_materials_tuple)},
    {"Sand",    std::get<Sand>(g_materials_tuple)},
    {"+100 K",  std::get<Plus100K>(g_materials_tuple)},
    {"-100 K",  std::get<Minus100K>(g_materials_tuple)},
    {"Water",   std::get<Water>(g_materials_tuple)},
    {"Ice",     std::get<Ice>(g_materials_tuple)},
    {"Steam",   std::get<Steam>(g_materials_tuple)},
    {"Tap",     std::get<Tap>(g_materials_tuple)},
});

std::unique_ptr<SimulationManager> _make_simulation_manager(GameMap &map)
{
    auto sim = std::make_unique<SimulationManager>(map);
    for(auto &[name, ctl] : g_materials_map)
        sim->register_controller(ctl, name);
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
                               { std::get<Space>(g_materials_tuple).init_point(map, x, y); });
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
            drawing::fill(map, material_factory);
        })
        .def("draw_rect", [](GameMap &map, drawing::Rect area, py::function material_factory) 
        {
            drawing::rect(map, area, material_factory);
        })
        .def("draw_ellipse", [](GameMap &map, drawing::Rect area, py::function material_factory) 
        {
            drawing::ellipse(map, area, material_factory);
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

            drawing::line(map, begin, end, width, material_factory, ends_value);
        })
        .def("dump", [](GameMap &, py::object) {})
        .def("load", [](GameMap &, py::object) {})
    ;

    py::class_<SimulationManager, std::unique_ptr<SimulationManager>>(m, "SimulationManager")
        .def(py::init([](GameMap &map) { return _make_simulation_manager(map); }))
        .def("tick", &SimulationManager::tick)
        .def("is_paused", &SimulationManager::is_paused)
        .def("set_paused", &SimulationManager::set_paused)
    ;

    py::class_<MaterialController>(m, "MaterialController")
        .def("__call__", [](MaterialController &ctl, GameMap &map, size_t x, size_t y)
        { 
            ctl.init_point(map, x, y);
            return DotProxy(map, x, y);
        })
        .def("is_placeable_on", &MaterialController::is_placeable_on)
    ;

    m.def("ls_materials", []
    {
        return py::dict(py::cast(g_materials_map, py::return_value_policy::reference));
    });

    py::class_<Renderer>(m, "Renderer")
        .def(py::init([](GameMap &map, py::object dst_surf, py::object bg_color)
        {
            return Renderer(map, dst_surf, py::int_(bg_color));
        }))
        .def("render", &Renderer::render)
        .def("take_screenshot", [](Renderer &) {})
        .def("next_render_mask", [](Renderer &) {})
    ;
}
