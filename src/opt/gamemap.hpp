#ifndef MOOX_GAMEMAP_HPP
#define MOOX_GAMEMAP_HPP

#include <memory>
#include <any>
#include "materialdefs.hpp"
#include <cstdint>
#include <span>


class GameMap
{
public:
    GameMap(size_t width, size_t height);

    inline size_t width()     const { return m_width; }
    inline size_t height()    const { return m_height; }
    inline size_t flat_size() const { return m_width * m_height; }

    inline size_t point_to_idx(size_t x, size_t y) const { return y * m_width + x; }
    inline std::pair<size_t, size_t> strides()     const { return {1, m_width}; }

    inline bool in_bounds(size_t x, size_t y) const { return x < m_width && y < m_height; }

    DotProxy make_proxy(size_t x, size_t y);
    const DotProxy make_proxy(size_t x, size_t y) const;

    std::span<float> temp_span();
    std::span<const float> temp_span() const;
    float& temp(size_t x, size_t y);
    const float& temp(size_t x, size_t y) const;

    std::span<float> heat_capacity_span();
    std::span<const float> heat_capacity_span() const;
    float& heat_capacity(size_t x, size_t y);
    const float& heat_capacity(size_t x, size_t y) const;

    std::span<float> thermal_conductivity_span();
    std::span<const float> thermal_conductivity_span() const;
    float& thermal_conductivity(size_t x, size_t y);
    const float& thermal_conductivity(size_t x, size_t y) const;

    std::span<uint32_t> color_span();
    std::span<const uint32_t> color_span() const;
    uint32_t& color(size_t x, size_t y);
    const uint32_t& color(size_t x, size_t y) const;

    std::span<MaterialTags> tag_span();
    std::span<const MaterialTags> tag_span() const;
    MaterialTags& tag(size_t x, size_t y);
    const MaterialTags& tag(size_t x, size_t y) const;

    std::span<MaterialPhysicalBehavior> physical_behavior_span();
    std::span<const MaterialPhysicalBehavior> physical_behavior_span() const;
    MaterialPhysicalBehavior& physical_behavior(size_t x, size_t y);
    const MaterialPhysicalBehavior& physical_behavior(size_t x, size_t y) const;

    std::span<std::any> aux_span();
    std::span<const std::any> aux_span() const;
    std::any& aux(size_t x, size_t y);
    const std::any& aux(size_t x, size_t y) const;

    std::span<MaterialID> material_id_span();
    std::span<const MaterialID> material_id_span() const;
    MaterialID& material_id(size_t x, size_t y);
    const MaterialID& material_id(size_t x, size_t y) const;

private:
    size_t m_width, m_height;
    std::unique_ptr<float[]> m_temps, m_heat_capacities, m_thermal_conductivities;
    std::unique_ptr<uint32_t[]> m_colors;
    std::unique_ptr<MaterialTags[]> m_tags;
    std::unique_ptr<MaterialPhysicalBehavior[]> m_physical_behaviors;
    std::unique_ptr<std::any[]> m_auxs;
    std::unique_ptr<MaterialID[]> m_material_ids;
};

#endif // MOOX_GAMEMAP_HPP
