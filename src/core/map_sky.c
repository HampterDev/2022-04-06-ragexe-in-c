#include "game/mode_misc.h"
#include "effects/effect_mgr.h"
#include "effects/skills/cloud.h"
#include "contents/client_lua.h"
#include "actor/weather.h"
#include <stdlib.h>
#include "render/renderer.h"
#include <string.h>
#include "core/map_sky.h"
#include "core/client.h"
#include "game/context.h"

#include "render/vec3.h"

static struct MapSkyMgr *g_mapSkyMgrInstance;

struct MapSkyMgr *map_sky_mgr_instance(void)
{
    if (!g_mapSkyMgrInstance) {
        struct MapSkyMgr *manager = mem_try_alloc(sizeof *manager);
        g_mapSkyMgrInstance = manager ? map_sky_mgr_construct(manager) : NULL;
    }
    return g_mapSkyMgrInstance;
}

void map_sky_mgr_destroy(void)
{
    if (!g_mapSkyMgrInstance)
        return;
    map_sky_mgr_destruct(g_mapSkyMgrInstance);
    mem_free(g_mapSkyMgrInstance);
    g_mapSkyMgrInstance = NULL;
}

struct MapSkyMgr *map_sky_mgr_construct(struct MapSkyMgr *self)
{
    struct ClientLua *lua;

    self->lua = NULL;
    lua = malloc(sizeof *lua);
    lua = lua ? client_lua_construct(lua) : NULL;
    self->lua = lua;
    client_lua_open(lua);
    return self;
}

void map_sky_mgr_destruct(struct MapSkyMgr *self)
{
    if (self->lua) {
        client_lua_destruct(self->lua);
        free(self->lua);
        self->lua = NULL;
    }
}

unsigned char map_sky_mgr_load(struct MapSkyMgr *self)
{
    return client_lua_load_file(self->lua, "Lua Files\\MapSkyData\\MapSkyData", 1, 0);
}

static DWORD sky_color(int r, int g, int b)
{
    return (((((DWORD)(BYTE)r + 0xff00) << 8) + (BYTE)g) << 8) + (BYTE)b;
}

void map_sky_mgr_set_background(struct MapSkyMgr *self, const struct String *map)
{
    int r = 0, g = 0, b = 0;

    client_lua_call(self->lua, "GetBackgroundColor", "s>ddd", string_cstr(map), &r, &g, &b);
    g_renderer->clearColor = sky_color(r, g, b);
}

static int sky_rand_mod(int n)
{
    int value = rand() % n;

    return value >= 0 ? value : 0;
}

void map_sky_mgr_make_clouds(struct MapSkyMgr *self, const struct String *map)
{
    int types = 0, type, i, count, view_range, r, g, b, fade_in_min, fade_in_range, fade_out_min, fade_out_range;
    double size_min, size_range, pulse, fade_in_step, fade_out_step, cloud_y, spread_y;
    int width, height, half_w, half_h, sign, offset;
    float cell_w, cell_h, x, z;
    struct Cloud *cloud;
    Vec3 pos;

    if (!game_map_dimensions(&width, &height))
        return;
    client_lua_call(self->lua, "GetCloudTypeNum", "s>d", string_cstr(map), &types);

    for (type = 1; type <= types; type++) {
        count = 0;
        view_range = 0x82;
        r = g = b = 0;
        fade_in_min = fade_in_range = fade_out_min = fade_out_range = 0;
        size_min = size_range = pulse = fade_in_step = fade_out_step = cloud_y = spread_y = 0.0;
        client_lua_call(self->lua, "GetCloudInfo", "sd>dddddlllddlddlll", string_cstr(map), type, &count,
                &view_range, &r, &g, &b, &size_min, &size_range, &pulse, &fade_in_min, &fade_in_range, &fade_in_step,
                &fade_out_min, &fade_out_range, &fade_out_step, &cloud_y, &spread_y);
        cell_w = (float)width / (float)(count + 1);
        cell_h = (float)height / (float)(count + 1);
        if (count <= 0)
            continue;
        half_w = (int)((float)width * 0.5f);
        half_h = (int)((float)height * 0.5f);
        for (i = 0; i < count; i++) {
            sign = rand() % 2;
            if (sign < 0)
                sign = 0;
            offset = half_w ? sky_rand_mod(half_w) : 0;
            x = (float)((1 - sign * 2) * offset);
            sign = rand() % 2;
            if (sign < 0)
                sign = 0;
            offset = half_h ? sky_rand_mod(half_h) : 0;
            z = (float)((1 - sign * 2) * offset);
            pos.x = x;
            pos.y = (float)cloud_y;
            pos.z = z;
            cloud = (struct Cloud *)effect_mgr_spawn(g_effectMgr, 0, 0x47a, pos, 0, -1, 0, 0);
            if (!cloud)
                continue;
            cloud->base.view_range = view_range;
            cloud->color[0] = (BYTE)r;
            cloud->color[1] = (BYTE)g;
            cloud->color[2] = (BYTE)b;
            cloud->size_min = (float)size_min;
            cloud->size_range = (float)size_range;
            cloud->pulse = (float)pulse;
            cloud->fade_in_min = fade_in_min;
            cloud->fade_in_range = fade_in_range;
            cloud->fade_out_min = fade_out_min;
            cloud->fade_out_range = fade_out_range;
            cloud->fade_in_step = (float)fade_in_step;
            cloud->fade_out_step = (float)fade_out_step;
            cloud->spread_y = (float)spread_y;
            cloud->spread_x = cell_w;
            cloud->spread_z = cell_h;
            cloud_respawn(&cloud->base);
        }
    }
}

void map_sky_mgr_old_clouds(struct MapSkyMgr *self, const struct String *map)
{
    int count = 0, i, kind;

    client_lua_call(self->lua, "GetOldCloudTypeNum", "s>d", string_cstr(map), &count);
    for (i = 1; i <= count; i++) {
        kind = -1;
        client_lua_call(self->lua, "GetOldCloudInfo", "sd>d", string_cstr(map), i, &kind);
        if (kind != -1)
            weather_launch(&g_weather, kind);
    }
}

void map_sky_mgr_stars(struct MapSkyMgr *self, const struct String *map)
{
    BYTE draw = 0;

    client_lua_call(self->lua, "IsDrawStar", "s>b", string_cstr(map), &draw);
    if (draw == 1)
        weather_launch(&g_weather, 9);
}

void map_sky_mgr_apply(struct MapSkyMgr *self, const struct String *map)
{
    struct String arg, name;
    const char *sky_name;
    BYTE exists = 0;

    string_init(&arg);
    string_assign_cstr(&arg, string_cstr(map));
    sky_name = game_mode_short_name(arg);
    string_init(&name);
    string_assign_cstr(&name, sky_name);
    /* No manager (its allocation failed): no sky data, as when the file did not load. */
    if (self)
        client_lua_call(self->lua, "IsExistMapSkyData", "s>b", string_cstr(&name), &exists);
    if (!exists) {
        g_renderer->clearColor = 0xff000000;
    } else {
        map_sky_mgr_set_background(self, &name);
        map_sky_mgr_stars(self, &name);
        map_sky_mgr_make_clouds(self, &name);
        map_sky_mgr_old_clouds(self, &name);
    }
    string_free(&name);
}
