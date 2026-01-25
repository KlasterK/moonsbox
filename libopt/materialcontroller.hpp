#ifndef MOOX_MATERIALCONTROLLER_HPP
#define MOOX_MATERIALCONTROLLER_HPP

#include "materialdefs.hpp"

class MaterialController
{
public:
    virtual void init_point(GameMap &map, size_t x, size_t y) = 0;
    virtual void static_update(GameMap &map) = 0;
    virtual void dynamic_update(GameMap &map) = 0;

    inline MaterialID material_id()
    {
        return reinterpret_cast<MaterialID>(this);
    }
};

#endif // MOOX_MATERIALCONTROLLER_HPP
