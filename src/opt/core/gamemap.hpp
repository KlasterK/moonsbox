#ifndef KK_OPT_GAMEMAP_HPP
#define KK_OPT_GAMEMAP_HPP

#include <vector>
#include <iterator>
#include <modapi.h>
#include "util.hpp"

constexpr Point operator * (Point a, Point b) { return {a.x * b.x, a.y * b.y}; }

class GameMap
{
public:
    GameMap(Point size, setup_dot_func_t factory);

    inline opt_ref<const MaterialData> at(Point pos) const noexcept;

    inline opt_ref<MaterialData> at(Point pos) noexcept;

    constexpr Point get_size() noexcept { return m_size; }

    constexpr Point strides() const noexcept { return {m_size.y, 1}; }

    constexpr bool bounds(Point pos) const noexcept { return pos.x >= 0 && pos.x < m_size.x
                                                      && pos.y >= 0 && pos.y < m_size.y; }

    void resize(Point new_size);

private:
    std::unique_ptr<MaterialData[]> m_data;
    Point m_size;
    setup_dot_func_t m_default_factory;
};

#endif // include guard
