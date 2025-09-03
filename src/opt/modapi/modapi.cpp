#include <modapi.h>

import modloader;

void mo_register_material(
    void* ctx_ptr,
    const char* name, 
    uint8_t version[2],
    void (*update_func)(Point pos),
    size_t (*serialize_aux_func)(Point pos, void* buffer, size_t size),
    uintptr_t (*deserialize_aux_func)(const void* data, size_t size)
)
{
    auto *ctx = static_cast<ExternalContext*>(ctx_ptr);
    
}