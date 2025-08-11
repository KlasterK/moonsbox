module;
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
export module util;

export using Point = std::array<int, 2>;

export std::array<int, 4> pygame_rect_to_xywh(py::object rect)
{
    return {
        rect.attr("x").cast<int>(),
        rect.attr("y").cast<int>(),
        rect.attr("w").cast<int>(),
        rect.attr("h").cast<int>(),
    };
}