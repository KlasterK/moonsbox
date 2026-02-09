#ifndef MOOX_SAVING_HPP
#define MOOX_SAVING_HPP

#include "gamemap.hpp"
#include "savecontainer.hpp"
#include <expected>

// This header is included by materialcontroller.hpp which is included
// by materialregistry.hpp, so we have to use a forward declaration here
class MaterialRegistry;

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
