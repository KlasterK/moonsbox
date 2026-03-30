#ifndef MOOX_COMMON_HPP
#define MOOX_COMMON_HPP

#include <algorithm>
#include <array>
#include <type_traits>
#include <simulationengine/core/gamemap.hpp>
#include <simulationengine/core/materialcontroller.hpp>
#include <simulationengine/core/materialdefs.hpp>
#include <simulationengine/core/materialregistry.hpp>
#include <simulationengine/algorithms/fastprng.hpp>
#include <simulationengine/algorithms/drawing.hpp>


template<typename T>
requires std::is_integral_v<T> && std::is_signed_v<T>
T _map_clamp(T value, T in_min, T in_max, T out_min, T out_max)
{
    T mapped = (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return std::clamp(mapped, std::min(out_min, out_max), std::max(out_min, out_max));
}

constexpr std::array g_von_neumann_deltas{
    std::array{ 0,  1}, 
    std::array{ 0, -1}, 
    std::array{ 1,  0}, 
    std::array{-1,  0},
};

constexpr std::array g_moore_deltas{
    std::array{ 0,  1},
    std::array{ 0, -1},
    std::array{ 1,  0},
    std::array{ 1,  1},
    std::array{ 1, -1},
    std::array{-1,  0},
    std::array{-1,  1},
    std::array{-1, -1},
};

#endif // MOOX_COMMON_HPP
