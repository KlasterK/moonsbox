#ifndef KK_OPT_MODAPI_H
#define KK_OPT_MODAPI_H

#include <utility>
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Basic Definitions */

typedef struct tagPoint 
{
    int x, y;
} Point;

typedef unsigned long long MaterialFlags; 
enum
{
    MAT_FLAG_NULL   = 0,
    MAT_FLAG_SOLID  = 1,
    MAT_FLAG_BULK   = 2,
    MAT_FLAG_LIQUID = 4,
    MAT_FLAG_GAS    = 8,
    MAT_FLAG_SPACE  = 16,
    MAT_FLAG_FLOAT  = 32,

    MAT_FLAG_SPARSENESS = MAT_FLAG_GAS | MAT_FLAG_SPACE,
    MAT_FLAG_MOVABLE    = MAT_FLAG_BULK | MAT_FLAG_LIQUID | MAT_FLAG_GAS | MAT_FLAG_FLOAT,
};

typedef struct tagMaterialData 
{
    float temp, heat_capacity, thermal_conductivity;
    uint32_t color_rgba;
    uintptr_t aux;
    MaterialFlags flags;
    void (*update_func)(Point pos);
} MaterialData;

typedef struct tagOptionalMaterialData
{
    MaterialData value;
    bool is_present;
} OptionalMaterialData;

/* Game-Mod API */

OptionalMaterialData mo_get_value_at(void* ctx, Point pos);
MaterialData* mo_get_ptr_at(void* ctx, Point pos);
void mo_register_material(
    void* ctx, 
    const char* name, 
    uint8_t api_version[2],
    void (*update_func)(Point pos),
    size_t (*serialize_aux_func)(MaterialData dot, void* buffer, size_t size),
    uintptr_t (*deserialize_aux_func)(const void* data, size_t size)
);

/* Mod Loader Definitions */

typedef struct tagModEntry
{
    uint8_t api_version[2];
    const char* mod_name;
    uint8_t mod_version[2];
    void (*init_func)(void* ctx);
    void (*exit_func)();
} ModEntry;

/* Utility */

void util_swap(void* ctx, Point pos1, Point pos2);
void util_fall_gas(void* ctx, Point pos);
void util_fall_liquid(void* ctx, Point pos);
void util_fall_sand(void* ctx, Point pos);
void util_fall_ash(void* ctx, Point pos);
int util_von_neumann_neighbours(void* ctx, Point pos, OptionalMaterialData* neighbours_buffer);
int util_moore_neighbours(void* ctx, Point pos, OptionalMaterialData* neighbours_buffer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* include guard - KK_OPT_MODAPI_H */