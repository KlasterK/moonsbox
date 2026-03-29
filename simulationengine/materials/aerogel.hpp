#ifndef MOOX_AEROGEL_HPP
#define MOOX_AEROGEL_HPP

#include <simulationengine/materials/common.hpp>

class Aerogel : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.99f;
        map.thermal_conductivities(x, y) = 0.01f;
        uint32_t grayscale = 0xAA + fastprng::get_u64() % 0x12;
        map.colors(x, y) = grayscale << 24 | grayscale << 16 | grayscale << 8 | 0x40;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }
};


#endif // MOOX_AEROGEL_HPP
