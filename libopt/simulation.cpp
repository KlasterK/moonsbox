#include "simulation.hpp"
#include "drawing.hpp"
#include <ranges>
#include <algorithm>
#include <functional>


SimulationManager::SimulationManager(GameMap &map)
    : m_map(map)
{}


bool SimulationManager::register_controller(MaterialController &controller, std::string_view name)
{
    auto [_, was_inserted] = m_controllers.insert({name, &controller});
    return was_inserted;
}


MaterialController *SimulationManager::find_controller_by_name(std::string_view name)
{
    if(!m_controllers.contains(name))
        return nullptr;
    
    return m_controllers[name];
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


void _fall_sand(GameMap &map, size_t x, size_t y)
{
    if(y < 1)
        return;
    
    if(MtlTag::IsFlowable(map.tags(x, y-1)))
    {
        drawing::swap(map, x, y, x, y-1);
        return;
    }

    if(rand() > RAND_MAX / 2)
    {
        if(x > 0 && MtlTag::IsFlowable(map.tags(x-1, y-1)))
            drawing::swap(map, x, y, x-1, y-1);
    }
    else
    {
        if(x < (map.width() - 1) && MtlTag::IsFlowable(map.tags(x+1, y-1)))
            drawing::swap(map, x, y, x+1, y-1);
    }
}


void SimulationManager::tick()
{
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
        ctl->static_update();
        ctl->dynamic_update();
    }

    static_assert(MaterialPhysicalBehaviorRevision == 20260124ULL);
    std::array phys_procedures{
        +[](GameMap &, size_t, size_t) {}, // Null
        _fall_sand,                        // Sand
        +[](GameMap &, size_t, size_t) {}, // Liquid
        +[](GameMap &, size_t, size_t) {}, // LightGas
        +[](GameMap &, size_t, size_t) {}, // HeavyGas
    };

    for(size_t y{}; y < h; ++y)
    {
        for(size_t x{}; x < w; ++x)
        {
            auto idx = static_cast<size_t>(m_map.physical_behaviors(x, y));
            phys_procedures[idx](m_map, x, y);
        }
    }
}
