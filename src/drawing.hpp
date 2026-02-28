#ifndef MOOX_DRAWING_HPP
#define MOOX_DRAWING_HPP

#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include <functional>

namespace drawing
{
    using SignedPoint = std::array<int, 2>;
    using Rect = std::array<int, 4>;
    using MaterialFactory = std::function<void(size_t, size_t)>;

    enum class LineEnds {None, Square, Round};

    void swap(GameMap &map, size_t ax, size_t ay, size_t bx, size_t by);

    void fill(GameMap &map, MaterialFactory material_factory);

    void rect(GameMap &map, Rect area, MaterialFactory material_factory);

    void ellipse(GameMap &map, Rect area, MaterialFactory material_factory);

    void line(GameMap &map, SignedPoint begin, SignedPoint end, int width, 
              MaterialFactory material_factory, LineEnds ends);
    
    inline MaterialFactory make_controller_init_point_factory(GameMap &map, MaterialController &ctl)
    {
        return [&](size_t x, size_t y) { ctl.init_point(map, x, y); };
    }
}

#endif // MOOX_DRAWING_HPP
