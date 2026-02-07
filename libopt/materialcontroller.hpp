#ifndef MOOX_MATERIALCONTROLLER_HPP
#define MOOX_MATERIALCONTROLLER_HPP

#include "materialdefs.hpp"
#include "gamemap.hpp"
#include <vector>
#include <span>

class MaterialController
{
public:
    enum class DeserializationError
    {
        Success, VersionTooOld, VersionTooNew, InvalidDataLength, 
        InvalidDataFormat, BrokenInvariant
    };

    struct SaveVersion
    {
        int major, minor, patch, revision;
    };

public:
    virtual ~MaterialController() = default;
    virtual void init_point(GameMap &, size_t, size_t) {}
    virtual void static_update(GameMap &) {}
    virtual void dynamic_update(GameMap &) {}
    virtual void on_register(class SimulationManager &) {}
    virtual bool is_placeable_on(GameMap &, size_t, size_t) { return true; }

    virtual std::vector<uint8_t> serialize(GameMap &, size_t, size_t) { return {}; }
    virtual bool deserialize(GameMap &, size_t, size_t, std::span<const uint8_t>) { return {}; }

    inline MaterialID material_id()
    {
        return reinterpret_cast<MaterialID>(this);
    }
};

#endif // MOOX_MATERIALCONTROLLER_HPP
