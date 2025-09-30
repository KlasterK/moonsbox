#ifndef KK_OPT_DRAWING_HPP
#define KK_OPT_DRAWING_HPP

#include <modapi.h>
#include "util.hpp"

class GameMap;

namespace drawing
{
    enum LineEnds { Square, Round };

    void fill(GameMap& map, setup_dot_func_t factory);

    void draw_rect(GameMap& map, const Rect& rect, setup_dot_func_t factory);

    void draw_ellipse(GameMap& map, const Rect& rect, setup_dot_func_t factory);

    void draw_line(
        GameMap &map, 
        Point begin, 
        Point end, 
        int width, 
        setup_dot_func_t factory, 
        LineEnds ends
    );
}

#endif // include guard