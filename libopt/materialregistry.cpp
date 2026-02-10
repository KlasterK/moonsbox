#include "materialregistry.hpp"
#include <optional>

bool MaterialRegistry::register_controller(MaterialController &controller, std::string_view name)
{
    auto [name_ctl_it, name_ctl_was_inserted] = m_name_ctl_map.insert({name, &controller});
    if(!name_ctl_was_inserted)
        return false;

    auto [_, ctl_name_was_inserted] = m_ctl_name_map.insert({&controller, name});
    if(!ctl_name_was_inserted)
    {
        m_name_ctl_map.erase(name_ctl_it);
        return false;
    }

    controller.on_register(*this);
    return true;
}

MaterialController *MaterialRegistry::find_controller_by_name(std::string_view name) const
{
    auto it = m_name_ctl_map.find(name);
    if(it == m_name_ctl_map.end())
        return nullptr;
    
    return it->second;
}

std::optional<std::string_view> 
    MaterialRegistry::get_name_of_controller(const MaterialController *ctl) const
{
    // We only store pointers so const cast is not a problem
    auto it = m_ctl_name_map.find(const_cast<MaterialController *>(ctl));
    if(it == m_ctl_name_map.end())
        return std::nullopt;
    
    return it->second;
}

bool MaterialRegistry::is_controller_registered(const MaterialController *ctl) const
{
    return m_ctl_name_map.contains(const_cast<MaterialController *>(ctl));
}
