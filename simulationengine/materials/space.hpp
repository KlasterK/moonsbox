#ifndef MOOX_SPACE_HPP
#define MOOX_SPACE_HPP

#include <simulationengine/materials/common.hpp>

class Space : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.01f;
        map.thermal_conductivities(x, y) = 1.f;
        map.colors(x, y) = 0x00000000;
        map.tags(x, y).reset().set(MtlTag::Space);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }
};

#endif // MOOX_SPACE_HPP
