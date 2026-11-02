#include "../src/core/map_sky.c"
#include "effects/skills/cloud.h"
#include "contents/client_lua.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct EffectMgr manager;
struct EffectMgr *g_effectMgr = &manager;
static struct Cloud cloud, expected_cloud;
static int map_present, clouds_to_make, spawn_calls, restart_calls, lua_calls;
static int missing_effect;

void *mem_alloc(size_t bytes) { return calloc(1, bytes); }
__declspec(noreturn) void fatal_error(const char *message)
{ fputs(message, stderr); abort(); }

int game_map_dimensions(int *width, int *height)
{ *width = 120; *height = 80; return map_present; }

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;
    const char *map;
    (void)lua;
    ++lua_calls;
    va_start(args, format);
    map = va_arg(args, const char *);
    assert(!strcmp(map, "test"));
    if (!strcmp(function, "GetCloudTypeNum")) {
        assert(!strcmp(format, "s>d"));
        *va_arg(args, int *) = 1;
    } else {
        assert(!strcmp(function, "GetCloudInfo"));
        assert(!strcmp(format, "sd>dddddlllddlddlll"));
        assert(va_arg(args, int) == 1);
        *va_arg(args, int *) = clouds_to_make;
        *va_arg(args, int *) = 177;
        *va_arg(args, int *) = 10;
        *va_arg(args, int *) = 20;
        *va_arg(args, int *) = 30;
        *va_arg(args, double *) = 1.5;
        *va_arg(args, double *) = 2.5;
        *va_arg(args, double *) = 0.25;
        *va_arg(args, int *) = 3;
        *va_arg(args, int *) = 4;
        *va_arg(args, double *) = 0.5;
        *va_arg(args, int *) = 5;
        *va_arg(args, int *) = 6;
        *va_arg(args, double *) = 0.75;
        *va_arg(args, double *) = 12.5;
        *va_arg(args, double *) = 7.5;
    }
    va_end(args);
    return 1;
}

void cloud_respawn(struct Effect *effect)
{
    assert(effect == &cloud.base);
    assert(!memcmp(&cloud, &expected_cloud, sizeof cloud));
    ++restart_calls;
}

struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos,
                                int target_aid, int level, unsigned char reuse, int key)
{
    assert(self == &manager && !aid && !target_aid && level == -1 && !reuse && !key);
    ++spawn_calls;
    if (type == 0x47a) {
        assert(pos.y == 12.5f && pos.x > -60 && pos.x < 60 && pos.z > -40 && pos.z < 40);
        return missing_effect ? NULL : &cloud.base;
    }
    assert(0);
    return NULL;
}


static void test_cloud_setup(void)
{
    char map_name[] = "test";
    struct String map = {0};
    struct MapSkyMgr sky = {0};
    map.data = map_name;
    map.size = map.capacity = 4;
    memset(&cloud, 0x5a, sizeof cloud);
    expected_cloud = cloud;
    expected_cloud.base.view_range = 177;
    expected_cloud.color[0] = 10; expected_cloud.color[1] = 20; expected_cloud.color[2] = 30;
    expected_cloud.size_min = 1.5f; expected_cloud.size_range = 2.5f; expected_cloud.pulse = 0.25f;
    expected_cloud.fade_in_min = 3; expected_cloud.fade_in_range = 4;
    expected_cloud.fade_in_step = 0.5f;
    expected_cloud.fade_out_min = 5; expected_cloud.fade_out_range = 6;
    expected_cloud.fade_out_step = 0.75f; expected_cloud.spread_y = 7.5f;
    expected_cloud.spread_x = 60; expected_cloud.spread_z = 40;
    map_present = clouds_to_make = 1;
    map_sky_mgr_make_clouds(&sky, &map);
    assert(spawn_calls == 1 && restart_calls == 1 && lua_calls == 2);
    missing_effect = 1;
    map_sky_mgr_make_clouds(&sky, &map);
    assert(spawn_calls == 2 && restart_calls == 1);
    clouds_to_make = 0;
    map_sky_mgr_make_clouds(&sky, &map);
    assert(spawn_calls == 2);
    map_present = 0; lua_calls = 0;
    map_sky_mgr_make_clouds(&sky, &map);
    assert(!lua_calls && spawn_calls == 2);
}

int main(void)
{
    test_cloud_setup();
    puts("Named client dispatch: cloud settings and restart pass.");
    return 0;
}

#include "named_client_dispatch_unreached.h"
