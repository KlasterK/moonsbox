#ifndef MOOX_STEAM_HPP
#define MOOX_STEAM_HPP

#include <simulationengine/materials/common.hpp>

class Steam : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 420.f;
        map.heat_capacities(x, y) = 0.7f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0x28BBC56D;
        map.tags(x, y).reset().set(MtlTag::Gas);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
        map.material_ctls(x, y) = this;
    }

    inline void pre_static_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Steam was never registered in SimulationManager");

        if(m_water == nullptr)
        {
            m_water = m_registry->find_controller_by_name("Water");
            if(m_water == nullptr)
                throw std::logic_error("Steam cannot not find Water material in SimulationManager");
        }
    }

    inline void static_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(map.material_ctls(x, y) != this)
            return;

        if(float temp = map.temps(x, y); temp < 370.f)
        {
            m_water->init_point(map, x, y);
            map.temps(x, y) = temp;
            if(m_play_sound_cb)
                m_play_sound_cb("convert.Steam_condensates", std::nullopt, false);
        }
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return MtlTag::IsSparseness(map.tags(x, y));
    }

    inline void set_play_sound_callback(PlaySoundCallback &&cb) override
    {
        m_play_sound_cb = std::move(cb);
    }

    inline void play_place_sound(GameMap &, size_t, size_t) override
    {
        if(m_play_sound_cb)
            m_play_sound_cb("material.Steam", std::nullopt, false);
    }

private:
    PlaySoundCallback m_play_sound_cb{nullptr};
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_water = nullptr;
};


#endif // MOOX_STEAM_HPP
