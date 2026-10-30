#include "../src/ui/render/render_target.c"
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
static struct Effect render_effect;
static struct EffectOps fake_render_ops;
static struct UIRenderTargetWnd window;
static ParticleSubmitFn submitted;
static int map_present, clouds_to_make, spawn_calls, restart_calls, lua_calls;
static int missing_effect, window_present;

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

static void register_submit(struct Effect *effect, ParticleSubmitFn submit)
{ assert(effect == &render_effect); submitted = submit; }

struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos,
                                int target_aid, int level, unsigned char reuse, int key)
{
    assert(self == &manager && !aid && !target_aid && level == -1 && !reuse && !key);
    ++spawn_calls;
    if (type == 0x47a) {
        assert(pos.y == 12.5f && pos.x > -60 && pos.x < 60 && pos.z > -40 && pos.z < 40);
        return missing_effect ? NULL : &cloud.base;
    }
    assert(type == 0x442 && !pos.x && !pos.y && !pos.z);
    return missing_effect ? NULL : &render_effect;
}

struct CharacterRender *character_render_create(int width, int height)
{ assert(width == 200 && height == 100); return mem_alloc(sizeof(struct CharacterRender)); }
void character_render_set_robe(struct CharacterRender *self, int robe)
{ assert(self && robe == 0x11); }
struct UIWindow *ui_window_find(int id)
{ assert(id == 0x125); return window_present ? &window.frame.base : NULL; }

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

static void test_effect_callback(void)
{
    void *effects_head[3], *quads_head[3];
    struct TexDrawPrim first = {0}, second = {0};
    void **node, **next;
    effects_head[0] = effects_head[1] = effects_head;
    quads_head[0] = quads_head[1] = quads_head;
    window.effects.head = (struct ListNode *)effects_head;
    window.effect_quads.head = (struct ListNode *)quads_head;
    fake_render_ops.set_submit = register_submit;
    render_effect.ops = &fake_render_ops;
    missing_effect = 0; window_present = 1;
    ui_render_target_wnd_on_create((struct UIWindow *)(&window), 200, 100);
    assert(submitted && window.effects.size == 1);
    assert(((void **)effects_head[0])[2] == &render_effect);
    submitted(&first, 0x1234);
    submitted(&second, -1);
    assert(window.effect_quads.size == 2);
    node = quads_head[0]; assert(node[2] == &first);
    node = node[0]; assert(node[2] == &second && node[0] == quads_head);
    window_present = 0;
    submitted(&first, 0);
    assert(window.effect_quads.size == 2);
    for (node = quads_head[0]; node != quads_head; node = next) {
        next = node[0]; free(node);
    }
    free(effects_head[0]); free(window.character);
}

int main(void)
{
    test_cloud_setup();
    test_effect_callback();
    puts("Named client dispatch: cloud settings and restart, effect callback registration and quad order pass.");
    return 0;
}

#include "named_client_dispatch_unreached.h"
