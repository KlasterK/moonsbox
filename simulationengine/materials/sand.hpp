#ifndef MOOX_SAND_HPP
#define MOOX_SAND_HPP

#include <simulationengine/materials/common.hpp>

class Sand : public MaterialController
{
private:
    static constexpr int32_t SandColorR = 0xFF, SandColorB = 0, SandColorA = 0xFF;
    struct Aux
    {
        bool is_glass{};
        uint32_t sand_color_g{};
    };

public:
    inline std::pair<std::vector<uint8_t>, SemanticVersion> 
        serialize(const GameMap &map, size_t x, size_t y) override
    {
        auto *aux = std::any_cast<Aux>(&map.auxs(x, y));
        if(aux == nullptr)
            return {};

        return {
            {aux->is_glass, static_cast<uint8_t>(aux->sand_color_g)}, 
            {2, 0, 0, 0}
        };
    }

    inline DeserializationResult deserialize(GameMap &map, size_t x, size_t y, 
                                            std::span<const uint8_t> data, 
                                            SemanticVersion ver) override
    { 
        if(data.size() != 2)
            return DeserializationResult::InvalidDataLength;

        if(ver.major < 2)
            return DeserializationResult::VersionTooOld;

        if(ver.major > 2 || ver.minor > 0)
            return DeserializationResult::VersionTooNew;

        map.auxs(x, y).emplace<Aux>(data[0], data[1]);
        return DeserializationResult::Success;
    }

    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 0.1f;
        map.tags(x, y).reset().set(MtlTag::Bulk);
        map.auxs(x, y).emplace<Aux>(false, 0x99 + fastprng::get_u64() % (0xFF - 0x99));
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        map.material_ctls(x, y) = this;
    }

    inline void static_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(map.material_ctls(x, y) != this)
            return;

        auto *aux = std::any_cast<Aux>(&map.auxs(x, y));
        if(aux == nullptr)
            return;
        
        auto temp = static_cast<int32_t>(map.temps(x, y));

        if(aux->is_glass)
            map.colors(x, y) = uint32_t(
                _map_clamp(temp,   400, 1973, 0x96, 0xFF) << 24
                | _map_clamp(temp, 400, 1973, 0x94, 0x88) << 16
                | _map_clamp(temp, 400, 1973, 0x77, 0x00) << 8
                | _map_clamp(temp, 400, 1973, 0x55, 0x85)
            );
        else
            map.colors(x, y) = uint32_t(
                SandColorR << 24
                | _map_clamp(temp, 400, 1973, 
                             static_cast<int32_t>(aux->sand_color_g), 0x66) << 16
                | SandColorB << 8
                | _map_clamp(temp, 400, 1973, SandColorA, 0xAA)
            );

        if(temp > 1973)
        {
            if(!aux->is_glass)
            {
                aux->is_glass = true;
                if(m_play_sound_cb)
                    m_play_sound_cb("convert.Sand_to_glass", std::nullopt, false);
            }
                
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
            map.tags(x, y).reset().set(MtlTag::Liquid);
        }
        else if(!aux->is_glass)
        {
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
            map.tags(x, y).reset().set(MtlTag::Bulk);
        }
        else
        {
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
            map.tags(x, y).reset().set(MtlTag::Solid);
        }
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        auto &tags = map.tags(x, y);
        return MtlTag::IsMovable(tags) || tags.test(MtlTag::Space);
    }

    inline void set_play_sound_callback(PlaySoundCallback &&cb) override
    {
        m_play_sound_cb = std::move(cb);
    }

    inline void play_place_sound(GameMap &, size_t, size_t) override
    {
        if(m_play_sound_cb)
            m_play_sound_cb("material.Sand", std::nullopt, false);
    }

private:
    PlaySoundCallback m_play_sound_cb{nullptr};
};

#endif // MOOX_SAND_HPP
