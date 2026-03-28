#ifndef MOOX_ABSORBENT_HPP
#define MOOX_ABSORBENT_HPP

#include <simulationengine/materials/common.hpp>

class Absorbent : public MaterialController
{
public:
    inline std::pair<std::vector<uint8_t>, SemanticVersion> 
        serialize(const GameMap &map, size_t x, size_t y) override
    {
        auto *aux = std::any_cast<int32_t>(&map.auxs(x, y));
        if(aux == nullptr)
            return {};

        return {
            {static_cast<uint8_t>(*aux)},
            {2, 0, 0, 0}
        };
    }

    inline DeserializationResult deserialize(GameMap &map, size_t x, size_t y, 
                                            std::span<const uint8_t> data, 
                                            SemanticVersion ver) override
    { 
        if(data.size() != 1)
            return DeserializationResult::InvalidDataLength;

        if(ver.major < 2)
            return DeserializationResult::VersionTooOld;

        if(ver.major > 2 || ver.minor > 0)
            return DeserializationResult::VersionTooNew;

        map.auxs(x, y).emplace<int32_t>(data[0]);
        return DeserializationResult::Success;
    }

    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        uint32_t grayscale = 0xDD + fastprng::get_u64() % (0xFF - 0xDD);
        uint32_t yellowness = 0x11 + fastprng::get_u64() % (0x33 - 0x11);
        map.colors(x, y) = grayscale << 24 | grayscale << 16 
                         | (grayscale - yellowness) << 8 | 0xFF;
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 0.1f;
        map.tags(x, y).reset().set(MtlTag::Float);
        map.auxs(x, y).emplace<int32_t>(fastprng::get_u64() % 200);
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        map.material_ctls(x, y) = this;
    }

    inline void pre_dynamic_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Absorbent was never registered in SimulationManager");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error(
                    "Absorbent cannot not find Space material in SimulationManager"
                );
        }
    }

    inline void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        auto *ttl = std::any_cast<int32_t>(&map.auxs(x, y));
        if(ttl == nullptr)
            return;

        for(auto [dx, dy] : g_moore_deltas)
        {
            if(!map.in_bounds(x+dx, y+dy) || !map.tags(x+dx, y+dy).test(MtlTag::Liquid))
                continue;

            m_space->init_point(map, x+dx, y+dy);
            *ttl -= 50;
        }

        if(*ttl < 0)
            m_space->init_point(map, x, y);
        else
            *ttl -= 1;
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


#endif // MOOX_ABSORBENT_HPP
