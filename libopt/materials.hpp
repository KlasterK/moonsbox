#ifndef MOOX_MATERIALS_HPP
#define MOOX_MATERIALS_HPP

#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include <iostream>

class Space : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 1.f;
        map.colors(x, y) = 0x00000000;
        map.tags(x, y).reset().set(MtlTag::Space);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &) override
    {}

    inline void dynamic_update(GameMap &) override
    {}
};

class Sand : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 0.1f;
        map.colors(x, y) = 0xFF9900FF | + (rand() % (0xFF - 0x99) * 0x10000);
        map.tags(x, y).reset().set(MtlTag::Bulk);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &) override
    {}

    inline void dynamic_update(GameMap &) override
    {}
};

#endif // MOOX_MATERIALS_HPP
