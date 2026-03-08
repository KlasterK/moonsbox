#ifndef MOOX_DRAWING_HPP
#define MOOX_DRAWING_HPP

#include <cstddef>
#include <array>
#include <functional>
#include <simulationengine/core/materialcontroller.hpp>

class GameMap;

namespace drawing
{
    using SignedPoint = std::array<int, 2>;
    using Rect = std::array<int, 4>;
    using MaterialFactory = std::move_only_function<void(size_t, size_t)>;

    enum class LineEnds {None, Square, Round};

    void swap(GameMap &map, size_t ax, size_t ay, size_t bx, size_t by);

    void fill(GameMap &map, MaterialFactory &material_factory);

    void rect(GameMap &map, Rect area, MaterialFactory &material_factory);

    void ellipse(GameMap &map, Rect area, MaterialFactory &material_factory);

    void line(GameMap &map, SignedPoint begin, SignedPoint end, int width, 
              MaterialFactory &material_factory, LineEnds ends);
    
    inline MaterialFactory make_controller_init_point_factory(GameMap &map, MaterialController &ctl)
    {
        return [&](size_t x, size_t y) { ctl.init_point(map, x, y); };
    }

    // rvalue overloads since you can't pass a prvalue as lvalue non-const reference

    inline void fill(GameMap &map, MaterialFactory &&material_factory) 
    { 
        fill(map, material_factory); 
    }
    inline void rect(GameMap &map, Rect area, MaterialFactory &&material_factory) 
    { 
        rect(map, area, material_factory); 
    }
    inline void ellipse(GameMap &map, Rect area, MaterialFactory &&material_factory) 
    { 
        ellipse(map, area, material_factory); 
    }
    inline void line(GameMap &map, SignedPoint begin, SignedPoint end, int width, 
                     MaterialFactory &&material_factory, LineEnds ends) 
    { 
        line(map, begin, end, width, material_factory, ends); 
    }
}

#endif // MOOX_DRAWING_HPP
