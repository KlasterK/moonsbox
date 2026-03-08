#include "gamemap.hpp"

GameMap::GameMap(size_t width, size_t height)
    : temps(width, height)
    , heat_capacities(width, height)
    , thermal_conductivities(width, height)
    , colors(width, height)
    , tags(width, height)
    , physical_behaviors(width, height)
    , auxs(width, height)
    , material_ctls(width, height)
    , m_width(width)
    , m_height(height)
{}

template class _Layer<float>;
template class _Layer<uint32_t>;
template class _Layer<MaterialTags>;
template class _Layer<MaterialPhysicalBehavior>;
template class _Layer<std::any>;
template class _Layer<MaterialController *>;
