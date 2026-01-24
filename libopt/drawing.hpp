#ifndef DRAWING_HPP
#define DRAWING_HPP

#include "gamemap.hpp"


namespace draw
{
using T = int;
using Point = std::array<int, 2>;
using Rect  = std::array<int, 4>;

enum class LineEnds {Square, Round};

void fill(GameMap &map, T material_factory);
void rect(GameMap &map, Rect area, T material_factory);
void ellipse(GameMap &map, Rect area, T material_factory);
void line(GameMap &map, Point begin, Point end, int width, T material_factory, LineEnds ends);
}

#endif // DRAWING_HPP
