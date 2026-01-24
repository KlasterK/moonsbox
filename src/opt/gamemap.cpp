#include "gamemap.hpp"
#include <format>
#include <SDL2/SDL.h>

auto &_makeSurface(size_t width, size_t height)
{
    auto *surf = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_RGBA8888
    );

    if(surf == nullptr)
    {
        throw std::runtime_error(std::format(
            "GameMap::GameMap: SDL_CreateRGBSurfaceWithFormat failed: {}", 
            SDL_GetError()
        ));
    }

    return *surf;
}

_Layer<_SDLColorLayerTag>::_Layer(size_t width, size_t height)
    : m_surface(_makeSurface(width, height))
    , m_width(width)
    , m_height(height)
{

}

_Layer<_SDLColorLayerTag>::~_Layer() 
{
    SDL_FreeSurface(&m_surface);
}

uint32_t &_Layer<_SDLColorLayerTag>::operator()(size_t x, size_t y)
{
    size_t stride = m_surface.pitch / sizeof(uint32_t);
    return static_cast<uint32_t *>(m_surface.pixels)[y * stride + x];
}

const uint32_t &_Layer<_SDLColorLayerTag>::operator()(size_t x, size_t y) const
{
    size_t stride = m_surface.pitch / sizeof(uint32_t);
    return static_cast<const uint32_t *>(m_surface.pixels)[y * stride + x];
}

SDL_Surface &_Layer<_SDLColorLayerTag>::surface()
{
    return m_surface;
}

const SDL_Surface &_Layer<_SDLColorLayerTag>::surface() const
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
    , material_ids(width, height)
    , m_width(width)
    , m_height(height)
{}

DotProxy GameMap::make_proxy(size_t x, size_t y)
{
    return DotProxy{
        .temp = temps(x, y),
        .heat_capacity = heat_capacities(x, y),
        .thermal_conductivity = thermal_conductivities(x, y),
        .color = colors(x, y),
        .tags = tags(x, y),
        .physical_behavior = physical_behaviors(x, y),
        .aux = auxs(x, y),
        .id = material_ids(x, y)
    };
}

ConstDotProxy GameMap::make_proxy(size_t x, size_t y) const
{
    return ConstDotProxy{
        .temp = temps(x, y),
        .heat_capacity = heat_capacities(x, y),
        .thermal_conductivity = thermal_conductivities(x, y),
        .color = colors(x, y),
        .tags = tags(x, y),
        .physical_behavior = physical_behaviors(x, y),
        .aux = auxs(x, y),
        .id = material_ids(x, y)
    };
}
