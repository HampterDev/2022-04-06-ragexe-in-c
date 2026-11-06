#include "world/ground_api.h"
#include "resources/gnd.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/gamemode.h"
#include "game/context.h"
#include "world/world.h"
#include "world/mode.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "actor/actor_ops.h"
#include "core/app.h"
#include "effects/skills/skill_view.h"
#include "ui/wndmgr/wndmgr.h"
#include "std/list.h"

static int world_load_from(struct World *self, struct C3dWorldRes *rsw)
{
    struct GndRes *gnd;
    struct C3dGround15 *ground;
    size_t index;
    unsigned count;
    int lastPct;
    struct WorldLighting lighting;
    float playerPos[3] = { 0 };

    self->videoList = world_video_list_create();
    if (!self->videoList) return 0;
    self->prebuilt = rsw->tree;
    game_set_map_bounds(self->mode, rsw->bounds);
    lighting.direction[0] = rsw->lightDir[0];
    lighting.direction[1] = rsw->lightDir[1];
    lighting.direction[2] = rsw->lightDir[2];
    lighting.diffuse[0] = rsw->diffuse[0];
    lighting.diffuse[1] = rsw->diffuse[1];
    lighting.diffuse[2] = rsw->diffuse[2];
    lighting.ambient[0] = rsw->ambient[0];
    lighting.ambient[1] = rsw->ambient[1];
    lighting.ambient[2] = rsw->ambient[2];
    g_renderer->twoPassAlpha = rsw->twoPassAlpha;
    renderer_set_light(g_renderer, lighting.direction, lighting.diffuse, lighting.ambient);

    if (rsw->gatFile[0] == 0) {
        error_msg("attr 파일이 rsw 내에 지정되어 "
                 "있지 않습니다", 0);
        return 0;
    }
    self->attr = res_mgr_get(g_resMgr(), rsw->gatFile);
    if (!self->attr) {
        error_msg("attr 파일 읽기에 실패", 0);
        return 0;
    }
    self->attr->zoom = 5;
    self->nearRange *= self->attr->zoom;

    if (rsw->gndFile[0] == 0) {
        error_msg("gnd 파일이 rsw 내에 지정되어 "
                 "있지 않습니다", 0);
        return 0;
    }
    gnd = res_mgr_get(g_resMgr(), rsw->gndFile);
    if (!gnd) {
        error_msg("ground에 NULL이 들어왔습니다.", 0);
        return 0;
    }

    ground = ground_new();
    if (!ground) {
        res_mgr_release(g_resMgr(), &gnd->res);
        return 0;
    }
    if (!c3d_ground15_load(ground, gnd, lighting.direction, lighting.diffuse, lighting.ambient)) {
        ground_free(ground);
        res_mgr_release(g_resMgr(), &gnd->res);
        return 0;
    }
    ground_set_attribute_map(ground, self->attr);
    {
        struct GroundWaterSettings water;
        const struct GroundWaterSettings *scene_water = NULL;
        if (gnd->major != 1 || gnd->minor < 8) {
            water.level = rsw->waterLevel;
            water.type = rsw->waterType;
            water.waveHeight = rsw->waveHeight;
            water.waveSpeed = rsw->waveSpeed;
            water.wavePitch = rsw->wavePitch;
            water.frameDelay = rsw->waterAnimSpeed;
            scene_water = &water;
        }
        if (!c3d_ground15_load_water(ground, gnd, scene_water)) {
            ground_free(ground);
            res_mgr_release(g_resMgr(), &gnd->res);
            return 0;
        }
    }
    self->ground = ground;
    res_mgr_release(g_resMgr(), &gnd->res);

    count = rsw->modelCount;
    if (!world_models_prepare(self, count, &lighting))
        return 0;
    lastPct = -1;
    for (index = 0; index < rsw->modelCount; ++index) {
        struct RswModel *m;
        float dx, dy, dz, dist;
        struct C3dActor *actor;
        int pct;

        pct = (int)(((uint64_t)index + 1) * 100 / count);
        if (lastPct != pct) {
            lastPct = pct;
            ui_window_mgr_send_msg_(&g_windowMgr, 4, pct, 0, 0, 0);
            ui_window_mgr_render(&g_windowMgr);
            ui_window_mgr_render_windows(&g_windowMgr);
            if ((BYTE)renderer_draw_scene(g_renderer))
                renderer_flip(g_renderer, 0);
            check_system_message();
        }

        m = &rsw->models[index];
        game_mode_cell_to_world(game_current_mode(), g_session1.playerPosX, g_session1.playerPosY,
                                &playerPos[0], &playerPos[2]);
        dx = m->pos[0] - playerPos[0];
        dy = m->pos[1] - playerPos[1];
        dz = m->pos[2] - playerPos[2];
        dist = sqrtf(dy * dy + dx * dx + dz * dz);
        if (dist > WORLD_NEAR_MODEL_DIST) {
            if (!world_models_queue(self, m, dist, index))
                return 0;
            continue;
        }

        actor = world_model_create(m, &lighting);
        if (!actor)
            continue;
        if (!world_models_add(self, actor)) {
            world_model_destroy(actor);
            return 0;
        }
    }

    if (!world_build_scene_graph(self))
        return 0;
    world_load_effects_and_sounds(self);
    c3d_ground15_build_vertex_colours(self->ground);
    world_models_start(self);
    if (!world_create_player(self)) {
        error_msg("Cannot create the local player.", 0);
        return 0;
    }
    g_session1.game_over = 0;
    return 1;
}

int world_load(struct World *self)
{
    struct String name;
    const char *text;
    struct C3dWorldRes *rsw;
    int loaded = 0;

    text = (const char *)mode_send_msg(self->mode, 9, 0, 0, 0, 0);
    string_init(&name);
    string_assign_cstr(&name, text);
    rsw = res_mgr_get(g_resMgr(), string_cstr(&name));
    if (!rsw) {
        error_msg("world에 NULL값이 들어왔습니다", 0);
    } else {
        loaded = world_load_from(self, rsw);
    }
    string_free(&name);
    return loaded;
}

void world_load_effects_and_sounds(struct World *self)
{
    struct String name;
    const char *text;
    struct C3dWorldRes *rsw;
    size_t index;

    text = (const char *)mode_send_msg(self->mode, 9, 0, 0, 0, 0);
    string_init(&name);
    string_assign_cstr(&name, text);
    rsw = res_mgr_get(g_resMgr(), string_cstr(&name));
    for (index = 0; index < rsw->effectCount; ++index) {
        struct RswEffect *e = &rsw->effects[index];
        if (effect_mgr_is_registered(g_effectMgr, e->id)) {
            Vec3 pos;
            pos.x = e->pos[0];
            pos.y = e->pos[1];
            pos.z = e->pos[2];
            effect_mgr_spawn(g_effectMgr, 0, e->id, pos, 0, -1, 0, 0);
        } else {
            void *obj = effect_maker_new();
            render_object_send(obj, NULL, 0xe, MSG_ARG_PTR(e), 0, 0, 0);
            list_push_back(&self->gameObjects, &obj, sizeof obj);
        }
    }
    effect_mgr_load_effect_tool(g_effectMgr);
    if (g_soundMode) {
        for (index = 0; index < rsw->soundCount; ++index) {
            const struct RswSound *src = &rsw->sounds[index];
            void *p = mem_alloc(sizeof(struct SoundMaker));
            void *obj = p ? world_sound_construct(p) : NULL;
            render_object_send(obj, NULL, 0xe, MSG_ARG_PTR(src), 0, 0, 0);
            list_push_back(&self->gameObjects, &obj, sizeof obj);
        }
    }
    string_free(&name);
}
