#ifndef MOOX_SAVECONTAINER_HPP
#define MOOX_SAVECONTAINER_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

enum class SaveFileSemantics {Unknown, ColorLayer, Meta, DataLayer};

/// Elements of this array MUST BE lowercase ASCII letters (a-z)
using SaveSubfileName = std::array<char, 4>;


class ReadSaveContainer
{
public:
    /// Stats of files stored in container
    struct ContainedFilesStats
    {
        size_t valid_files{};
        /// Files intentionally ignored by decoder.
        /// @example files not matching naming conventions in zip container
        size_t ignored_files{};
        /// Files that failed to decode or parse
        size_t invalid_files{};
    };

public:
    virtual ~ReadSaveContainer() = default;
    virtual bool has_file(SaveSubfileName) const = 0;
    virtual ContainedFilesStats get_contained_files_stats() const = 0;
    virtual void get_file_names(std::vector<SaveSubfileName> &) const = 0;
    virtual std::optional<std::pair<std::vector<uint8_t>, SaveFileSemantics>> 
        load_file(SaveSubfileName) const = 0;
};


class WriteSaveContainer
{
public:
    virtual ~WriteSaveContainer() = default;
    virtual void store_file(SaveSubfileName, std::span<const uint8_t>, SaveFileSemantics) = 0;
    virtual bool close() = 0;
};

#endif // MOOX_SAVECONTAINER_HPP
