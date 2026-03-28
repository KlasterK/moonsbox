#ifndef MOOX_FIRE_HPP
#define MOOX_FIRE_HPP

#include <simulationengine/materials/common.hpp>

class Fire : public MaterialController
{
private:
    static constexpr size_t   StepsCount    = 20;
    static constexpr uint32_t MinTTLColor   = 0xFF000044;
    static constexpr uint32_t MaxTTLColor   = 0xFFFF00FF;
    static constexpr uint32_t ColorStep     = (0xFF - 0x00) / StepsCount * 0x10000 
                                            + (0xFF - 0x44) / StepsCount;

public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 1000.f;
        map.heat_capacities(x, y) = 1.f;
        map.thermal_conductivities(x, y) = 1.f;
        map.colors(x, y) = MaxTTLColor - ColorStep * (fastprng::get_u64() % StepsCount);
        map.tags(x, y).reset().set(MtlTag::Gas);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
        map.material_ctls(x, y) = this;
    }

    inline void pre_dynamic_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Fire was never registered in SimulationManager");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error(
                    "Fire cannot not find Space material in SimulationManager"
                );
        }
    }

    inline void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(map.material_ctls(x, y) != this)
            return;

        map.colors(x, y) -= ColorStep;
        if(map.colors(x, y) <= MinTTLColor)
        {
            float old_temp = map.temps(x, y);
            m_space->init_point(map, x, y);
            map.temps(x, y) = old_temp;
        }
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return map.tags(x, y).test(MtlTag::Space);
    }

    inline void set_play_sound_callback(PlaySoundCallback &&cb) override
    {
        m_play_sound_cb = std::move(cb);
    }

    inline void play_place_sound(GameMap &, size_t, size_t) override
    {
        if(m_play_sound_cb)
            m_play_sound_cb("material.Fire", std::nullopt, false);
    }

private:
    PlaySoundCallback m_play_sound_cb{nullptr};
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_space = nullptr;
};


#endif // MOOX_FIRE_HPP
