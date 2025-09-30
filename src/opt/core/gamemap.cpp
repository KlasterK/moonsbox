#include "gamemap.hpp"
#include "drawing.hpp"

GameMap::GameMap(Point size, setup_dot_func_t filler)
    : m_default_factory(filler)
{
    resize(size);
}

inline opt_ref<const MaterialData> GameMap::at(Point pos) const noexcept
{
    if(!bounds(pos))
        return std::nullopt;
    return m_data[pos.x * m_size.y + pos.y];
}

inline opt_ref<MaterialData> GameMap::at(Point pos) noexcept
{
    if(!bounds(pos))
        return std::nullopt;
    return m_data[pos.x * m_size.y + pos.y];
}

void GameMap::resize(Point new_size)
{
    m_size = new_size;
    m_data = std::make_unique<MaterialData[]>(new_size.x * new_size.y);
    drawing::fill(*this, m_default_factory);
}
