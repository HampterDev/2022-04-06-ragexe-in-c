#ifndef CORE_MAP_SKY_H
#define CORE_MAP_SKY_H

#include "platform/win_types.h"
#include "base.h"
#include "std/std.h"

struct ClientLua;
struct MapSkyMgr {
    struct ClientLua *lua;
};

struct MapSkyMgr *map_sky_mgr_instance(void);
void map_sky_mgr_destroy(void);

struct MapSkyMgr *map_sky_mgr_construct(struct MapSkyMgr *self);
void map_sky_mgr_destruct(struct MapSkyMgr *self);
unsigned char map_sky_mgr_load(struct MapSkyMgr *self);
void map_sky_mgr_set_background(struct MapSkyMgr *self, const struct String *map);
void map_sky_mgr_make_clouds(struct MapSkyMgr *self, const struct String *map);
void map_sky_mgr_apply(struct MapSkyMgr *self, const struct String *map);
void map_sky_mgr_old_clouds(struct MapSkyMgr *self, const struct String *map);
void map_sky_mgr_stars(struct MapSkyMgr *self, const struct String *map);

#endif
