#ifndef MOOX_TAP_HPP
#define MOOX_TAP_HPP

#include <cstring>
#include <format>
#include <simulationengine/materials/common.hpp>

class Tap : public MaterialController
{
public:
    inline std::pair<std::vector<uint8_t>, SemanticVersion> 
        serialize(const GameMap &map, size_t x, size_t y) override
    {
        auto *aux = std::any_cast<MaterialController *>(&map.auxs(x, y));
        if(aux == nullptr)
            return {};

        auto name_opt = m_registry->get_name_of_controller(*aux);
        if(!name_opt)
            throw std::logic_error(std::format(
                "Tap::serialize: material controller at 0x{:x}"
                " was never registered in the registry",
                reinterpret_cast<uintptr_t>(*aux)
            ));

        std::vector<uint8_t> vec(name_opt->size());
        std::memcpy(vec.data(), name_opt->data(), name_opt->size());

        return {
            std::move(vec),
            {2, 0, 0, 0}
        };
    }

    inline DeserializationResult deserialize(GameMap &map, size_t x, size_t y, 
                                            std::span<const uint8_t> data, 
                                            SemanticVersion ver) override
    { 
        if(ver.major < 2)
            return DeserializationResult::VersionTooOld;

        if(ver.major > 2 || ver.minor > 0)
            return DeserializationResult::VersionTooNew;

        std::string_view name{reinterpret_cast<const char *>(data.data()), data.size()};
        auto *ctl = m_registry->find_controller_by_name(name);
        if(ctl == nullptr)
            return DeserializationResult::MissingDependency;
        
        map.auxs(x, y) = ctl;
        return DeserializationResult::Success;
    }

    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.2f;
        map.thermal_conductivities(x, y) = 0.6f;
        map.colors(x, y) = 0x67A046FF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

    inline void pre_dynamic_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Tap was never registered in SimulationManager");
    }

    inline void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(map.material_ctls(x, y) != this)
            return;

        auto *aux = std::any_cast<MaterialController *>(&map.auxs(x, y));
        if(aux == nullptr)
        {
            for(auto [dx, dy] : g_von_neumann_deltas)
            {
                if(!map.in_bounds(x+dx, y+dy) || !MtlTag::IsMovable(map.tags(x+dx, y+dy)))
                    continue;

                map.auxs(x, y) = map.material_ctls(x+dx, y+dy);
                break;
            }

            return;
        }

        if(fastprng::propability(1, 6))
        {
            for(auto [dx, dy] : g_von_neumann_deltas)
            {
                 if(!map.in_bounds(x+dx, y+dy))
                    continue;

                if(map.tags(x+dx, y+dy).test(MtlTag::Space))
                {
                    (**aux).init_point(map, x+dx, y+dy);
                    (**aux).play_place_sound(map, x+dx, y+dy);
                }
            }
        }
        else if(fastprng::propability(1, 30))
        {
            for(auto [dx, dy] : g_moore_deltas)
            {
                if(!map.in_bounds(x+dx, y+dy))
                    continue;
                
                if(map.material_ctls(x+dx, y+dy) == this)
                    map.auxs(x+dx, y+dy) = *aux;
            }
        }
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return !map.tags(x, y).test(MtlTag::Unbreakable);
    }

private:
    MaterialRegistry *m_registry = nullptr;
};


#endif // MOOX_TAP_HPP
