#ifndef MOOX_MATERIALCONTROLLER_HPP
#define MOOX_MATERIALCONTROLLER_HPP

#include <simulationengine/core/compat.hpp>
#include <simulationengine/core/materialdefs.hpp>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include <span>
#include <optional>

class GameMap;
class MaterialRegistry;

class MaterialController
{
public:
    enum class DeserializationResult
    {
        Success, NotImplemented, 
        VersionTooOld, VersionTooNew, InvalidDataLength, InvalidDataFormat, 
        BrokenInvariant, MissingDependency,
    };

    using PlaySoundCallback = MoveOnlyOrOldFunction<void(
        std::string_view name,
        std::optional<std::string_view> category,
        bool do_override
    )>;

public:
    virtual ~MaterialController() = default;

    virtual void init_point(GameMap &, size_t, size_t) {}
    virtual void static_update(GameMap &) {}
    virtual void dynamic_update(GameMap &) {}
    virtual bool is_placeable_on(GameMap &, size_t, size_t) { return true; }

    virtual void set_play_sound_callback(PlaySoundCallback &&) {}
    virtual void play_place_sound(GameMap &, size_t, size_t) {}

    virtual std::pair<std::vector<uint8_t>, SemanticVersion> 
        serialize(const GameMap &, size_t, size_t) { return {}; }

    virtual DeserializationResult
        deserialize(GameMap &, size_t, size_t, std::span<const uint8_t>, SemanticVersion) 
        { return DeserializationResult::NotImplemented; }

protected:
    virtual void on_register(MaterialRegistry &) {}
    friend MaterialRegistry;
};

#endif // MOOX_MATERIALCONTROLLER_HPP
