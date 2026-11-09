#ifndef RESOURCES_WORLD_RES_H
#define RESOURCES_WORLD_RES_H
#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include "resources/world_records.h"

struct C3dWorldRes {
    struct Res res;
    struct RswModel *models;
    size_t modelCount;
    struct RswEffect *effects;
    size_t effectCount;
    struct RswSound *sounds;
    size_t soundCount;
    struct RswBounds *tree;
    char gndFile[41], gatFile[41], srcFile[41];
    float waterLevel;
    int waterType;
    float waveHeight, waveSpeed, wavePitch;
    int waterAnimSpeed;
    int lightLatitude, lightLongitude;
    float lightDir[3], diffuse[3], ambient[3];
    /* custom: the share of the light the map's editor took away in the shadows it baked into the
       ground's lightmaps (they hold 255 lit, about 255 * (1 - shadowOpacity) in shadow), from
       version 1.7 on; 0.5 before */
    float shadowOpacity;
    unsigned char major, minor;
    int bounds[4];
    unsigned char twoPassAlpha;
};

BYTE c3d_world_res_load(struct C3dWorldRes *self, const char *name);
void world_resource_clear(struct C3dWorldRes *self);
void world_resource_finish(struct C3dWorldRes *self);

/* Called by c3d_world_res_load with the ambient and diffuse light of a map it has loaded.
   client_install_hooks() (core/client_init.c) installs weather_take_map_light
   (actor/weather_hooks.c). NULL: the light is not passed on. */
extern void (*g_worldResLight)(const float ambient[3], const float diffuse[3]);
#endif
