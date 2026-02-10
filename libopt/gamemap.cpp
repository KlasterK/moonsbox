#include "gamemap.hpp"
#include <format>
#include <SDL2/SDL.h>

_Layer<_SDLColorLayerTag>::_Layer(size_t width, size_t height)
    : m_surface(SDL_CreateRGBSurfaceWithFormat(
          0, width, height, 32, SDL_PIXELFORMAT_RGBA8888
      ))
    , m_width(width)
    , m_height(height)
{
    if(m_surface == nullptr)
        throw std::runtime_error(std::format(
            "GameMap::GameMap: SDL_CreateRGBSurfaceWithFormat failed: {}", 
            SDL_GetError()
        ));
}

_Layer<_SDLColorLayerTag>::_Layer(_Layer &&other)
{
    m_width = other.m_width;
    m_height = other.m_height;
    m_surface = other.m_surface;
    other.m_surface = nullptr;
}

_Layer<_SDLColorLayerTag> &_Layer<_SDLColorLayerTag>::operator=(_Layer<_SDLColorLayerTag> &&other)
{
    m_width = other.m_width;
    m_height = other.m_height;
    m_surface = other.m_surface;
    other.m_surface = nullptr;
    return *this;
}

_Layer<_SDLColorLayerTag>::~_Layer()
{
    if(m_surface)
        SDL_FreeSurface(m_surface);
}

uint32_t &_Layer<_SDLColorLayerTag>::operator()(size_t x, size_t y)
{
    if(m_surface == nullptr)
        throw std::runtime_error("_Layer<_SDLColorLayerTag>::operator(): m_surface == nullptr");

    size_t stride = m_surface->pitch / sizeof(uint32_t);
    return static_cast<uint32_t *>(m_surface->pixels)[y * stride + x];
}

const uint32_t &_Layer<_SDLColorLayerTag>::operator()(size_t x, size_t y) const
{
    if(m_surface == nullptr)
        throw std::runtime_error("_Layer<_SDLColorLayerTag>::operator(): m_surface == nullptr");

    size_t stride = m_surface->pitch / sizeof(uint32_t);
    return static_cast<const uint32_t *>(m_surface->pixels)[y * stride + x];
}

SDL_Surface &_Layer<_SDLColorLayerTag>::surface()
{
    if(m_surface == nullptr)
        throw std::runtime_error("_Layer<_SDLColorLayerTag>::surface: m_surface == nullptr");

    return *m_surface;
}

const SDL_Surface &_Layer<_SDLColorLayerTag>::surface() const
{
    if(m_surface == nullptr)
        throw std::runtime_error("_Layer<_SDLColorLayerTag>::surface: m_surface == nullptr");

    return *m_surface;
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
