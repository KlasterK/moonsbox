#include "minizipsavecontainer.hpp"
#include "savecontainer.hpp"
#include <cassert>
#include <cctype>
#include <cstring>
#include <format>
#include <iostream>
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <zlib.h>

namespace SubfileHeaderSemanticsValue
{
    constexpr uint8_t Unknown = 0;
    constexpr uint8_t ColorLayer = 1;
    constexpr uint8_t Meta = 2;
    constexpr uint8_t DataLayer = 3;
}

#pragma pack(push, 1)
struct SubfileHeader
{
    uint8_t semantics;
    uint8_t unused[63];
};
static_assert(sizeof(SubfileHeader) == 64);
#pragma pack(pop)

inline std::optional<SaveSubfileName> _check_make_subfile_name(const std::string_view buf)
{
    assert(buf.size() >= std::tuple_size_v<SaveSubfileName>);

    SaveSubfileName subfile_name;
    for(int i{}; i < std::tuple_size_v<SaveSubfileName>; ++i)
    {
        if(!std::isalpha(buf[i]))
            return std::nullopt;

        subfile_name[i] = std::tolower(buf[i]);
    }
    return subfile_name;
}

inline std::string _make_string_from_subfile_name(SaveSubfileName subfile_name)
{
    std::string out(1 + std::tuple_size_v<SaveSubfileName>, 0);
    for(int i{}; i < std::tuple_size_v<SaveSubfileName>; ++i)
    {
        out[i] = subfile_name[i];
    }
    return out;
}



MinizipReadSaveContainer::MinizipReadSaveContainer(std::filesystem::path file_path)
    : m_file(unzOpen(file_path.c_str()))
{
    if(!m_file)
        throw std::runtime_error(std::format(
            "MinizipReadSaveContainer::MinizipReadSaveContainer"
            ": Cannot open zip file {}",
            file_path.c_str()
        ));
}

MinizipReadSaveContainer::~MinizipReadSaveContainer()
{
    if(unzClose(m_file) != UNZ_OK)
        std::cerr << "MinizipReadSaveContainer::~MinizipReadSaveContainer"
                     ": unzClose returned != UNZ_OK" << std::endl;
}

bool MinizipReadSaveContainer::has_file(SaveSubfileName name) const
{
    return UNZ_OK == unzLocateFile(m_file, _make_string_from_subfile_name(name).c_str(), 1);
}

ReadSaveContainer::ContainedFilesStats MinizipReadSaveContainer::get_contained_files_stats() const
{
    throw std::logic_error("MinizipReadSaveContainer::get_contained_files_stats: not implemented");
}

void MinizipReadSaveContainer::get_file_names(std::vector<SaveSubfileName> &out_names) const
{
    size_t added_items_idx_begin = out_names.size();

    constexpr size_t name_len = std::tuple_size_v<SaveSubfileName>;
    char name_buf[1 + name_len]{};

    if(UNZ_OK != unzGoToFirstFile(m_file))
        return;

    do
    {
        if(UNZ_OK != unzGetCurrentFileInfo(m_file, nullptr, name_buf, sizeof(name_buf), nullptr, 0, nullptr, 0))
            continue;

        auto name_opt = _check_make_subfile_name(name_buf);
        if(name_opt)
            out_names.push_back(std::move(*name_opt));
        memset(name_buf,0, sizeof(name_buf));
    } while(UNZ_OK == unzGoToNextFile(m_file));

    auto added_items_iter_begin = out_names.begin() + added_items_idx_begin;
    if(added_items_iter_begin == out_names.end())
        return;

    std::sort(added_items_iter_begin, out_names.end());
    auto last = std::unique(added_items_iter_begin, out_names.end());
    out_names.erase(last, out_names.end());
}

std::optional<std::pair<std::vector<uint8_t>, SaveFileSemantics>>
    MinizipReadSaveContainer::load_file(SaveSubfileName name) const
{
    // Find file by name and select it
    if(UNZ_OK != unzLocateFile(m_file, _make_string_from_subfile_name(name).c_str(), 1))
        return std::nullopt;

    // Get file size
    unz_file_info64 info;
    if(UNZ_OK != unzGetCurrentFileInfo64(m_file, &info, nullptr, 0, nullptr, 0, nullptr, 0))
        return std::nullopt;

    if(info.uncompressed_size < sizeof(SubfileHeader))
        return std::nullopt;

    if(UNZ_OK != unzOpenCurrentFile(m_file))
        return std::nullopt;

    // Read header to get file semantics
    SubfileHeader hdr;
    if(sizeof(SubfileHeader) != unzReadCurrentFile(m_file, &hdr, sizeof(SubfileHeader)))
    {
        unzCloseCurrentFile(m_file);
        return std::nullopt;
    }

    SaveFileSemantics semantics;
    if(hdr.semantics == SubfileHeaderSemanticsValue::ColorLayer)
        semantics = SaveFileSemantics::ColorLayer;
    if(hdr.semantics == SubfileHeaderSemanticsValue::Meta)
        semantics = SaveFileSemantics::Meta;
    if(hdr.semantics == SubfileHeaderSemanticsValue::DataLayer)
        semantics = SaveFileSemantics::DataLayer;
    else
        semantics = SaveFileSemantics::Unknown;

    std::vector<uint8_t> data(info.uncompressed_size - sizeof(SubfileHeader));
    if(data.size() != unzReadCurrentFile(m_file, data.data(), sizeof(SubfileHeader)))
    {
        unzCloseCurrentFile(m_file);
        return std::nullopt;
    }

    unzCloseCurrentFile(m_file);
    return std::make_pair(std::move(data), semantics);
}



MinizipWriteSaveContainer::MinizipWriteSaveContainer(std::filesystem::path file_path)
    : m_file(zipOpen(file_path.c_str(), APPEND_STATUS_CREATE))
{
    if(!m_file)
        throw std::runtime_error(std::format(
            "MinizipWriteSaveContainer::MinizipWriteSaveContainer"
            ": Cannot open zip file {}",
            file_path.c_str()
        ));
}

MinizipWriteSaveContainer::~MinizipWriteSaveContainer()
{
    if(!m_file)
        return;
    
    if(zipClose(m_file, "") != ZIP_OK)
        std::cerr << "MinizipWriteSaveContainer::~MinizipWriteSaveContainer"
                     ": zipClose returned != ZIP_OK" << std::endl;
}

void MinizipWriteSaveContainer::store_file(SaveSubfileName name, std::span<const uint8_t> data, 
                                           SaveFileSemantics semantics)
{
    if(!m_file)
        throw std::runtime_error(
            "MinizipWriteSaveContainer::store_file"
            ": File is closed"
        );
    
    zip_fileinfo info{};

    int compression = Z_DEFAULT_COMPRESSION;
    switch(semantics)
    {
    case SaveFileSemantics::ColorLayer:
    case SaveFileSemantics::DataLayer:
        compression = Z_BEST_COMPRESSION;
        break;
    case SaveFileSemantics::Meta:
        compression = Z_BEST_SPEED;
        break;
    default:
    }

    if(ZIP_OK != zipOpenNewFileInZip(
        m_file, 
        _make_string_from_subfile_name(name).c_str(), 
        &info,
        nullptr, 0, nullptr, 0, nullptr, 
        Z_DEFLATED, 
        compression
    ))
        throw std::runtime_error(
            "MinizipWriteSaveContainer::store_file"
            ": zipOpenNewFileInZip returned != ZIP_OK"
        );

    SubfileHeader hdr;
    if(semantics == SaveFileSemantics::ColorLayer)
        hdr.semantics = SubfileHeaderSemanticsValue::ColorLayer;
    if(semantics == SaveFileSemantics::Meta)
        hdr.semantics = SubfileHeaderSemanticsValue::Meta;
    if(semantics == SaveFileSemantics::DataLayer)
        hdr.semantics = SubfileHeaderSemanticsValue::DataLayer;
    else
        hdr.semantics = SubfileHeaderSemanticsValue::Unknown;
    
    if(ZIP_OK != zipWriteInFileInZip(m_file, &hdr, sizeof(SubfileHeader)))
    {
        zipCloseFileInZip(m_file);
        throw std::runtime_error(
            "MinizipWriteSaveContainer::store_file"
            ": zipWriteInFileInZip returned != ZIP_OK"
        );
    }

    if(ZIP_OK != zipWriteInFileInZip(m_file, data.data(), data.size_bytes()))
    {
        zipCloseFileInZip(m_file);
        throw std::runtime_error(
            "MinizipWriteSaveContainer::store_file"
            ": zipWriteInFileInZip returned != ZIP_OK"
        );
    }

    if(ZIP_OK != zipCloseFileInZip(m_file))
        throw std::runtime_error(
            "MinizipWriteSaveContainer::store_file"
            ": zipCloseFileInZip returned != ZIP_OK"
        );
}

bool MinizipWriteSaveContainer::close()
{
    if(!m_file)
        return true;

    if(ZIP_OK != zipClose(m_file, ""))
        return false;

    m_file = 0;
    return true;
}
