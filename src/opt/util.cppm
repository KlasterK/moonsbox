module;
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
export module util;

namespace py = pybind11;
using namespace py::literals;

export using Point = std::array<int, 2>;

export template<typename T>
using opt_ref = std::optional<std::reference_wrapper<T>>;

struct Rect : std::array<int, 4>
{
public:
    int w()         { return (*this)[2]; }
    int x0()        { return (*this)[0]; }
    int x1()        { return (*this)[0] + (*this)[2]; }
    int center_x()  { return (*this)[0] + (*this)[2] / 2; }

    int h()         { return (*this)[3]; }
    int y0()        { return (*this)[1]; }
    int y1()        { return (*this)[1] + (*this)[3]; }
    int center_y()  { return (*this)[1] + (*this)[3] / 2; }
};

export class Clock
{
public:
    // FIXME: implement Clock class
    double get_fps() const { return 0.0; }
    void tick(double framerate = 0.0) {}
};

constexpr float operator ""_kelv(long double f) { return f; }
constexpr float operator ""_cels(long double f) { return f + 273.15; }
