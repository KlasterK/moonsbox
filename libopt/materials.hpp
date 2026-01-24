#ifndef MOOX_MATERIALS_HPP
#define MOOX_MATERIALS_HPP

#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include <iostream>

class Space : public MaterialController
{
public:
    Space(GameMap &map) : m_map(map) {}

    inline void init_point(size_t x, size_t y) override
    {
        m_map.temps(x, y) = 300.f;
        m_map.heat_capacities(x, y) = 0.3f;
        m_map.thermal_conductivities(x, y) = 1.f;
        m_map.colors(x, y) = 0x00000000;
        m_map.tags(x, y).reset().set(MtlTag::Space);
        m_map.auxs(x, y).reset();
        m_map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        m_map.material_ids(x, y) = material_id();
    }

    inline void static_update() override
    {}

    inline void dynamic_update() override
    {}

private:
    GameMap &m_map;
};

class Sand : public MaterialController
{
public:
    Sand(GameMap &map) : m_map(map) {}

    inline void init_point(size_t x, size_t y) override
    {
        m_map.temps(x, y) = 300.f;
        m_map.heat_capacities(x, y) = 0.3f;
        m_map.thermal_conductivities(x, y) = 0.1f;
        m_map.colors(x, y) = 0xFF9900FF | + (rand() % (0xFF - 0x99) * 0x10000);
        m_map.tags(x, y).reset().set(MtlTag::Bulk);
        m_map.auxs(x, y).reset();
        m_map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        m_map.material_ids(x, y) = material_id();
    }

    inline void static_update() override
    {}

    inline void dynamic_update() override
    {}

private:
    GameMap &m_map;
};

#endif // MOOX_MATERIALS_HPP
