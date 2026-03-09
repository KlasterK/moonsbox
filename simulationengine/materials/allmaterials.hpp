#ifndef MOOX_ALLMATERIALS_HPP
#define MOOX_ALLMATERIALS_HPP

#include <simulationengine/materials/absorbent.hpp>
#include <simulationengine/materials/fire.hpp>
#include <simulationengine/materials/lava.hpp>
#include <simulationengine/materials/plus100k.hpp>
#include <simulationengine/materials/pureglass.hpp>
#include <simulationengine/materials/space.hpp>
#include <simulationengine/materials/tap.hpp>
#include <simulationengine/materials/water.hpp>
#include <simulationengine/materials/aerogel.hpp>
#include <simulationengine/materials/blackhole.hpp>
#include <simulationengine/materials/dryice.hpp>
#include <simulationengine/materials/ice.hpp>
#include <simulationengine/materials/minus100k.hpp>
#include <simulationengine/materials/propane.hpp>
#include <simulationengine/materials/sand.hpp>
#include <simulationengine/materials/steam.hpp>
#include <simulationengine/materials/unbreakablewall.hpp>

template<template<typename ...> class Container>
struct AllMaterials
{
    using Type = Container<
        Space, Sand, Plus100K, Minus100K, Water, Ice, Steam,
        Tap, UnbreakableWall, BlackHole, Propane, Fire, PureGlass,
        Lava, Absorbent, Aerogel, DryIce
    >;
};

template<template<typename...> class Container>
using AllMaterialsT = AllMaterials<Container>::Type;

template<typename T>
constexpr void register_all_materials(T &container, MaterialRegistry &registry)
{
    registry.register_controller(std::get<Space>(container),            "Space");
    registry.register_controller(std::get<Sand>(container),             "Sand");
    registry.register_controller(std::get<Plus100K>(container),         "+100 K");
    registry.register_controller(std::get<Minus100K>(container),        "-100 K");
    registry.register_controller(std::get<Water>(container),            "Water");
    registry.register_controller(std::get<Ice>(container),              "Ice");
    registry.register_controller(std::get<Steam>(container),            "Steam");
    registry.register_controller(std::get<Tap>(container),              "Tap");
    registry.register_controller(std::get<UnbreakableWall>(container),  "Unbreakable Wall");
    registry.register_controller(std::get<BlackHole>(container),        "Black Hole");
    registry.register_controller(std::get<Propane>(container),          "Propane");
    registry.register_controller(std::get<Fire>(container),             "Fire");
    registry.register_controller(std::get<PureGlass>(container),        "Glass");
    registry.register_controller(std::get<Lava>(container),             "Lava");
    registry.register_controller(std::get<Absorbent>(container),        "Absorbent");
    registry.register_controller(std::get<Aerogel>(container),          "Aerogel");
    registry.register_controller(std::get<DryIce>(container),           "Dry Ice");
}

#endif // MOOX_ALLMATERIALS_HPP
