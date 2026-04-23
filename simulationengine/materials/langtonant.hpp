#ifndef MOOX_LANGTONSANT_HPP
#define MOOX_LANGTONSANT_HPP

#include "simulationengine/core/materialdefs.hpp"
#include <limits>
#include <print>
#include <simulationengine/materials/common.hpp>

class LangtonAnt : public MaterialController
{
private:
    static constexpr auto DirectionsTable = std::to_array<std::array<int, 2>>({
        { 0,  1},
        { 1,  0},
        { 0, -1},
        {-1,  0},
    });
    static_assert(DirectionsTable.size() <= std::numeric_limits<uint8_t>::max());

    struct AntData
    {
        MaterialController &produced_material;
        uint8_t direction{};
        bool is_standing_on_black{};
        int was_updated_in_tick_num{};
    };

public:
    void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.2f;
        map.thermal_conductivities(x, y) = 0.8f;
        map.colors(x, y) = 0xBD8ABFFF;
        map.tags(x, y).reset().set(MtlTag::Bulk);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        map.material_ctls(x, y) = this;
    }

    void pre_static_update(GameMap &) override
    {
        m_space = m_registry->find_controller_by_name("Space");
        assert(m_space != nullptr);

        ++m_current_tick_num;
    }

    void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        AntData *data = std::any_cast<AntData>(&map.auxs(x, y));
        if(data == nullptr)
        {
            for(auto [dx, dy] : g_moore_deltas)
            {
                size_t nb_x = x+dx, nb_y = y+dy;
                if (
                    !map.in_bounds(nb_x, nb_y) 
                    || !map.tags(nb_x, nb_y).test(MtlTag::Solid)
                    || map.tags(nb_x, nb_y).test(MtlTag::Unbreakable)
                    || map.material_ctls(nb_x, nb_y) == this
                ) continue;

                uint8_t random_direction = fastprng::get_u8() % DirectionsTable.size();

                map.auxs(x, y) = AntData{
                    .produced_material = *map.material_ctls(nb_x, nb_y),
                    .direction = random_direction,
                    .is_standing_on_black = false,
                };
                data = std::any_cast<AntData>(&map.auxs(x, y));
                
                map.tags(x, y).reset().set(MtlTag::Solid);
                map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
            }
            return;
        }

        if(data->was_updated_in_tick_num == m_current_tick_num)
            return;
        data->was_updated_in_tick_num = m_current_tick_num;

        data->direction = (data->direction + 
                            (data->is_standing_on_black ? (DirectionsTable.size() - 1) : 1)
                          ) % DirectionsTable.size();

        size_t next_x = x + DirectionsTable[data->direction][0];
        size_t next_y = y + DirectionsTable[data->direction][1];

        if (!map.in_bounds(next_x, next_y) || map.tags(next_x, next_y).test(MtlTag::Unbreakable))
            return;

        if(map.material_ctls(next_x, next_y) == this)
        {
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
            return;
        }
        else if(map.physical_behaviors(x, y) != MaterialPhysicalBehavior::Null)
        {
            map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        }

        bool is_current_black = data->is_standing_on_black;
        MaterialController &produced_material = data->produced_material;

        data->is_standing_on_black = map.tags(next_x, next_y).test(MtlTag::Solid);
        drawing::copy(map, x, y, next_x, next_y);

        if(is_current_black)
            m_space->init_point(map, x, y);
        else
            produced_material.init_point(map, x, y);
    }

    void on_register(MaterialRegistry &r) override
    {
        m_registry = &r;
    }

private:
    MaterialRegistry *m_registry{};
    MaterialController *m_space{};
    int m_current_tick_num{};
};

#endif // MOOX_LANGTONSANT_HPP
