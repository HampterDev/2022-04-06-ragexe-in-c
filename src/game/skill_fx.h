#ifndef GAMEMODE_SKILL_FX_H
#define GAMEMODE_SKILL_FX_H

#include "game/packet_handlers.h"
#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "game/world_objects.h"
#include "world/object_lookup.h"
#include "resources/attr_queries.h"
#include "contents/sound.h"
#include "effects/durations.h"

#include "game/gamemode.h"
#include "game/session_bridge.h"
#include "game/cells.h"
#include "game/camera.h"
#include <stdint.h>

static __inline void spawned_effect_delay(struct Effect *effect, int delay_ms)
{
    effect->ops->set_start_delay(effect, delay_ms);
}
static __inline void spawned_effect_zoom(struct Effect *effect, float zoom)
{
    effect->ops->set_zoom(effect, zoom);
}

static __inline struct Effect *spawn_effect(int aid, int effect_id, Vec3 at)
{
    return effect_mgr_spawn(g_effectMgr, aid, effect_id, at, 0, -1, 0, 0);
}
static __inline struct Effect *spawn_on(int aid, int effect_id)
{
    Vec3 origin = { 0.0f, 0.0f, 0.0f };
    return effect_mgr_spawn(g_effectMgr, aid, effect_id, origin, 0, -1, 0, 0);
}
static __inline struct Effect *spawn_on_delayed(int aid, int effect_id, int delay_ms)
{
    struct Effect *effect = spawn_on(aid, effect_id);
    if (effect)
        spawned_effect_delay(effect, delay_ms);
    return effect;
}

static __inline Vec3 cell_on_ground(struct GameMode *mode, int x, int y)
{
    Vec3 at;
    float world_x, world_z;

    game_mode_cell_to_world(mode, x, y, &world_x, &world_z);
    at.x = world_x;
    at.y = c3d_attr_get_height(mode->world->attr, world_x, world_z);
    at.z = world_z;
    return at;
}

static __inline void play_skill_wave(const char *wave)
{
    sound_play_wave(wave, 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
}

static __inline struct RagEffect *actor_launch_effect(struct GameActor *actor, int effect_id)
{
    Vec3 here = { 0.0f, 0.0f, 0.0f };
    return game_actor_launch_effect(actor, effect_id, here, 0.0f);
}

enum {
    EFFECT_MSG_SET_POS = 0xe,
};
static __inline void effect_set_pos(struct RagEffect *effect, const Vec3 *at)
{
    rag_effect_send_msg(effect, NULL, EFFECT_MSG_SET_POS, (intptr_t)at, 0, 0);
}

static __inline float actor_screen_heading(struct GameMode *mode, struct GameActor *actor)
{
    float facing = actor->roty;
    float heading = (float)(game_mode_get_view_heading(mode) + (double)facing);

    if (heading >= 360.0f)
        heading -= 360.0f;
    return heading;
}

struct PacketNotifyGroundSkill;

void skill_hit_caster_effects(struct GameMode *mode, int skill_id, struct GameActor *caster,
                              struct GameActor *target);
void skill_hit_target_effects(int skill_id, int level, struct GameActor *caster, struct GameActor *target);

struct PacketNotifySkill;
struct PacketNotifySkillPosition;
struct PacketNotifySkill2;

#endif
