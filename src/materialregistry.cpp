#include "materialregistry.hpp"
#include "materialcontroller.hpp"

bool MaterialRegistry::register_controller(MaterialController &controller, std::string_view name)
{
    for(auto &[reg_name, reg_ctl] : m_materials_table)
    {
        if(name == reg_name || &controller == reg_ctl)
            return false;
    }
    m_materials_table.emplace_back(name, &controller);
    controller.on_register(*this);
    return true;
}

MaterialController *MaterialRegistry::find_controller_by_name(std::string_view name) const
{
    for(auto &[reg_name, reg_ctl] : m_materials_table)
    {
        if(name == reg_name)
            return reg_ctl;
    }
    return nullptr;
}

std::optional<std::string_view> 
    MaterialRegistry::get_name_of_controller(const MaterialController *ctl) const
{
    for(auto &[reg_name, reg_ctl] : m_materials_table)
    {
        if(ctl == reg_ctl)
            return reg_name;
    }
    return std::nullopt;
}

bool MaterialRegistry::is_controller_registered(const MaterialController &ctl) const
{
    for(auto &[reg_name, reg_ctl] : m_materials_table)
    {
        if(&ctl == reg_ctl)
            return false;
    }
    return true;
}
