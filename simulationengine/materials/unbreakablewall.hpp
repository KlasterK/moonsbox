#ifndef MOOX_UNBREAKABLEWALL_HPP
#define MOOX_UNBREAKABLEWALL_HPP

#include <simulationengine/materials/common.hpp>

class UnbreakableWall : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.6f;
        map.thermal_conductivities(x, y) = 0.4f;
        map.colors(x, y) = 0xFFFFFFFF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }
};


#endif // MOOX_UNBREAKABLEWALL_HPP
