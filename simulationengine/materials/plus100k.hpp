#ifndef MOOX_PLUS100K_HPP
#define MOOX_PLUS100K_HPP

#include <simulationengine/materials/common.hpp>

class Plus100K : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) += 100.f;
    }
};


#endif // MOOX_PLUS100K_HPP
