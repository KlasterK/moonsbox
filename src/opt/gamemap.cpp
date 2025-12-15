#include "gamemap.hpp"

#define MOOX_IMPLEMENT_FIELD(type, fn_at, fn_span, member_data) \
    std::span<type> GameMap::fn_span() \
    { \
        return {member_data.get(), flat_size()}; \
    } \
    std::span<const type> GameMap::fn_span() const \
    { \
        return {member_data.get(), flat_size()}; \
    } \
    type& GameMap::fn_at(size_t x, size_t y) \
    { \
        return member_data[point_to_idx(x, y)]; \
    } \
    const type& GameMap::fn_at(size_t x, size_t y) const \
    { \
        return member_data[point_to_idx(x, y)]; \
    } \
// MOOX_IMPLEMENT_FIELD

#define MOOX_IMPLFD_FROM_NAME(name) \
    MOOX_IMPLEMENT_FIELD( \
        std::remove_cvref_t<decltype(GameMap::m_##name##s[0])>, \
        name, \
        name##_span, \
        m_##name##s \
    ) \
// MOOX_IMPLFD_FROM_NAME

#define MOOX_IMPLFD_FROM_NAME_MEMBER_NAME(name, member_name) \
    MOOX_IMPLEMENT_FIELD( \
        std::remove_cvref_t<decltype(GameMap::member_name[0])>, \
        name, \
        name##_span, \
        member_name \
    ) \
// MOOX_IMPLFD_FROM_NAME_MEMBER_NAME

MOOX_IMPLFD_FROM_NAME(temp)
MOOX_IMPLFD_FROM_NAME_MEMBER_NAME(heat_capacity, m_heat_capacities)
MOOX_IMPLFD_FROM_NAME_MEMBER_NAME(thermal_conductivity, m_thermal_conductivities)
MOOX_IMPLFD_FROM_NAME(color)
MOOX_IMPLFD_FROM_NAME(tag)
MOOX_IMPLFD_FROM_NAME(physical_behavior)
MOOX_IMPLFD_FROM_NAME(aux)
MOOX_IMPLFD_FROM_NAME(material_id)

#define MOOX_CONST_NONCONST(nonconst_t, const_t, signature, code) \
    nonconst_t GameMap::signature code \
    const_t GameMap::signature const code \
// MOOX_CONST_NONCONST

MOOX_CONST_NONCONST(DotProxy, const DotProxy, make_proxy(size_t x, size_t y), 
{
    size_t idx = point_to_idx(x, y);
    return DotProxy(
        m_temps[idx],
        m_heat_capacities[idx],
        m_thermal_conductivities[idx],
        m_colors[idx],
        m_tags[idx],
        m_physical_behaviors[idx],
        m_auxs[idx],
        m_material_ids[idx]
    );
})
