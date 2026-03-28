#ifndef MOOX_ICE_HPP
#define MOOX_ICE_HPP

#include <simulationengine/materials/common.hpp>

class Ice : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 220.f;
        map.heat_capacities(x, y) = 0.7f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0x66C8E0B7;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

    inline void pre_static_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Ice was never registered in SimulationManager");

        if(m_water == nullptr)
        {
            m_water = m_registry->find_controller_by_name("Water");
            if(m_water == nullptr)
                throw std::logic_error("Ice cannot not find Water material in SimulationManager");
        }
    }

    inline void static_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(map.material_ctls(x, y) != this)
            return;

        if(float temp = map.temps(x, y); temp > 275.f)
        {
            m_water->init_point(map, x, y);
            map.temps(x, y) = temp;
            if(m_play_sound_cb)
                m_play_sound_cb("convert.Ice_melts", std::nullopt, false);
        }
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

    inline void set_play_sound_callback(PlaySoundCallback &&cb) override
    {
        m_play_sound_cb = std::move(cb);
    }

    inline void play_place_sound(GameMap &, size_t, size_t) override
    {
        if(m_play_sound_cb)
            m_play_sound_cb("material.Ice", std::nullopt, false);
    }

private:
    PlaySoundCallback m_play_sound_cb{nullptr};
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_water = nullptr;
};


#endif // MOOX_ICE_HPP
