#include <simulationengine/serialization/saving.hpp>
#include <simulationengine/core/gamemap.hpp>
#include <simulationengine/core/materialcontroller.hpp>
#include <simulationengine/core/materialdefs.hpp>
#include <simulationengine/serialization/savecontainer.hpp>
#include <simulationengine/core/materialregistry.hpp>
#include <zlib.h>
#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <cstddef>
#include <cstring>
#include <exception>
#include <format>
#include <optional>
#include <ranges>
#include <bit>
#include <array>
#include <utility>
#include <vector>

static_assert(std::endian::native == std::endian::little);

#pragma pack(push, 1)
    struct PackedVersion
    {
        uint8_t major, minor, patch;
        /// Revision should be 0 for releases and ASCII character for experimental builds
        uint8_t revision;
    };

    struct MainHeader
    {
        uint8_t signature[16];
        PackedVersion game_version;
        uint32_t map_width, map_height;
        uint32_t main_header_crc32_zlib;
        uint64_t creation_unix_timestamp;
        uint8_t unused[24];
    };
    static_assert(sizeof(MainHeader) == 64);

    struct ArbitraryDataHeader
    {
        uint32_t skipped_dots_count;
        uint32_t data_size;
        PackedVersion version;
    };
#pragma pack(pop)

constexpr uint8_t MoonsboxSignature[16] = {'m', 'o', 'o', 'n', 's', 'b', 'o', 'x',
                                           ' ', 'b', '\r', '\n', '\0', '\3', '\4', '\32'};

constexpr PackedVersion CurrentGameVersion = {2, 0, 0, '\0'};

constexpr SaveSubfileName MainHeaderSubfileName{'m', 'o', 'm', 'h'};
constexpr SaveSubfileName ColorsSubfileName{'m', 'o', 'c', 'l'};
constexpr SaveSubfileName TempsSubfileName{'m', 'o', 't', 'e'};
constexpr SaveSubfileName HeatCapacitiesSubfileName{'m', 'o', 'h', 'c'};
constexpr SaveSubfileName ThermalConductivitiesSubfileName{'m', 'o', 't', 'c'};
constexpr SaveSubfileName TagsSubfileName{'m', 'o', 't', 'g'};
constexpr SaveSubfileName PhysicalBehaviorsSubfileName{'m', 'o', 'p', 'b'};
constexpr SaveSubfileName MaterialIndicesSubfileName{'m', 'o', 'm', 'i'};
constexpr SaveSubfileName MaterialNamesSubfileName{'m', 'o', 'm', 'n'};
constexpr SaveSubfileName ArbitraryDataSubfileName{'m', 'o', 'a', 'd'};

template<typename T>
auto _to_u8_span(const T *ptr, size_t items_count = 1)
{
    return std::span{reinterpret_cast<const uint8_t *>(ptr), sizeof(T) * items_count};
}

template<typename T>
auto _to_u8_span(std::span<const T> span)
{
    return std::span{reinterpret_cast<const uint8_t *>(span.data()), span.size_bytes()};
}



std::string _write_main_header(WriteSaveContainer &container, const GameMap &map)
{
    MainHeader main_hdr{
        .signature={},
        .game_version=CurrentGameVersion,
        .map_width=static_cast<uint32_t>(map.width()),
        .map_height=static_cast<uint32_t>(map.height()),
        .main_header_crc32_zlib=0,
        .creation_unix_timestamp=static_cast<uint32_t>(time(nullptr)),
        .unused={},
    };
    std::memcpy(&main_hdr, MoonsboxSignature, sizeof(MainHeader::signature));
    main_hdr.main_header_crc32_zlib = crc32(0, reinterpret_cast<uint8_t *>(&main_hdr), 
                                            sizeof(MainHeader));

    try
    {
        container.store_file(MainHeaderSubfileName, _to_u8_span(&main_hdr), 
                             SaveFileSemantics::Meta);
    }
    catch(const std::exception &e)
    {
        return std::format(
            "Failed to write main header to save container"
            "\n\nWriteSaveContainer::store_file thrown an exception."
            "\nwhat(): {}",
            e.what()
        );
    }
    return "";
}

std::string _write_basic_layers(WriteSaveContainer &container, const GameMap &map)
{
    std::string_view current_layer;
    try
    {
        current_layer = "colors";
        container.store_file(
            ColorsSubfileName,
            _to_u8_span(map.colors.span()), 
            SaveFileSemantics::ColorLayer
        );
        current_layer = "temperatures";
        container.store_file(
            TempsSubfileName, 
            _to_u8_span(map.temps.span()), 
            SaveFileSemantics::DataLayer
        );
        current_layer = "heat capacities";
        container.store_file(
            HeatCapacitiesSubfileName, 
            _to_u8_span(map.heat_capacities.span()), 
            SaveFileSemantics::DataLayer
        );
        current_layer = "thermal conductivities";
        container.store_file(
            ThermalConductivitiesSubfileName, 
            _to_u8_span(map.thermal_conductivities.span()), 
            SaveFileSemantics::DataLayer
        );
        current_layer = "physical behaviors";
        container.store_file(
            PhysicalBehaviorsSubfileName, 
            _to_u8_span(map.physical_behaviors.span()), 
            SaveFileSemantics::DataLayer
        );
    }
    catch(const std::exception &e)
    {
        return std::format(
            "Failed to write {} layer to save container"
            "\n\nWriteSaveContainer::store_file thrown an exception."
            "\nwhat(): {}",
            current_layer, e.what()
        );
    }
    return "";
}

std::string _write_tags_layer(WriteSaveContainer &container, const GameMap &map)
{
    static_assert(MtlTag::N <= sizeof(uint64_t) * 8);

    std::vector<uint8_t> vec;
    vec.resize(map.flat_size() * sizeof(uint64_t));

    auto *u64_data = reinterpret_cast<uint64_t *>(vec.data());
    auto tags_span = map.tags.span();

    for(size_t i{}; i < map.flat_size(); ++i)
    {
        u64_data[i] = tags_span[i].to_ullong();
    }

    try
    {
        container.store_file(TagsSubfileName, vec, SaveFileSemantics::DataLayer);
    }
    catch(const std::exception &e)
    {
        return std::format(
            "Failed to write tags layer to save container"
            "\n\nWriteSaveContainer::store_file thrown an exception."
            "\nwhat(): {}",
            e.what()
        );
    }
    return "";
}

std::string _write_material_ctls_layer(WriteSaveContainer &container, const GameMap &map, 
                                       const MaterialRegistry &registry)
{
    auto ctls_span = map.material_ctls.span();
    std::vector<MaterialController *> sorted_unique_ctls(map.flat_size());
    std::memcpy(sorted_unique_ctls.data(), ctls_span.data(), ctls_span.size_bytes());

    // Sort and pick unique controller ptrs for faster search
    std::ranges::sort(sorted_unique_ctls.begin(), sorted_unique_ctls.end());
    auto non_unique_range = std::ranges::unique(sorted_unique_ctls);
    sorted_unique_ctls.erase(non_unique_range.begin(), sorted_unique_ctls.end());
    sorted_unique_ctls.shrink_to_fit();

    assert(sorted_unique_ctls.size() <= std::numeric_limits<uint32_t>::max());

    // Write a file with controllers indices
    {
        std::vector<uint8_t> indices_vec(sizeof(uint32_t) * map.flat_size());
        auto *indices_u32 = reinterpret_cast<uint32_t *>(indices_vec.data());

        MaterialController *previous_ctl = 0;
        size_t previous_idx = 0;

        for(size_t i{}; i < ctls_span.size(); ++i)
        {
            if(previous_ctl != ctls_span[i])
            {
                previous_ctl = ctls_span[i];
                previous_idx = std::ranges::lower_bound(sorted_unique_ctls, previous_ctl) 
                             - sorted_unique_ctls.begin();
            }
            indices_u32[i] = previous_idx;
        }

        try 
        {
            container.store_file(
                MaterialIndicesSubfileName, 
                indices_vec, 
                SaveFileSemantics::DataLayer
            );
        } 
        catch (const std::exception &e)
        {
            return std::format(
                "Failed to write material controllers indices layer to save container"
                "\n\nWriteSaveContainer::store_file thrown an exception."
                "\nwhat(): {}",
                e.what()
            );
        }
    }

    // Write a file with material controllers names
    std::vector<uint8_t> names_vec;
    for(auto &ctl : sorted_unique_ctls)
    {
        auto name_opt = registry.get_name_of_controller(ctl);
        for(char c : name_opt.value())
        {
            names_vec.push_back(c);
        }
        names_vec.push_back('\0');
    }
    names_vec.push_back('\0');

    try 
    {
        container.store_file(
            MaterialNamesSubfileName, 
            names_vec, 
            SaveFileSemantics::DataLayer
        );
    } 
    catch (const std::exception &e)
    {
        return std::format(
            "Failed to write material names to save container"
            "\n\nWriteSaveContainer::store_file thrown an exception."
            "\nwhat(): {}",
            e.what()
        );
    }

    return {};
}

std::string _write_arbitrary_data_layer(WriteSaveContainer &container, const GameMap &map)
{
    uint32_t no_data_dots_count{};
    std::vector<uint8_t> file_data;

    for(size_t y{}; y < map.height(); ++y)
    {
        for(size_t x{}; x < map.width(); ++x)
        {
            auto *ctl = map.material_ctls(x, y);

            auto [dot_data_vec, dot_data_version] = ctl->serialize(map, x, y);
            if(dot_data_vec.empty())
            {
                ++no_data_dots_count;
                continue;
            }

            size_t block_begin_idx = file_data.size();
            size_t hdr_data_size = sizeof(ArbitraryDataHeader) + dot_data_vec.size();
            file_data.resize(file_data.size() + hdr_data_size);

            auto *hdr = reinterpret_cast<ArbitraryDataHeader *>(&file_data[block_begin_idx]);
            *hdr = {
                .skipped_dots_count=no_data_dots_count,
                .data_size=static_cast<uint32_t>(dot_data_vec.size()),
                .version={
                    .major=static_cast<uint8_t>(dot_data_version.major),
                    .minor=static_cast<uint8_t>(dot_data_version.minor),
                    .patch=static_cast<uint8_t>(dot_data_version.patch),
                    .revision=static_cast<uint8_t>(dot_data_version.revision),
                },
            };

            std::memcpy(hdr + 1, dot_data_vec.data(), dot_data_vec.size());
            no_data_dots_count = 0;
        }
    }

    try 
    {
        container.store_file(
            ArbitraryDataSubfileName, 
            file_data, 
            SaveFileSemantics::DataLayer
        );
    } 
    catch (const std::exception &e)
    {
        return std::format(
            "Failed to write arbitrary data layer to save container"
            "\n\nWriteSaveContainer::store_file thrown an exception."
            "\nwhat(): {}",
            e.what()
        );
    }

    return {};
}

std::expected<void, std::string> saving::serialize(WriteSaveContainer &container, 
                                                   const GameMap &map, 
                                                   const MaterialRegistry &registry)
{
    if(auto str = _write_main_header(container, map); !str.empty())
        return std::unexpected(std::move(str));

    if(auto str = _write_basic_layers(container, map); !str.empty())
        return std::unexpected(std::move(str));

    if(auto str = _write_tags_layer(container, map); !str.empty())
        return std::unexpected(std::move(str));

    if(auto str = _write_material_ctls_layer(container, map, registry); !str.empty())
        return std::unexpected(std::move(str));

    if(auto str = _write_arbitrary_data_layer(container, map); !str.empty())
        return std::unexpected(std::move(str));

    return {};
}



std::optional<MainHeader> _read_main_header(const ReadSaveContainer &container)
{
    auto file_opt = container.load_file(MainHeaderSubfileName);
    if(!file_opt)
        return std::nullopt;

    auto &[vec, sem] = *file_opt;
    if(vec.size() != sizeof(MainHeader) || sem != SaveFileSemantics::Meta)
        return std::nullopt;

    MainHeader hdr;
    std::memcpy(&hdr, vec.data(), sizeof(MainHeader));
    return hdr;
}

std::string _read_basic_layers(const ReadSaveContainer &container, GameMap &map)
{
    std::initializer_list layers{
        std::make_tuple(ColorsSubfileName, "colors", sizeof(uint32_t), 
                        static_cast<void *>(map.colors.span().data())),
        std::make_tuple(TempsSubfileName, "temperatures", sizeof(float), 
                        static_cast<void *>(map.temps.span().data())),
        std::make_tuple(HeatCapacitiesSubfileName, "heat capacities", sizeof(float),
                        static_cast<void *>(map.heat_capacities.span().data())),
        std::make_tuple(ThermalConductivitiesSubfileName, "thermal conductivities", sizeof(float), 
                        static_cast<void *>(map.thermal_conductivities.span().data())),
        std::make_tuple(PhysicalBehaviorsSubfileName, "physical behaviors", 
                        sizeof(PhysicalBehaviorsSubfileName),
                        static_cast<void *>(map.physical_behaviors.span().data())),
    };

    for(auto &[subfile_name, visible_name, elem_size, dst] : layers)
    {
        auto file_opt = container.load_file(subfile_name);
        if(!file_opt)
            return std::format("Container subfile of {} layer not found", visible_name);

        auto &[vec, _] = *file_opt;
        if(vec.size() != map.flat_size() * elem_size)
            return std::format("Subfile of {} layer is wrong size", visible_name);

        std::memcpy(dst, vec.data(), vec.size());
    }
    return {};
}

std::string _read_tags_layer(const ReadSaveContainer &container, GameMap &map)
{
    auto file_opt = container.load_file(TagsSubfileName);
    if(!file_opt)
        return "Container subfile of tags layer not found";

    auto &[vec, _] = *file_opt;
    if(vec.size() != map.flat_size() * sizeof(uint64_t))
        return "Subfile of tags layer is wrong size";

    const auto *u64_data = reinterpret_cast<const uint64_t *>(vec.data());
    auto tags_span = map.tags.span();

    for(size_t i{}; i < map.flat_size(); ++i)
    {
        tags_span[i] = MaterialTags(u64_data[i]);
    }
    return {};
}

std::string _read_material_ctls_layer(const ReadSaveContainer &container, GameMap &map, 
                                      const MaterialRegistry &registry) 
{
    auto names_file_opt = container.load_file(MaterialNamesSubfileName);
    if(!names_file_opt)
        return "Container subfile of material names not found";

    const auto &read_names_vec = names_file_opt->first;
    std::vector<MaterialController *> ctls_vec;

    // Get material controllers indices from names subfile
    {
        auto it = read_names_vec.begin();
        for(;;)
        {
            auto nul_it = std::find(it, read_names_vec.end(), '\0');
            if(nul_it == read_names_vec.end())
                break;
            
            std::string_view name(
                reinterpret_cast<const char *>(&*it), 
                static_cast<size_t>(nul_it - it)
            );
            it = nul_it + 1;

            if(name.empty())
                continue;

            auto *ctl_ptr = registry.find_controller_by_name(name);
            if(ctl_ptr == nullptr)
                return std::format(
                    "Material with name {} not found",
                    "\n\nMaterial with this name is used in the save,"
                    " but it's not present in the registry.",
                    name
                );

            ctls_vec.push_back(ctl_ptr);
        }
    }

    auto file_opt = container.load_file(MaterialIndicesSubfileName);
    if(!file_opt)
        return "Container subfile of material controllers indices layer not found";

    const auto &vec = file_opt->first;
    if(vec.size() != map.flat_size() * sizeof(uint32_t))
        return "Subfile of tags layer is wrong size";

    const auto *u32_indices = reinterpret_cast<const uint32_t *>(vec.data());
    auto ctls_span = map.material_ctls.span();

    for(size_t i{}; i < map.flat_size(); ++i)
    {
        uint32_t idx = u32_indices[i];
        if(idx >= ctls_vec.size())
            return std::format(
                "Material index {} went out of bounds (only {} materials in save)",
                idx, vec.size()
            );
        
        ctls_span[i] = ctls_vec[idx];
    }
    return {};
}

std::string _read_arbitrary_data_layer(const ReadSaveContainer &container, GameMap &map, 
                                       const MaterialRegistry &registry)
{
    auto file_opt = container.load_file(ArbitraryDataSubfileName);
    if(!file_opt)
        return "Container subfile of arbitrary data layer not found";

    const auto &[vec, _] = *file_opt;
    if(vec.empty())
        return {};

    size_t map_idx{};
    for(auto block_begin_it = vec.begin();;)
    {
        const auto *hdr = reinterpret_cast<const ArbitraryDataHeader *>(&*block_begin_it);

        map_idx += hdr->skipped_dots_count;
        if(map_idx >= map.flat_size())
            return "Failed to parse arbitrary data layer"
                   "\n\nSkipped dots count went out of map bounds";
        
        if(hdr->data_size > vec.end() - block_begin_it - sizeof(ArbitraryDataHeader))
            return "Failed to parse arbitrary data layer"
                   "\n\nDot data size went out of file bounds";

        const auto *data_ptr_u8 = reinterpret_cast<const uint8_t *>(hdr + 1);

        auto *ctl = reinterpret_cast<MaterialController *>(map.material_ctls.span()[map_idx]);

        SemanticVersion ver{
            .major=hdr->version.major,
            .minor=hdr->version.minor,
            .patch=hdr->version.patch,
            .revision=static_cast<char>(hdr->version.revision),
        };

        auto err = ctl->deserialize(
            map, 
            map_idx % map.width(), 
            map_idx / map.width(), 
            {data_ptr_u8, hdr->data_size}, 
            ver
        );

        std::string_view err_msg;
        switch(err)
        {
            using enum MaterialController::DeserializationResult;
        case VersionTooOld:
            err_msg = "Version too old.";
            break;
        case VersionTooNew:
            err_msg = "Version too new.";
            break;
        case InvalidDataLength:
            err_msg = "Invalid data length.";
            break;
        case InvalidDataFormat:
            err_msg = "Invalid data format.";
            break;
        case BrokenInvariant:
            err_msg = "Broken invariant.";
            break;
        case MissingDependency:
            err_msg = "Missing dependency.";
            break;
        case Success:
        case NotImplemented:
            break;
        }

        if(!err_msg.empty())
            return std::format(
                "{} material refused to deserialize data\n\n{}",
                registry.get_name_of_controller(ctl).value(), err_msg
            );

        block_begin_it += sizeof(ArbitraryDataHeader) + hdr->data_size;
        if(static_cast<size_t>(vec.end() - block_begin_it) < sizeof(ArbitraryDataHeader))
            break;
        ++map_idx;
    }

    return {};
}

std::expected<GameMap, std::string> saving::deserialize(const ReadSaveContainer &container, 
                                                        const MaterialRegistry &registry)
{
    auto main_hdr_opt = _read_main_header(container);
    if(!main_hdr_opt)
        return std::unexpected("Failed to read main header from container");
    auto main_hdr = *main_hdr_opt;

    if(0 != std::memcmp(main_hdr.signature, MoonsboxSignature, sizeof(MainHeader::signature)))
        return std::unexpected("Save file is not a moonsbox save");

    auto crc_from_hdr = main_hdr.main_header_crc32_zlib;
    main_hdr.main_header_crc32_zlib = 0;
    if(crc_from_hdr != crc32(0, reinterpret_cast<uint8_t *>(&main_hdr), sizeof(MainHeader)))
        return std::unexpected("Main header checksum mismatch");

    if (
        main_hdr.game_version.major != CurrentGameVersion.major 
        || main_hdr.game_version.minor < CurrentGameVersion.minor
    )
        return std::unexpected(std::format(
            "Save file is outdated. Save version is {}.{}.{}{}, current version is {}.{}.{}{}",
            main_hdr.game_version.major,
            main_hdr.game_version.minor,
            main_hdr.game_version.patch,
            (char []){(char)main_hdr.game_version.revision, 0},
            CurrentGameVersion.major,
            CurrentGameVersion.minor,
            CurrentGameVersion.patch,
            (char []){(char)CurrentGameVersion.revision, 0}
        ));
    
    GameMap map(main_hdr.map_width, main_hdr.map_height);
    auto *space = registry.find_controller_by_name("Space");
    if(space == nullptr)
        throw std::runtime_error("saving::deserialize: Space is not registered in registry");

    for(size_t y{}; y < map.height(); ++y)
        for(size_t x{}; x < map.width(); ++x)
            space->init_point(map, x, y);
    
    if(auto str = _read_basic_layers(container, map); !str.empty())
        return std::unexpected(std::move(str));
    
    if(auto str = _read_tags_layer(container, map); !str.empty())
        return std::unexpected(std::move(str));
    
    if(auto str = _read_material_ctls_layer(container, map, registry); !str.empty())
        return std::unexpected(std::move(str));
    
    if(auto str = _read_arbitrary_data_layer(container, map, registry); !str.empty())
        return std::unexpected(std::move(str));

    return map;
}
