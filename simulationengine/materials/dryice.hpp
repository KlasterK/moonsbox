#ifndef MOOX_DRYICE_HPP
#define MOOX_DRYICE_HPP

#include <simulationengine/materials/common.hpp>

class DryIce : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        int random_value = fastprng::get_u8();
        map.colors(x, y) = uint32_t(
            _map_clamp(random_value, 0, 255, 0xDB, 0xC2) << 24
            | _map_clamp(random_value, 0, 255, 0xE2, 0xD9) << 16
            | _map_clamp(random_value, 0, 255, 0xEE, 0xDF) << 8
        );
        map.temps(x, y) = 175.f;
        map.heat_capacities(x, y) = 0.95f;
        map.thermal_conductivities(x, y) = 1.f;
        map.tags(x, y).reset().set(MtlTag::Bulk);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        map.material_ctls(x, y) = this;
    }

    inline void static_update(GameMap &map) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("DryIce was never registered in SimulationManager");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error(
                    "DryIce cannot not find Space material in SimulationManager"
                );
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ctls(x, y) != this)
                    continue;

                auto temp = static_cast<int32_t>(map.temps(x, y));
                if(temp > 250)
                {
                    m_space->init_point(map, x, y);
                }
                else if(temp > 195)
                {
                    map.tags(x, y).reset().set(MtlTag::Gas);
                    map.physical_behaviors(x, y) = MaterialPhysicalBehavior::HeavyGas;
                }
                else
                {
                    map.tags(x, y).reset().set(MtlTag::Bulk);
                    map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
                }

                uint32_t &color = map.colors(x, y);
                color &= 0xFFFFFF00;
                color |= _map_clamp(temp, 175, 250, 0xFF, 0x00);
            }
        }
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        auto &tags = map.tags(x, y);
        return MtlTag::IsMovable(tags) || tags.test(MtlTag::Space);
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

private:
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_space = nullptr;
};


#endif // MOOX_DRYICE_HPP
