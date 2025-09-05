#ifndef KK_OPT_MODAPI_H
#define KK_OPT_MODAPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef void(*setup_dot_func_t)(Point pos);
typedef void(*update_dot_func_t)(Point pos);
typedef size_t(*get_serialize_aux_size_func_t)(Point pos);
typedef void(*serialize_aux_into_func_t)(Point pos, void* buffer, size_t size);
typedef void(*deserialize_aux_func_t)(Point pos, const void* data, size_t size);

MaterialData* mo_get_at(void* ctx, Point pos);
const MaterialData* mo_get_const_at(void* ctx, Point pos);
void mo_register_material(
    void* ctx,
    const char* name,
    uint8_t version[2],
    setup_dot_func_t setup_func,
    update_dot_func_t update_func,
    get_serialize_aux_size_func_t get_serialize_aux_size_func,
    serialize_aux_into_func_t serialize_aux_into_func_t,
    deserialize_aux_func_t deserialize_aux_func
);

/* Mod Loader Definitions */

typedef void (*init_mod_func_t)(void* ctx);
typedef void (*exit_mod_func_t)(void);

typedef struct tagModEntry
{
    uint8_t api_version[2];
    const char* mod_name;
    uint8_t mod_version[2];
    init_mod_func_t init_func;
    exit_mod_func_t exit_func;
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
