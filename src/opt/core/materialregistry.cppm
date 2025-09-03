#include <stdexcept>
module;
#include <modapi.h>
#include <string>
#include <cstdint>
#include <functional>
#include <cstdint>
#include <array>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <expected>
#include <utility>
export module materialregistry;

export class MaterialRegistry
{
public:
    struct ID
    {
        std::string name;
        std::optional<std::array<uint8_t, 2>> version;
    };

    using update_func_t = std::function<void(Point pos)>;
    using serialize_aux_func_t = std::function<size_t(Point pos, void* buffer, size_t size)>;
    using deserialize_aux_func_t = std::function<uintptr_t(const void* data, size_t size)>;

    struct FuncSet
    {
        update_func_t update_func;
        serialize_aux_func_t serialize_aux_func;
        deserialize_aux_func_t deserialize_aux_func;
    };

public:
    MaterialRegistry() = default;

    ~MaterialRegistry() = default;

    void register_material(
        ID id,
        FuncSet func_set
    ) 
    {
        if(id.version && (*id.version)[0] == 0) id.version = std::nullopt;
        m_table.push_back({id, func_set}); 
    }

    std::optional<FuncSet> get_func_set_from_id(ID id)
    {
        for(const auto& item : m_table)
        {
            auto cmp_id = item.first;
            auto cmp_ver = *cmp_id.version, ver = *id.version;
            if (
                cmp_id.name == cmp_id.name 
                && !cmp_id.version 
                || !id.version 
                || (cmp_ver[0] == ver[0] && cmp_ver[1] >= ver[1])
            ) return item.second;
        }
        return std::nullopt;
    }

    std::optional<ID> get_id_from_update_func(update_func_t update_func)
    {}

private:
    std::vector<std::pair<ID, FuncSet>> m_table;
};
