#ifndef MOOX_MATERIALDEFS_HPP
#define MOOX_MATERIALDEFS_HPP

#include <bitset>
#include <any>
#include <cstdint>

namespace MtlTag
{
    constexpr size_t Solid  = 0;
    constexpr size_t Bulk   = 1;
    constexpr size_t Liquid = 2;
    constexpr size_t Gas    = 3;
    constexpr size_t Space  = 4;
    constexpr size_t Float  = 5;
    constexpr size_t N      = 6;
}
using MaterialTags = std::bitset<MtlTag::N>;

enum class MaterialPhysicalBehavior
{
    Null, Sand, Liquid, LightGas, HeavyGas
};
constexpr auto MaterialPhysicalBehaviorRevision = 20260124ULL;

using MaterialID = uintptr_t;

namespace MtlTag
{
    constexpr bool IsSparseness(const MaterialTags &tags) 
        { return tags.test(MtlTag::Gas) || tags.test(MtlTag::Space); }

    constexpr bool IsFlowable(const MaterialTags &tags) 
        { return IsSparseness(tags) || tags.test(MtlTag::Liquid); }

    constexpr bool IsMovable(const MaterialTags &tags) 
        { return tags.test(MtlTag::Bulk) || tags.test(MtlTag::Liquid)
              || tags.test(MtlTag::Gas)  || tags.test(MtlTag::Float); }
}

#endif // MOOX_MATERIALDEFS_HPP
