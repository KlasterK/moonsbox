#ifndef MOOX_MATERIALS_HPP
#define MOOX_MATERIALS_HPP

#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include "simulation.hpp"
#include <iostream>


template<typename T>
requires std::is_integral_v<T> && std::is_signed_v<T>
T _map_clamp(T value, T in_min, T in_max, T out_min, T out_max)
{
    T mapped = (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return std::clamp(mapped, std::min(out_min, out_max), std::max(out_min, out_max));
}


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
                        _map_clamp(temp, 400, 1973, 0x96, 0xFF)   << 24
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


#endif // MOOX_MATERIALS_HPP
