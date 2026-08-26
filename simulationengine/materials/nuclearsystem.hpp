#ifndef MOOX_NUCLEARSYSTEM_HPP
#define MOOX_NUCLEARSYSTEM_HPP

#include <simulationengine/algorithms/fastprng.hpp>
#include <simulationengine/core/materialdefs.hpp>
#include <cstdint>
#include <simulationengine/materials/common.hpp>                       
 
struct NeutronData
{
    drawing::DotCopy underlying_dot;
    std::array<int, 2> direction;
    bool tick_parity{};
};

class Uranium : public MaterialController
{
public:
    void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.7f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0x33FF99FF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

    void pre_static_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Uranium::pre_static_update: instance was never registered");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error("Uranium::pre_static_update: cannot find Space material controller");
        }

        if(m_mtl_neutron == nullptr)
        {
            m_mtl_neutron = m_registry->find_controller_by_name("--nuclear-neutron");
            if(m_mtl_neutron == nullptr)
                throw std::logic_error("Uranium::pre_static_update: cannot find --nuclear-neutron material controller");
        }
    }

    void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        using namespace array_element_wise_operators;

        if(map.temps(x, y) > 500.f)
        {
            m_mtl_neutron->init_point(map, x, y);
            return;
        }

        // auto [nb_x, nb_y] = std::array{x, y} + *fastprng::choice(g_moore_deltas);
        // if(!map.in_bounds(nb_x, nb_y))
        //     return;

        // const auto &nb_tags = map.tags(nb_x, nb_y);

        // if (map.material_ctls(nb_x, nb_y) != this
        //     && !nb_tags.test(MtlTag::Unbreakable)
        //     && !MtlTag::IsSparseness(nb_tags))
        // {
        //     map.temps(nb_x, nb_y) += 700.f;
        //     m_mtl_neutron->init_point(map, nb_x, nb_y);

        //     auto &ttl = std::any_cast<uint32_t &>(map.auxs(x, y));
        //     if(ttl == 0)
        //         m_space->init_point(map, x, y);
        //     else
        //         --ttl;
        // }
    }

    void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

    bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return MtlTag::IsFlowable(map.tags(x, y));
    }

private:
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_space = nullptr, *m_mtl_neutron = nullptr;
};



namespace hidden_materials
{



class NuclearNeutron : public MaterialController
{
private:
    void partial_init_point(GameMap &map, size_t x, size_t y)
    {
        map.colors(x, y) = uint32_t(
            _map_clamp(int32_t(fastprng::get_u8()),   0, 255, 0xAA, 0xFF) << 24
            | _map_clamp(int32_t(fastprng::get_u8()), 0, 255, 0x88, 0xAA) << 16
            | _map_clamp(int32_t(fastprng::get_u8()), 0, 255, 0x00, 0x33) << 8
            | 0xCC
        );
        map.heat_capacities(x, y) = 0.9f;
        map.thermal_conductivities(x, y) = 0.5f;
        map.tags(x, y).reset().set(MtlTag::Solid).set(MtlTag::Unbreakable).set(MtlTag::NonCopyable);
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

public:
    void init_point(GameMap &map, size_t x, size_t y) override
    {
        auto underlying_dot = drawing::DotCopy::clone(map, x, y);
        partial_init_point(map, x, y);
        map.auxs(x, y) = NeutronData{
            .underlying_dot = std::move(underlying_dot),
            .direction      = *fastprng::choice(g_moore_deltas),
            .tick_parity    = m_current_tick_parity,
        };
    }

    void pre_static_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("hidden_materials::NuclearNeutron::pre_static_update: "
                                   "instance was never registered");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error("hidden_materials::NuclearNeutron::pre_static_update: "
                                       "cannot find Space material controller");
        }

        m_current_tick_parity = !m_current_tick_parity;
    }

    void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        using namespace array_element_wise_operators;

        auto data = std::any_cast<const NeutronData &>(map.auxs(x, y));
        if(data.tick_parity == m_current_tick_parity)
            return;

        data.tick_parity = m_current_tick_parity;

        float my_temp = map.temps(x, y);
        data.underlying_dot.restore(map, x, y);
        map.temps(x, y) = my_temp;

        auto [nb_x, nb_y] = std::array{x, y} + data.direction;
        if(map.in_bounds(nb_x, nb_y))
        {
            if(map.tags(nb_x, nb_y).test(MtlTag::NonCopyable))
                m_space->init_point(map, nb_x, nb_y);

            auto underlying_dot = drawing::DotCopy::clone(map, nb_x, nb_y);
            partial_init_point(map, nb_x, nb_y);
            map.temps(nb_x, nb_y) = my_temp;
            map.auxs(nb_x, nb_y) = NeutronData{
                .underlying_dot = std::move(underlying_dot),
                .direction      = data.direction,
                .tick_parity    = data.tick_parity,
            };
        }
    }

    void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

private:
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_space = nullptr;
    bool m_current_tick_parity{};
};

} // namespace hidden_materials

#endif // MOOX_NUCLEARSYSTEM_HPP
