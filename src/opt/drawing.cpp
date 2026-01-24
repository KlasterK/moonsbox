#include "drawing.hpp"

void draw::fill(GameMap &map, T material_factory)
{
    for(size_t y{}; y < map.height(); ++y)
    {
        for(size_t x{}; x < map.width(); ++x)
        {
            map.temps(x, y) = static_cast<float>(material_factory);
        }
    }
}
