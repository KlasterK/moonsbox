#include <Python.h>
#include <pybind11/pybind11.h>
#include "gamemap.hpp"
#include "drawing.hpp"

namespace py = pybind11;

PYBIND11_MODULE(opt, m)
{
    auto pygame = py::module_::import("pygame");

    py::class_<DotProxy>(m, "DotProxy")
        .def_property("temp",
                      [](DotProxy &self) { return self.temp; },
                      [](DotProxy &self, float value) { self.temp = value; })
        .def_property("heat_capacity",
                      [](DotProxy &self) { return self.heat_capacity; },
                      [](DotProxy &self, float value) { self.heat_capacity = value; })
        .def_property("thermal_conductivity",
                      [](DotProxy &self) { return self.thermal_conductivity; },
                      [](DotProxy &self, float value) { self.thermal_conductivity = value; })
        .def_property("color",
                      [](DotProxy &self) { return self.color; },
                      [](DotProxy &self, uint32_t value) { self.color = value; })
        .def_property("tags",
                      [](DotProxy &self) { return self.tags.to_ullong(); },
                      [](DotProxy &self, unsigned long long value) { self.tags = value; })
        .def_property("physical_entity",
                      [](DotProxy &self) { return static_cast<unsigned long long>(self.physical_behavior); },
                      [](DotProxy &self, unsigned long long value) { self.physical_behavior = static_cast<MaterialPhysicalBehavior>(value); })
    ;

    py::class_<GameMap>(m, "GameMap")
        .def("__getitem__", [](GameMap &map, std::array<int, 2> pos) -> py::object
        {
            if(!map.in_bounds(pos[0], pos[1]))
                return py::none();
            return py::cast(map.make_proxy(pos[0], pos[1]), py::return_value_policy::reference_internal);
        })
        .def("__setitem__", [](GameMap &map, std::array<int, 2> pos, DotProxy value)
        {
            if(!map.in_bounds(pos[0], pos[1]))
                return;
            map.temps(pos[0], pos[1]) = value.temp;
            map.heat_capacities(pos[0], pos[1]) = value.heat_capacity;
            map.thermal_conductivities(pos[0], pos[1]) = value.thermal_conductivity;
            map.colors(pos[0], pos[1]) = value.color;
            map.tags(pos[0], pos[1]) = value.tags;
            map.physical_behaviors(pos[0], pos[1]) = value.physical_behavior;
            map.auxs(pos[0], pos[1]) = value.aux;
            map.material_ids(pos[0], pos[1]) = value.id;
        })
        .def("invy", [](GameMap &map, int y) { return map.height() - 1 - y; })
        .def("invy_pos", [](GameMap &map, std::array<int, 2> pos)
        {
            return std::array{pos[0], int(map.height() - 1 - pos[1])};
        })
        .def("bounds", [](GameMap &map, std::array<int, 2> pos) { return map.in_bounds(pos[0], pos[1]); })
        .def("fill", &draw::fill)
    ;
}
