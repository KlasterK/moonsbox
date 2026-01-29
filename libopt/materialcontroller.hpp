#ifndef MOOX_MATERIALCONTROLLER_HPP
#define MOOX_MATERIALCONTROLLER_HPP

#include "materialdefs.hpp"

class MaterialController
{
public:
    virtual ~MaterialController() = default;
    virtual void init_point(GameMap &map, size_t x, size_t y) {};
    virtual void static_update(GameMap &map) {};
    virtual void dynamic_update(GameMap &map) {};
    virtual void on_register(class SimulationManager &) {};

    inline MaterialID material_id()
    {
        return reinterpret_cast<MaterialID>(this);
    }
};

#endif // MOOX_MATERIALCONTROLLER_HPP
