#ifndef MOOX_BLACKHOLE_HPP
#define MOOX_BLACKHOLE_HPP

#include <simulationengine/materials/common.hpp>

class BlackHole : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.f;
        map.thermal_conductivities(x, y) = 0.f;
        map.colors(x, y) = 0x1F1F1FFF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

    inline void pre_dynamic_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("BlackHole was never registered in SimulationManager");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error(
                    "BlackHole cannot not find Space material in SimulationManager"
                );
        }
    }

    inline void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(map.material_ctls(x, y) != this)
            return;

        for(auto [dx, dy] : g_von_neumann_deltas)
        {
            if(map.in_bounds(x+dx, y+dy) && !map.tags(x+dx, y+dy).test(MtlTag::Solid))
            {
                m_space->init_point(map, x+dx, y+dy);
                m_space->play_place_sound(map, x+dx, y+dy);
            }
        }
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

private:
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_space = nullptr;
};


#endif // MOOX_BLACKHOLE_HPP
