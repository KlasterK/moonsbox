#ifndef MOOX_PROPANE_HPP
#define MOOX_PROPANE_HPP

#include <simulationengine/materials/common.hpp>

constexpr uint32_t SolidColor  = 0x6D8EC9BB;
constexpr uint32_t LiquidColor = 0x5376B899;
constexpr uint32_t GasColor    = 0x376CFF66;

class Propane : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 0.5f;
        map.colors(x, y) = GasColor;
        map.tags(x, y).reset().set(MtlTag::Gas);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
        map.material_ctls(x, y) = this;
    }

    inline void pre_static_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Propane was never registered in SimulationManager");

        if(m_fire == nullptr)
        {
            m_fire = m_registry->find_controller_by_name("Fire");
            if(m_fire == nullptr)
                throw std::logic_error(
                    "Propane cannot not find Fire material in SimulationManager"
                );
        }
    }

    inline void static_update_point(GameMap &map, size_t x, size_t y) override
    {
        float temp = map.temps(x, y);
        auto tags = map.tags(x, y);

        if(temp > 700.f)
        {
            m_fire->init_point(map, x, y);
            m_fire->play_place_sound(map, x, y);
            map.temps(x, y) = std::max(temp, 2800.f);
            
            for(auto [dx, dy] : g_moore_deltas)
            {
                if(!map.in_bounds(x+dx, y+dy) || map.material_ctls(x+dx, y+dy) != this)
                    continue;
                
                m_fire->init_point(map, x+dx, y+dy);
                map.temps(x+dx, y+dy) = std::max(temp, 2800.f);
            }
        }
        else if(tags.test(MtlTag::Solid))
        {
            if(temp > 85.f)
            {
                map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
                map.tags(x, y).reset().set(MtlTag::Liquid);
                map.colors(x, y) = LiquidColor;
            }
        }
        else if(tags.test(MtlTag::Liquid))
        {
            if(temp < 80.f)
            {
                map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
                map.tags(x, y).reset().set(MtlTag::Solid);
                map.colors(x, y) = SolidColor;
            }
            else if(temp > 235.f)
            {
                map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
                map.tags(x, y).reset().set(MtlTag::Gas);
                map.colors(x, y) = GasColor;
            }
        }
        else if(tags.test(MtlTag::Gas))
        {
            if(temp < 230.f)
            {
                map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
                map.tags(x, y).reset().set(MtlTag::Liquid);
                map.colors(x, y) = LiquidColor;
            }
        }
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        auto &tags = map.tags(x, y);
        return MtlTag::IsSparseness(tags);
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

private:
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_fire = nullptr;
};



#endif // MOOX_PROPANE_HPP
