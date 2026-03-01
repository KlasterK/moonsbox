#ifndef MOOX_SAVING_HPP
#define MOOX_SAVING_HPP

#include <string>
#include <expected>

class MaterialRegistry;
class WriteSaveContainer;
class ReadSaveContainer;
class GameMap;

namespace saving
{
    struct SaveVersion
    {
        int major, minor, patch, revision;
    };
    
    std::expected<void, std::string> serialize(
        WriteSaveContainer &container, const GameMap &map, const MaterialRegistry &registry
    );

    std::expected<GameMap, std::string> deserialize(
        const ReadSaveContainer &container, const MaterialRegistry &registry
    );
}

#endif // MOOX_SAVING_HPP
