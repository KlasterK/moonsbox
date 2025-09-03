#ifndef KK_OPT_UTIL_HPP
#define KK_OPT_UTIL_HPP

#include <array>
#include <optional>

template<typename T>
using opt_ref = std::optional<std::reference_wrapper<T>>;

struct Rect : std::array<int, 4>
{
public:
    constexpr int w()        const { return (*this)[2]; }
    constexpr int x0()       const { return (*this)[0]; }
    constexpr int x1()       const { return (*this)[0] + (*this)[2]; }
    constexpr int center_x() const { return (*this)[0] + (*this)[2] / 2; }

    constexpr int h()        const { return (*this)[3]; }
    constexpr int y0()       const { return (*this)[1]; }
    constexpr int y1()       const { return (*this)[1] + (*this)[3]; }
    constexpr int center_y() const { return (*this)[1] + (*this)[3] / 2; }
};

class Clock
{
public:
    // FIXME: implement Clock class
    inline double get_fps() const { return 0.0; }
    inline void tick(double framerate = 0.0) {}
};

constexpr float operator ""_kelv(long double f) { return f; }
constexpr float operator ""_cels(long double f) { return f + 273.15; }

#endif // include guard