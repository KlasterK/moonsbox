#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

import gamemap;

PYBIND11_MODULE(opt, m)
{
    py::class_<GameMap>(m, "GameMap")
        .def(py::init<Point>())
        .def("__getitem__", &GameMap::get_at)
        .def("__setitem__", &GameMap::set_at)
        .def_property("size", &GameMap::get_size, nullptr)
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
        
    py::class_<SimulationManager>(m, "SimulationManager")
        .def(py::init<py::object>())
        .def("tick", &SimulationManager::tick)
        .def("get_tps", &SimulationManager::get_tps);
}