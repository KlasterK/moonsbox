#ifndef MOOX_SOUNDSYSTEM_HPP
#define MOOX_SOUNDSYSTEM_HPP

#include <optional>
#include <string_view>
#include <type_traits>
#include <simulationengine/core/materialcontroller.hpp>

namespace sfx
{
    void init();
    void play_sound(
        std::string_view name,
        std::optional<std::string_view> category = std::nullopt,
        bool do_override = false
    );
    static_assert(std::is_convertible_v<decltype(play_sound), MaterialController::PlaySoundCallback>);
}

#endif // MOOX_SOUNDSYSTEM_HPP
