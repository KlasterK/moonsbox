#include "gamemap.hpp"
#include "drawing.hpp"

GameMap::GameMap(Point size, setup_dot_func_t filler)
{
    resize(size);
}

inline opt_ref<const MaterialData> GameMap::at(Point pos) const noexcept
{
    if(!bounds(pos))
        return std::nullopt;
    return m_data[pos * strides()];
}

inline opt_ref<MaterialData> GameMap::at(Point pos) noexcept
{
    if(!bounds(pos))
        return std::nullopt;
    return m_data[pos * strides()];
}

void GameMap::resize(Point new_size)
{

}
