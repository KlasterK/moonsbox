#ifndef MOOX_GAMEMAP_HPP
#define MOOX_GAMEMAP_HPP

#include <memory>
#include <any>
#include <cassert>
#include <cstdint>
#include <span>
#include <SDL2pp/SDL2pp.hh>
#include "materialdefs.hpp"


template<typename T>
class _Layer
{
public:
    _Layer(size_t width, size_t height)
        : m_data(std::make_unique<T[]>(width * height)) 
        , m_width(width)
        , m_height(height)
    {}
    _Layer(_Layer &&) = default;
    _Layer(const _Layer &) = delete;
    _Layer &operator=(_Layer &&) = default;
    _Layer &operator=(const _Layer &) = default;
    
    T& operator()(size_t x, size_t y)
    {
        assert(x < m_width && y < m_height);
        return m_data[y * m_width + x];
    }
    const T& operator()(size_t x, size_t y) const { return m_data[y * m_width + x]; }

    std::span<T> span()
    { 
        return {m_data.get(), m_width * m_height};
    }
    std::span<const T> span() const { return {m_data.get(), m_width * m_height}; }

private:
    std::unique_ptr<T[]> m_data;
    size_t m_width{}, m_height{};
};


struct _SDLColorLayerTag {};

template<>
class _Layer<_SDLColorLayerTag>
{
public:
    _Layer(size_t width, size_t height);

    uint32_t& operator()(size_t x, size_t y);
    const uint32_t& operator()(size_t x, size_t y) const;

    SDL2pp::Surface& surface();
    const SDL2pp::Surface& surface() const;

private:
    SDL2pp::Surface m_surface;
};

class GameMap
{
public:
    GameMap(size_t width, size_t height);

    inline size_t width()     const { return m_width; }
    inline size_t height()    const { return m_height; }
    inline size_t flat_size() const { return m_width * m_height; }

    inline size_t point_to_idx(size_t x, size_t y) const { return y * m_width + x; }
    inline std::array<size_t, 2> strides()         const { return {1, m_width}; }

    inline bool in_bounds(size_t x, size_t y) const { return x < m_width && y < m_height; }

    _Layer<float> temps;
    _Layer<float> heat_capacities;
    _Layer<float> thermal_conductivities;
    _Layer<_SDLColorLayerTag> colors;
    _Layer<MaterialTags> tags;
    _Layer<MaterialPhysicalBehavior> physical_behaviors;
    _Layer<std::any> auxs;
    _Layer<class MaterialController *> material_ctls;

private:
    size_t m_width{}, m_height{};
};

extern template class _Layer<float>;
extern template class _Layer<_SDLColorLayerTag>;
extern template class _Layer<MaterialTags>;
extern template class _Layer<MaterialPhysicalBehavior>;
extern template class _Layer<std::any>;
extern template class _Layer<class MaterialController *>;

#endif // MOOX_GAMEMAP_HPP
