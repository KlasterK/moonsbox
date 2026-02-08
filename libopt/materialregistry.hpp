#ifndef MOOX_MATERIALREGISTRY_HPP
#define MOOX_MATERIALREGISTRY_HPP

#include "materialcontroller.hpp"
#include <unordered_map>

class MaterialRegistry
{
public:
    bool register_controller(MaterialController &controller, std::string_view name);
    MaterialController *find_controller_by_name(std::string_view name) const;
    MaterialController *find_controller_by_id(MaterialID id) const;

    inline auto begin() const { return m_controllers.begin(); }
    inline auto end() const { return m_controllers.end(); }

private:
    std::unordered_map<std::string_view, MaterialController *> m_controllers;
};

#endif // MOOX_MATERIALREGISTRY_HPP
