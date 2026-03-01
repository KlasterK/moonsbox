#include "gamemap.hpp"
#include <SDL2/SDL_surface.h>
#include <SDL2pp/Exception.hh>

_Layer<_SDLColorLayerTag>::_Layer(size_t width, size_t height)
    : m_surface([=]
    {
        auto *surf = SDL_CreateRGBSurfaceWithFormat(
            0, int(width), int(height), 32, SDL_PIXELFORMAT_RGBA8888
        );
        if(surf == nullptr)
            throw SDL2pp::Exception("SDL_CreateRGBSurfaceWithFormat");
        return surf;
    }())
{
    assert(!SDL_HasSurfaceRLE(m_surface.Get()));
}

uint32_t &_Layer<_SDLColorLayerTag>::operator()(size_t x, size_t y)
{
    size_t stride = size_t(m_surface.Get()->pitch) / sizeof(uint32_t);
    return static_cast<uint32_t *>(m_surface.Get()->pixels)[y * stride + x];
}

const uint32_t &_Layer<_SDLColorLayerTag>::operator()(size_t x, size_t y) const
{
    size_t stride = size_t(m_surface.Get()->pitch) / sizeof(uint32_t);
    return static_cast<const uint32_t *>(m_surface.Get()->pixels)[y * stride + x];
}

SDL2pp::Surface &_Layer<_SDLColorLayerTag>::surface()
{
    return m_surface;
}

const SDL2pp::Surface &_Layer<_SDLColorLayerTag>::surface() const
{
    return m_surface;
}

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
template class _Layer<_SDLColorLayerTag>;
template class _Layer<MaterialTags>;
template class _Layer<MaterialPhysicalBehavior>;
template class _Layer<std::any>;
template class _Layer<MaterialController *>;
