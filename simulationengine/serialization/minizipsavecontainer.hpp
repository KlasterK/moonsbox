#ifndef MOOX_MINIZIPSAVECONTAINER_HPP
#define MOOX_MINIZIPSAVECONTAINER_HPP

#include <simulationengine/serialization/savecontainer.hpp>
#include <filesystem>
#include <vector>
#include <utility>
#include <optional>
#include <span>
#include <cstdint>
#include <filesystem>
#include <minizip/zip.h>
#include <minizip/unzip.h>

class MinizipReadSaveContainer : public ReadSaveContainer
{
public:
    MinizipReadSaveContainer(std::filesystem::path file_path);
    bool has_file(SaveSubfileName name) const override;
    ContainedFilesStats get_contained_files_stats() const override;
    void get_file_names(std::vector<SaveSubfileName> &out_names) const override;
    std::optional<std::pair<std::vector<uint8_t>, SaveFileSemantics>> 
        load_file(SaveSubfileName name) const override;

    ~MinizipReadSaveContainer();
    MinizipReadSaveContainer(const MinizipReadSaveContainer &) = delete;
    MinizipReadSaveContainer(MinizipReadSaveContainer &&) = delete;
    MinizipReadSaveContainer &operator=(const MinizipReadSaveContainer &) = delete;
    MinizipReadSaveContainer &operator=(MinizipReadSaveContainer &&) = delete;

private:
    unzFile m_file{};
};

class MinizipWriteSaveContainer : public WriteSaveContainer
{
public:
    MinizipWriteSaveContainer(std::filesystem::path file_path);
    void store_file(SaveSubfileName name, std::span<const uint8_t> data, 
                    SaveFileSemantics sem) override;
    bool close() override;

    ~MinizipWriteSaveContainer();
    MinizipWriteSaveContainer(const MinizipWriteSaveContainer &) = delete;
    MinizipWriteSaveContainer(MinizipReadSaveContainer &&) = delete;
    MinizipWriteSaveContainer &operator=(const MinizipWriteSaveContainer &) = delete;
    MinizipWriteSaveContainer &operator=(MinizipWriteSaveContainer &&) = delete;

private:
    zipFile m_file{};
};

#endif // MOOX_MINIZIPSAVECONTAINER_HPP
