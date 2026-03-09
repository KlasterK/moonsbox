#ifndef MOOX_LAVA_HPP
#define MOOX_LAVA_HPP

#include <simulationengine/materials/common.hpp>

class Lava : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 1200.f;
        map.heat_capacities(x, y) = 0.8f;
        map.thermal_conductivities(x, y) = 0.5f;
        map.tags(x, y).reset().set(MtlTag::Liquid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
        map.material_ctls(x, y) = this;
    }

    inline void static_update(GameMap &map) override
    {
        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ctls(x, y) != this)
                    continue;
                
                auto temp = static_cast<int32_t>(map.temps(x, y));
                if(temp > 800)
                {
                    map.colors(x, y) = (
                        0xFF0000FF
                        | _map_clamp(temp, 800, 1200, 0x00, 0xFF) << 16
                    );
                    map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
                    map.tags(x, y).reset().set(MtlTag::Liquid);
                }
                else
                {
                    map.colors(x, y) = (
                        0x000000FF
                        | _map_clamp(temp, 400, 800, 0x44, 0xFF) << 24
                    );
                    map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
                    map.tags(x, y).reset().set(MtlTag::Solid);
                }
            }
        }
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
            m_play_sound_cb("material.Lava", std::nullopt, false);
    }

private:
    PlaySoundCallback m_play_sound_cb{nullptr};
};


#endif // MOOX_LAVA_HPP
