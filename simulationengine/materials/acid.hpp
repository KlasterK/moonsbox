#ifndef MOOX_ACID_HPP
#define MOOX_ACID_HPP

#include "simulationengine/algorithms/fastprng.hpp"
#include "simulationengine/core/materialdefs.hpp"
#include <cstdint>
#include <simulationengine/materials/common.hpp>

class Acid : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.7f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0xFFFF00AA;
        map.tags(x, y).reset().set(MtlTag::Liquid);
        map.auxs(x, y).emplace<uint32_t>(1 + fastprng::get_u32() % 5);
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
        map.material_ctls(x, y) = this;
    }

    inline void pre_static_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Acid::pre_static_update: instance was never registered");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error("Acid::pre_static_update: cannot find Space material controller");
        }

        if(m_fox_tail == nullptr)
        {
            m_fox_tail = m_registry->find_controller_by_name("--fox-tail-NO2");
            if(m_fox_tail == nullptr)
                throw std::logic_error("Acid::pre_static_update: cannot find --fox-tail-NO2 material controller");
        }
    }

    inline void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        using namespace array_element_wise_operators;

        if(map.temps(x, y) > 375.f)
        {
            m_fox_tail->init_point(map, x, y);
            return;
        }

        auto [nb_x, nb_y] = std::array{x, y} + *fastprng::choice(g_moore_deltas);
        if(!map.in_bounds(nb_x, nb_y))
            return;

        const auto &nb_tags = map.tags(nb_x, nb_y);

        if (map.material_ctls(nb_x, nb_y) != this 
            && !nb_tags.test(MtlTag::Unbreakable) 
            && !MtlTag::IsSparseness(nb_tags))
        {
            map.temps(nb_x, nb_y) += 50.f;
            m_fox_tail->init_point(map, nb_x, nb_y);

            auto &ttl = std::any_cast<uint32_t &>(map.auxs(x, y));
            if(ttl == 0)
                m_space->init_point(map, x, y);
            else
                --ttl; 
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

private:
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_space = nullptr, *m_fox_tail = nullptr;
};


namespace hidden_materials
{
    class FoxTailNO2 : public MaterialController
    {
    public:
        inline void init_point(GameMap &map, size_t x, size_t y) override
        {
            map.colors(x, y) = uint32_t(
                _map_clamp(int32_t(fastprng::get_u8()),   0, 255, 0xAA, 0xFF) << 24
                | _map_clamp(int32_t(fastprng::get_u8()), 0, 255, 0x88, 0xAA) << 16
                | _map_clamp(int32_t(fastprng::get_u8()), 0, 255, 0x00, 0x33) << 8
                | 0xCC
            );
            map.heat_capacities(x, y) = 0.3f;
            map.thermal_conductivities(x, y) = 0.5f;
            map.tags(x, y).reset().set(MtlTag::Gas);
            map.auxs(x, y).reset();
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::HeavyGas;
            map.material_ctls(x, y) = this;
        }

        inline void pre_static_update(GameMap &) override
        {
            if(m_registry == nullptr)
                throw std::logic_error("hidden_materials::FoxTailNO2::pre_static_update: "
                                       "instance was never registered");

            if(m_space == nullptr)
            {
                m_space = m_registry->find_controller_by_name("Space");
                if(m_space == nullptr)
                    throw std::logic_error("hidden_materials::FoxTailNO2::pre_static_update: "
                                           "cannot find Space material controller");
            }
        }

        inline void on_register(MaterialRegistry &registry) override
        {
            m_registry = &registry;
        }

    private:
        MaterialRegistry *m_registry = nullptr;
        MaterialController *m_space = nullptr;
    };
}


#endif // MOOX_ACID_HPP
