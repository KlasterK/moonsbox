#include "materialregistry.hpp"


bool MaterialRegistry::register_controller(MaterialController &controller, std::string_view name)
{
    auto [_, was_inserted] = m_controllers.insert({name, &controller});
    if(was_inserted)
        controller.on_register(*this);
    return was_inserted;
}


MaterialController *MaterialRegistry::find_controller_by_name(std::string_view name) const
{
    auto it = m_controllers.find(name);
    if(it == m_controllers.end())
        return nullptr;
    
    return it->second;
}

MaterialController *MaterialRegistry::find_controller_by_id(MaterialID id) const
{
    auto id_as_ptr = reinterpret_cast<MaterialController *>(id);
    for(auto &[_, ctl] : m_controllers)
    {
        if(ctl == id_as_ptr)
            return ctl;
    }
    return nullptr;
}
