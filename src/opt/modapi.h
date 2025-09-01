#ifndef KK_OPT_MODAPI_H
#define KK_OPT_MODAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct tagPoint 
{
    int x, y;
} Point;

typedef long long MaterialFlags; 
enum 
{
    MAT_FLAG_NULL   = 0,
    MAT_FLAG_SOLID  = 1,
    MAT_FLAG_BULK   = 2,
    MAT_FLAG_LIQUID = 4,
    MAT_FLAG_GAS    = 8,
    MAT_FLAG_SPACE  = 16,

    MAT_FLAG_SPARSENESS = MAT_FLAG_GAS | MAT_FLAG_SPACE,
    MAT_FLAG_MOVABLE    = MAT_FLAG_BULK | MAT_FLAG_LIQUID | MAT_FLAG_GAS,
};

typedef struct tagMaterialData 
{
    float temp, heat_capacity, thermal_conductivity;
    uint32_t color_rgba;
    uintptr_t aux;
    MaterialFlags flags;
    void (*update_func)(Point pos);
} MaterialData;

typedef struct tagModExportTable
{
    char display_name[256];
    int version_major, version_minor, version_patch;
    void (*update_func)(Point);
} ModExportTable;

typedef struct tagModEntry
{
    char mod_name[256];
    int version_major, version_minor, version_patch;
    void (*init_func)(
        MaterialData* (*get_at)(Point),
        const MaterialData* (*get_at_const)(Point)
    );
    void (*exit_func)();
    const ModExportTable* export_table;
} ModEntry;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* include guard - KK_OPT_MODAPI_H */