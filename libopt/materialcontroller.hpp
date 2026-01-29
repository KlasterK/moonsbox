#ifndef MOOX_MATERIALCONTROLLER_HPP
#define MOOX_MATERIALCONTROLLER_HPP

#include "materialdefs.hpp"

class MaterialController
{
public:
    virtual ~MaterialController() = default;
    virtual void init_point(GameMap &, size_t, size_t) {};
    virtual void static_update(GameMap &) {};
    virtual void dynamic_update(GameMap &) {};
    virtual void on_register(class SimulationManager &) {};

    inline MaterialID material_id()
    {
        return reinterpret_cast<MaterialID>(this);
    }
};

#endif // MOOX_MATERIALCONTROLLER_HPP
