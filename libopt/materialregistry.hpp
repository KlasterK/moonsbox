#ifndef MOOX_MATERIALREGISTRY_HPP
#define MOOX_MATERIALREGISTRY_HPP

#include "materialcontroller.hpp"
#include <string_view>
#include <unordered_map>

class MaterialRegistry
{
public:
    bool register_controller(MaterialController &controller, std::string_view name);
    MaterialController *find_controller_by_name(std::string_view name) const;
    std::optional<std::string_view> get_name_of_controller(const MaterialController *ctl) const;
    bool is_controller_registered(const MaterialController *ctl) const;

    inline auto begin() const { return m_name_ctl_map.begin(); }
    inline auto end() const { return m_name_ctl_map.end(); }

private:
    std::unordered_map<std::string_view, MaterialController *> m_name_ctl_map;
    std::unordered_map<MaterialController *, std::string_view> m_ctl_name_map;
};

#endif // MOOX_MATERIALREGISTRY_HPP
