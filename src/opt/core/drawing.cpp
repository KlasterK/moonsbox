#include "drawing.hpp"
#include "gamemap.hpp"

void drawing::fill(GameMap &map, setup_dot_func_t factory)
{
    auto [w, h] = map.get_size();
    for(int x{}; x < w; ++x)
    {
        for(int y{}; y < h; ++y)
        {
            factory({x, y});
        }
    }
}
