#ifndef MOOX_WATER_HPP
#define MOOX_WATER_HPP

#include <simulationengine/materials/common.hpp>

class Water : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.7f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0x009599FF | fastprng::get_u64() % (0xBB - 0x95) * 0x10000;
        map.tags(x, y).reset().set(MtlTag::Liquid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
        map.material_ctls(x, y) = this;
    }

    inline void pre_static_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Water was never registered in SimulationManager");

        if(m_steam == nullptr)
        {
            m_steam = m_registry->find_controller_by_name("Steam");
            if(m_steam == nullptr)
                throw std::logic_error("Water cannot not find Steam material in SimulationManager");
        }

        if(m_ice == nullptr)
        {
            m_ice = m_registry->find_controller_by_name("Ice");
            if(m_ice == nullptr)
                throw std::logic_error("Water cannot not find Ice material in SimulationManager");
        }
    }

    inline void static_update_point(GameMap &map, size_t x, size_t y) override
    {
        float temp = map.temps(x, y);
        if(temp < 270.f)
        {
            m_ice->init_point(map, x, y);
            map.temps(x, y) = temp;
            if(m_play_sound_cb)
                m_play_sound_cb("convert.Water_freezes", std::nullopt, false);
        }
        else if(temp > 375.f)
        {
            m_steam->init_point(map, x, y);
            map.temps(x, y) = temp;
            if(m_play_sound_cb)
                m_play_sound_cb("convert.Water_evaporates", std::nullopt, false);
        }
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return MtlTag::IsFlowable(map.tags(x, y));
    }

    inline void set_play_sound_callback(PlaySoundCallback &&cb) override
    {
        m_play_sound_cb = std::move(cb);
    }

    inline void play_place_sound(GameMap &, size_t, size_t) override
    {
        if(m_play_sound_cb)
            m_play_sound_cb("material.Water", std::nullopt, false);
    }

private:
    PlaySoundCallback m_play_sound_cb{nullptr};
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_steam = nullptr, *m_ice = nullptr;
};


#endif // MOOX_WATER_HPP
