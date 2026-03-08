#ifndef MOOX_MATERIALREGISTRY_HPP
#define MOOX_MATERIALREGISTRY_HPP

#include <string_view>
#include <vector>
#include <optional>
#include <utility>
#include <string>

class MaterialController;

class MaterialRegistry
{
public:
    bool register_controller(MaterialController &controller, std::string_view name);
    MaterialController *find_controller_by_name(std::string_view name) const;
    std::optional<std::string_view> get_name_of_controller(const MaterialController *ctl) const;
    bool is_controller_registered(const MaterialController &ctl) const;

    inline auto begin() const { return m_materials_table.begin(); }
    inline auto end() const { return m_materials_table.end(); }

private:
    std::vector<std::pair<std::string, MaterialController *>> m_materials_table;
};

#endif // MOOX_MATERIALREGISTRY_HPP
