#ifndef MOOX_UNBREAKABLEWALL_HPP
#define MOOX_UNBREAKABLEWALL_HPP

#include <simulationengine/materials/common.hpp>

constexpr size_t BrickworkWidth  = 8;
constexpr size_t BrickworkHeight = 8;
constexpr std::array<std::bitset<BrickworkWidth>, BrickworkHeight> BrickworkPattern{
    0b11111111,
    0b10000000,
    0b10000000,
    0b10000000,
    0b11111111,
    0b00001000,
    0b00001000,
    0b00001000,
};

class UnbreakableWall : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.6f;
        map.thermal_conductivities(x, y) = 0.4f;
        map.colors(x, y) = BrickworkPattern[y % BrickworkHeight]
                           .test(BrickworkWidth - 1 - (x % BrickworkWidth))
                           ? 0xCCCCCCFF : 0xFFFFFFFF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }
};


#endif // MOOX_UNBREAKABLEWALL_HPP
