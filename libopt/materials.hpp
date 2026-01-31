#ifndef MOOX_MATERIALS_HPP
#define MOOX_MATERIALS_HPP

#include <algorithm>
#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include "materialdefs.hpp"
#include "simulation.hpp"


template<typename T>
requires std::is_integral_v<T> && std::is_signed_v<T>
T _map_clamp(T value, T in_min, T in_max, T out_min, T out_max)
{
    T mapped = (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return std::clamp(mapped, std::min(out_min, out_max), std::max(out_min, out_max));
}

constexpr std::array g_von_neumann_deltas{
    std::array{-1, 0}, 
    std::array{1, 0}, 
    std::array{0, -1}, 
    std::array{0, 1},
};

constexpr std::array g_moore_deltas{
    std::array{-1, 0}, 
    std::array{1, 0}, 
    std::array{0, -1}, 
    std::array{0, 1},
    std::array{-1, 1}, 
    std::array{1, 1}, 
    std::array{-1, -1}, 
    std::array{-1, 1},
};


class Space : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 1.f;
        map.colors(x, y) = 0x00000000;
        map.tags(x, y).reset().set(MtlTag::Space);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ids(x, y) = material_id();
    }
};


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
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 0.1f;
        map.tags(x, y).reset().set(MtlTag::Bulk);
        map.auxs(x, y).emplace<Aux>(false, 0x99 + rand() % (0xFF - 0x99));
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &map) override
    {
        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                auto *aux = std::any_cast<Aux>(&map.auxs(x, y));
                if(aux == nullptr)
                    continue;
                
                auto temp = static_cast<int32_t>(map.temps(x, y));

                if(aux->is_glass)
                    map.colors(x, y) = (
                        _map_clamp(temp,   400, 1973, 0x96, 0xFF) << 24
                        | _map_clamp(temp, 400, 1973, 0x94, 0x88) << 16
                        | _map_clamp(temp, 400, 1973, 0x77, 0x00) << 8
                        | _map_clamp(temp, 400, 1973, 0x55, 0x85)
                    );
                else
                    map.colors(x, y) = (
                        SandColorR << 24
                        | _map_clamp(temp, 400, 1973, 
                                     static_cast<int32_t>(aux->sand_color_g), 0x66) << 16
                        | SandColorB << 8
                        | _map_clamp(temp, 400, 1973, SandColorA, 0xAA)
                    );

                if(temp > 1973)
                {
                    if(!aux->is_glass)
                        aux->is_glass = true;
                        
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
        }
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        auto &tags = map.tags(x, y);
        return MtlTag::IsMovable(tags) || tags.test(MtlTag::Space);
    }
};


class Plus100K : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) += 100.f;
    }
};


class Minus100K : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) -= 100.f;
    }
};


class Water : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.7f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0x009599FF | rand() % (0xBB - 0x95) * 0x10000;
        map.tags(x, y).reset().set(MtlTag::Liquid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("Water was never registered in SimulationManager");

        if(m_steam == nullptr)
        {
            m_steam = m_sim->find_controller_by_name("Steam");
            if(m_steam == nullptr)
                throw std::logic_error("Water cannot not find Steam material in SimulationManager");
        }

        if(m_ice == nullptr)
        {
            m_ice = m_sim->find_controller_by_name("Ice");
            if(m_ice == nullptr)
                throw std::logic_error("Water cannot not find Ice material in SimulationManager");
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                float temp = map.temps(x, y);
                if(temp < 270.f)
                    m_ice->init_point(map, x, y);
                else if(temp > 375.f)
                    m_steam->init_point(map, x, y);
            }
        }
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return MtlTag::IsFlowable(map.tags(x, y));
    }

private:
    SimulationManager *m_sim = nullptr;
    MaterialController *m_steam = nullptr, *m_ice = nullptr;
};


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
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("Ice was never registered in SimulationManager");

        if(m_water == nullptr)
        {
            m_water = m_sim->find_controller_by_name("Water");
            if(m_water == nullptr)
                throw std::logic_error("Ice cannot not find Water material in SimulationManager");
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                if(float temp = map.temps(x, y); temp > 275.f)
                {
                    m_water->init_point(map, x, y);
                    map.temps(x, y) = temp;
                }
            }
        }
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

private:
    SimulationManager *m_sim = nullptr;
    MaterialController *m_water = nullptr;
};


class Steam : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 420.f;
        map.heat_capacities(x, y) = 0.7f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0x28BBC53D;
        map.tags(x, y).reset().set(MtlTag::Gas);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("Steam was never registered in SimulationManager");

        if(m_water == nullptr)
        {
            m_water = m_sim->find_controller_by_name("Water");
            if(m_water == nullptr)
                throw std::logic_error("Steam cannot not find Water material in SimulationManager");
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                if(float temp = map.temps(x, y); temp < 370.f)
                {
                    m_water->init_point(map, x, y);
                    map.temps(x, y) = temp;
                }
            }
        }
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return MtlTag::IsSparseness(map.tags(x, y));
    }

private:
    SimulationManager *m_sim = nullptr;
    MaterialController *m_water = nullptr;
};


class Tap : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.2f;
        map.thermal_conductivities(x, y) = 0.6f;
        map.colors(x, y) = 0x67A046FF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ids(x, y) = material_id();
    }

    inline void dynamic_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("Tap was never registered in SimulationManager");

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                auto *aux = std::any_cast<MaterialController *>(&map.auxs(x, y));
                if(aux == nullptr)
                {
                    for(auto [dx, dy] : g_von_neumann_deltas)
                    {
                        if(!map.in_bounds(x+dx, y+dy) || !MtlTag::IsMovable(map.tags(x+dx, y+dy)))
                            continue;

                        auto *ctl = m_sim->find_controller_by_id(map.material_ids(x+dx, y+dy));
                        if(ctl == nullptr)
                            continue;
                        map.auxs(x, y) = ctl;
                        break;
                    }

                    continue;
                }

                if(rand() % 6 == 0)
                {
                    for(auto [dx, dy] : g_von_neumann_deltas)
                    {
                         if(!map.in_bounds(x+dx, y+dy))
                            continue;

                        if(map.tags(x+dx, y+dy).test(MtlTag::Space))
                            (**aux).init_point(map, x+dx, y+dy);
                    }
                }
                else if(rand() % 30 == 0)
                {
                    for(auto [dx, dy] : g_moore_deltas)
                    {
                        if(!map.in_bounds(x+dx, y+dy))
                            continue;
                        
                        if(map.material_ids(x+dx, y+dy) == material_id())
                            map.auxs(x+dx, y+dy) = *aux;
                    }
                }
            }
        }
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

private:
    SimulationManager *m_sim = nullptr;
};


class UnbreakableWall : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.6f;
        map.thermal_conductivities(x, y) = 0.4f;
        map.colors(x, y) = 0xFFFFFFFF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ids(x, y) = material_id();
    }
};


class BlackHole : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.f;
        map.thermal_conductivities(x, y) = 0.f;
        map.colors(x, y) = 0x1F1F1FFF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ids(x, y) = material_id();
    }

    inline void dynamic_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("BlackHole was never registered in SimulationManager");

        if(m_space == nullptr)
        {
            m_space = m_sim->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error(
                    "BlackHole cannot not find Space material in SimulationManager"
                );
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                for(auto [dx, dy] : g_von_neumann_deltas)
                {
                    if(map.in_bounds(x+dx, y+dy) && !map.tags(x+dx, y+dy).test(MtlTag::Solid))
                        m_space->init_point(map, x+dx, y+dy);
                }
            }
        }
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

private:
    SimulationManager *m_sim = nullptr;
    MaterialController *m_space = nullptr;
};


class Propane : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 0.5f;
        map.colors(x, y) = 0x385DA345;
        map.tags(x, y).reset().set(MtlTag::Gas);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("Propane was never registered in SimulationManager");

        if(m_fire == nullptr)
        {
            m_fire = m_sim->find_controller_by_name("Fire");
            if(m_fire == nullptr)
                throw std::logic_error(
                    "Propane cannot not find Fire material in SimulationManager"
                );
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                float temp = map.temps(x, y);
                auto tags = map.tags(x, y);

                if(temp > 700.f)
                {
                    m_fire->init_point(map, x, y);
                    map.temps(x, y) = 2800.f;
                    
                    for(auto [dx, dy] : g_moore_deltas)
                    {
                        if(map.material_ids(x+dx, y+dy) != this->material_id())
                            continue;
                        
                        m_fire->init_point(map, x, y);
                        map.temps(x, y) = 2800.f;
                    }
                }
                else if(tags.test(MtlTag::Solid))
                {
                    if(temp > 85.f)
                    {
                        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
                        map.tags(x, y).reset().set(MtlTag::Liquid);
                        map.colors(x, y) = 0x5376B885;
                    }
                }
                else if(tags.test(MtlTag::Liquid))
                {
                    if(temp < 80.f)
                    {
                        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
                        map.tags(x, y).reset().set(MtlTag::Solid);
                        map.colors(x, y) = 0x6D8EC9B8;
                    }
                    else if(temp > 235.f)
                    {
                        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
                        map.tags(x, y).reset().set(MtlTag::Gas);
                        map.colors(x, y) = 0x385DA345;
                    }
                }
                else if(tags.test(MtlTag::Gas))
                {
                    if(temp < 230.f)
                    {
                        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
                        map.tags(x, y).reset().set(MtlTag::Liquid);
                        map.colors(x, y) = 0x5376B885;
                    }
                }
            }
        }
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        auto &tags = map.tags(x, y);
        return MtlTag::IsSparseness(tags);
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

private:
    SimulationManager *m_sim = nullptr;
    MaterialController *m_fire = nullptr;
};


class Fire : public MaterialController
{
private:
    static constexpr size_t   StepsCount    = 20;
    static constexpr uint32_t MinTTLColor   = 0xFF000044;
    static constexpr uint32_t MaxTTLColor   = 0xFFFF00FF;
    static constexpr uint32_t ColorStep     = (0xFF - 0x00) / StepsCount * 0x10000 
                                            + (0xFF - 0x44) / StepsCount;

public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 1000.f;
        map.heat_capacities(x, y) = 1.f;
        map.thermal_conductivities(x, y) = 1.f;
        map.colors(x, y) = MaxTTLColor - ColorStep * (rand() % StepsCount);
        map.tags(x, y).reset().set(MtlTag::Gas);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::LightGas;
        map.material_ids(x, y) = material_id();
    }

    inline void dynamic_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("Fire was never registered in SimulationManager");

        if(m_space == nullptr)
        {
            m_space = m_sim->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error(
                    "Fire cannot not find Space material in SimulationManager"
                );
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                map.colors(x, y) -= ColorStep;
                if(map.colors(x, y) <= MinTTLColor)
                    m_space->init_point(map, x, y);
            }
        }
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return map.tags(x, y).test(MtlTag::Space);
    }

private:
    SimulationManager *m_sim = nullptr;
    MaterialController *m_space = nullptr;
};


class PureGlass : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.5f;
        map.thermal_conductivities(x, y) = 0.05f;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &map) override
    {
        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                auto temp = static_cast<int32_t>(map.temps(x, y));

                map.colors(x, y) = (
                    _map_clamp(temp,   400, 1773, 0x53, 0xFF) << 24
                    | _map_clamp(temp, 400, 1773, 0xD4, 0xAA) << 16
                    | _map_clamp(temp, 400, 1773, 0x98, 0x00) << 8
                    | _map_clamp(temp, 400, 1773, 0x20, 0x85)
                );

                if(temp > 1773)
                {
                    map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Liquid;
                    map.tags(x, y).reset().set(MtlTag::Liquid);
                }
                else
                {
                    map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
                    map.tags(x, y).reset().set(MtlTag::Solid);
                }
            }
        }
    }
};


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
        map.material_ids(x, y) = material_id();
    }

    inline void static_update(GameMap &map) override
    {
        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
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
};


class Absorbent : public MaterialController
{
public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        uint32_t grayscale = 0xDD + rand() % (0xFF - 0xDD);
        uint32_t yellowness = 0x11 + rand() % (0x33 - 0x11);
        map.colors(x, y) = grayscale << 24 | grayscale << 16 
                         | (grayscale - yellowness) << 8 | 0xFF;
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.3f;
        map.thermal_conductivities(x, y) = 0.1f;
        map.tags(x, y).reset().set(MtlTag::Float);
        map.auxs(x, y).emplace<int32_t>(rand() % 200);
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Sand;
        map.material_ids(x, y) = material_id();
    }

    inline void dynamic_update(GameMap &map) override
    {
        if(m_sim == nullptr)
            throw std::logic_error("Absorbent was never registered in SimulationManager");

        if(m_space == nullptr)
        {
            m_space = m_sim->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error(
                    "Absorbent cannot not find Space material in SimulationManager"
                );
        }

        for(size_t y{}; y < map.height(); ++y)
        {
            for(size_t x{}; x < map.width(); ++x)
            {
                if(map.material_ids(x, y) != this->material_id())
                    continue;
                
                auto *ttl = std::any_cast<int32_t>(&map.auxs(x, y));
                if(ttl == nullptr)
                    continue;

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
        }
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        auto &tags = map.tags(x, y);
        return MtlTag::IsMovable(tags) || tags.test(MtlTag::Space);
    }

    inline void on_register(SimulationManager &sim) override
    {
        m_sim = &sim;
    }

private:
    SimulationManager *m_sim = nullptr;
    MaterialController *m_space = nullptr;
};


#endif // MOOX_MATERIALS_HPP
