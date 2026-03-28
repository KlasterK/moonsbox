#ifndef MOOX_PUREGLASS_HPP
#define MOOX_PUREGLASS_HPP

#include <simulationengine/materials/common.hpp>

class PureGlass : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.5f;
        map.thermal_conductivities(x, y) = 0.05f;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

    inline void static_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(map.material_ctls(x, y) != this)
            return;

        auto temp = static_cast<int32_t>(map.temps(x, y));

        map.colors(x, y) = (
            _map_clamp(temp,   400, 1773, 0x53, 0xFF) << 24
            | _map_clamp(temp, 400, 1773, 0xD4, 0xAA) << 16
            | _map_clamp(temp, 400, 1773, 0x98, 0x00) << 8
            | _map_clamp(temp, 400, 1773, 0x20, 0x85)
        );

        if(temp > 1773)
        {
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
            map.tags(x, y).reset().set(MtlTag::Liquid);
        }
        else
        {
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
            map.tags(x, y).reset().set(MtlTag::Solid);
        }
    }
};


#endif // MOOX_PUREGLASS_HPP
