#include "simulation.hpp"
#include "drawing.hpp"
#include <ranges>
#include <algorithm>
#include <functional>


SimulationManager::SimulationManager(GameMap &map)
    : m_map(map)
{}


bool SimulationManager::register_controller(MaterialController &controller, const std::string &name)
{
    auto [_, was_inserted] = m_controllers.insert({name, &controller});
    if(was_inserted)
        controller.on_register(*this);
    return was_inserted;
}


MaterialController *SimulationManager::find_controller_by_name(const std::string &name)
{
    auto it = m_controllers.find(name);
    if(it == m_controllers.end())
        return nullptr;
    
    return it->second;
}


float _calculate_delta_temp(const GameMap &map, size_t x, size_t y, size_t nx, size_t ny)
{
    return (
        (map.temps(nx, ny) - map.temps(x, y))
        * std::min(
            map.thermal_conductivities(nx, ny),
            map.thermal_conductivities(x, y)
        ) 
        * (1 - map.heat_capacities(x, y))
    );
}


bool _try_swap(GameMap &map, size_t src_x, size_t src_y, size_t dst_x, size_t dst_y, 
               bool (*test)(const MaterialTags &))
{
    if(!map.in_bounds(dst_x, dst_y))
        return false;

    if(test(map.tags(dst_x, dst_y)))
    {
        drawing::swap(map, src_x, src_y, dst_x, dst_y);
        return true;
    }
    return false;
}


void _diffuse(GameMap &map, size_t x, size_t y, bool (*test)(const MaterialTags &))
{
    auto positions = std::to_array<std::array<size_t, 2>>({
        {x-1, y}, {x+1, y}, {x, y-1}, {x, y+1}
    });

    auto nb_pos = positions[rand() % positions.size()];
    if(!map.in_bounds(nb_pos[0], nb_pos[1]))
        return;
    
    if(test(map.tags(nb_pos[0], nb_pos[1])))
        drawing::swap(map, x, y, nb_pos[0], nb_pos[1]);
}


void _fall_sand(GameMap &map, size_t x, size_t y)
{
    if(_try_swap(map, x, y, x, y-1, MtlTag::IsFlowable))
        return;

    if(rand() > RAND_MAX / 2)
    {
        if(_try_swap(map, x, y, x-1, y-1, MtlTag::IsFlowable))
            return;
    }
    else if(_try_swap(map, x, y, x+1, y-1, MtlTag::IsFlowable))
        return;
}


void _fall_liquid(GameMap &map, size_t x, size_t y)
{
    if(_try_swap(map, x, y, x, y-1, MtlTag::IsFlowable))
        return;

    if(rand() > RAND_MAX / 2)
    {
        if(_try_swap(map, x, y, x-1, y, MtlTag::IsFlowable))
            return;
    }
    else if(_try_swap(map, x, y, x+1, y, MtlTag::IsFlowable))
        return;

    if(rand() > RAND_MAX / 2)
    {
        if(_try_swap(map, x, y, x-1, y-1, MtlTag::IsFlowable))
            return;
    }
    else if(_try_swap(map, x, y, x+1, y-1, MtlTag::IsFlowable))
        return;

    if(rand() > RAND_MAX / 100)
        _diffuse(map, x, y, [](const MaterialTags &v){ return v.test(MtlTag::Liquid); });
}

bool _test_space(const MaterialTags &v) { return v.test(MtlTag::Space); }
bool _test_gas(const MaterialTags &v) { return v.test(MtlTag::Gas); }

void _fall_light_gas(GameMap &map, size_t x, size_t y)
{
    if(_try_swap(map, x, y, x, y+1, _test_space))
        return;

    if(rand() > RAND_MAX / 2)
    {
        if(_try_swap(map, x, y, x-1, y, _test_space))
            return;
    }
    else if(_try_swap(map, x, y, x+1, y, _test_space))
        return;

    if(rand() > RAND_MAX / 2)
    {
        if(_try_swap(map, x, y, x-1, y+1, _test_space))
            return;
    }
    else if(_try_swap(map, x, y, x+1, y+1, _test_space))
        return;

    if(rand() > RAND_MAX / 100)
        _diffuse(map, x, y, _test_gas);
}


void _fall_heavy_gas(GameMap &map, size_t x, size_t y)
{
    if (
        _try_swap(map,    x, y, x,     y + 1, _test_space)
        || _try_swap(map, x, y, x + 1, y,     _test_space)
        || _try_swap(map, x, y, x - 1, y,     _test_space)
        || _try_swap(map, x, y, x,     y - 1, _test_space)
        || _try_swap(map, x, y, x + 1, y + 1, _test_space)
        || _try_swap(map, x, y, x + 1, y - 1, _test_space)
        || _try_swap(map, x, y, x - 1, y + 1, _test_space)
        || _try_swap(map, x, y, x - 1, y - 1, _test_space)
    ) return;

    if(rand() > RAND_MAX / 100)
        _diffuse(map, x, y, _test_gas);
}


void SimulationManager::tick()
{
    if(m_is_paused)
    {
        for(auto *ctl : std::views::values(m_controllers))
            ctl->static_update(m_map);
        return;
    }
    
    size_t w = m_map.width();
    size_t h = m_map.height();
    float temp_delta{};

    for(size_t y{}; y < h; ++y)
    {
        for(size_t x{}; x < w; ++x)
        {
            temp_delta = 0.f;
            
            if(x > 0)
                temp_delta += _calculate_delta_temp(m_map, x, y, x-1, y);
            if(x < w-1)
                temp_delta += _calculate_delta_temp(m_map, x, y, x+1, y);
            if(y > 0)
                temp_delta += _calculate_delta_temp(m_map, x, y, x, y-1);
            if(y < h-1)
                temp_delta += _calculate_delta_temp(m_map, x, y, x, y+1);
            
            m_map.temps(x, y) += temp_delta / 4.f;

            if(m_map.temps(x, y) < 0.f)
                m_map.temps(x, y) = 0.f;
        }
    }

    auto values_view = std::views::values(m_controllers);
    for(auto *ctl : values_view)
    {
        ctl->static_update(m_map);
        ctl->dynamic_update(m_map);
    }

    static_assert(MaterialPhysicalBehaviorRevision == 20260124ULL);
    std::array phys_procedures{
        +[](GameMap &, size_t, size_t) {}, // Null
        _fall_sand,
        _fall_liquid,
        +[](GameMap &, size_t, size_t) {}, // LightGas
        _fall_heavy_gas,
    };

    for(size_t y{}; y < h; ++y)
    {
        for(size_t x{}; x < w; ++x)
        {
            auto idx = static_cast<size_t>(m_map.physical_behaviors(x, y));
            phys_procedures[idx](m_map, x, y);
        }
    }

    for(size_t y = h - 1; y < h; --y) // when y=0 decreases, it becomes SIZE_MAX and greater than h
    {
        for(size_t x{}; x < w; ++x)
        {
            if(m_map.physical_behaviors(x, y) == MaterialPhysicalBehavior::LightGas)
                _fall_light_gas(m_map, x, y);
        }
    }
}
