#ifndef MOOX_MATERIALCONTROLLER_HPP
#define MOOX_MATERIALCONTROLLER_HPP

#include "gamemap.hpp"
#include "saving.hpp"
#include <vector>
#include <span>

class MaterialController
{
public:
    enum class DeserializationResult
    {
        Success, NotImplemented, 
        VersionTooOld, VersionTooNew, InvalidDataLength, InvalidDataFormat, 
        BrokenInvariant, MissingDependency,
    };

public:
    virtual ~MaterialController() = default;
    virtual void init_point(GameMap &, size_t, size_t) {}
    virtual void static_update(GameMap &) {}
    virtual void dynamic_update(GameMap &) {}
    virtual void on_register(class MaterialRegistry &) {}
    virtual bool is_placeable_on(GameMap &, size_t, size_t) { return true; }

    virtual std::pair<std::vector<uint8_t>, saving::SaveVersion> 
        serialize(const GameMap &, size_t, size_t) { return {}; }

    virtual DeserializationResult deserialize(
        GameMap &, size_t, size_t, std::span<const uint8_t>, saving::SaveVersion
    ) { return DeserializationResult::NotImplemented; }
};

#endif // MOOX_MATERIALCONTROLLER_HPP
