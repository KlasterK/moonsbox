#ifndef MOOX_MINUS100K_HPP
#define MOOX_MINUS100K_HPP

#include <simulationengine/materials/common.hpp>

class Minus100K : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = std::max(0.001f, map.temps(x, y) - 100.f);
    }
};


#endif // MOOX_MINUS100K_HPP
