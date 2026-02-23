#ifndef MOOX_SOUNDSYSTEM_HPP
#define MOOX_SOUNDSYSTEM_HPP

#include <optional>
#include <string_view>

namespace sfx
{
    void init();
    void play_sound(
        std::string_view name, 
        std::optional<std::string_view> category = std::nullopt, 
        bool do_override = false
    );
}

#endif // MOOX_SOUNDSYSTEM_HPP
