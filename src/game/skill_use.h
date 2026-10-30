#include "contents/sound.h"
#include "game/context.h"
#ifndef GAMEMODE_SKILL_USE_H
#define GAMEMODE_SKILL_USE_H

#include "game/skill_fx.h"
#include "game/client.h"
#include "actor/actor_common.h"

struct SkillCast {
    struct GameMode *mode;
    int skill_id;
    int level;
    int effect_level;
    int src_aid;
    int target_aid;
    struct GameActor *caster;
    struct GameActor *target;
};

int skill_use_basic(const struct SkillCast *cast);
int skill_use_second(const struct SkillCast *cast);
int skill_use_extra(const struct SkillCast *cast);
int skill_use_third(const struct SkillCast *cast);
int skill_use_third2(const struct SkillCast *cast);
int skill_use_late(const struct SkillCast *cast);
int skill_use_fourth(const struct SkillCast *cast);
int skill_use_companion(const struct SkillCast *cast);

enum {
    ON_CASTER = 0,
    ON_TARGET = 1,
};
struct SimpleLook {
    unsigned short skill_id;
    unsigned short effect_id;
    unsigned char on;
};
#define SIMPLE_LOOK_COUNT(looks) ((int)(sizeof(looks) / sizeof((looks)[0])))
int play_simple_look(const struct SkillCast *cast, const struct SimpleLook *looks, int count);

struct WaveLook {
    unsigned short skill_id;
    unsigned short effect_id;
    unsigned char on;
    unsigned char wave_first;
    const char *wave;
};
int play_wave_look(const struct SkillCast *cast, const struct WaveLook *looks, int count);

void skill_use_counted_effect(struct GameActor *actor);

void skill_use_heal(const struct SkillCast *cast);

void skill_use_ninja_throw_look(const struct SkillCast *cast);
void skill_use_soul_strike_look(const struct SkillCast *cast);
void skill_use_water_look(const struct SkillCast *cast);
void skill_use_glow_both_look(const struct SkillCast *cast);
void skill_use_hallucination_walk_look(const struct SkillCast *cast);
void skill_use_jack_frost_ring_look(const struct SkillCast *cast);

enum {
    MODE_MSG_SKILL_CAST = 0x56,
    MODE_MSG_QUAKE = 0x7a,
    ACTOR_MSG_SMOKE = 0x3f,
    ACTOR_MSG_SHOW_NUMBER = 0x58,
};

static __inline void caster_effect(const struct SkillCast *cast, int effect_id)
{
    if (cast->caster)
        actor_launch_effect(cast->caster, effect_id);
}
static __inline void target_effect(const struct SkillCast *cast, int effect_id)
{
    if (cast->target)
        actor_launch_effect(cast->target, effect_id);
}

static __inline void actor_remove_effect(struct GameActor *actor, int effect_id)
{
    render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, effect_id, 0, 0, 0);
}
static __inline struct RagEffect *actor_restart_effect(struct GameActor *actor, int effect_id)
{
    actor_remove_effect(actor, effect_id);
    return actor_launch_effect(actor, effect_id);
}
static __inline void actor_remove_all_effects(struct GameActor *actor, int effect_id)
{
    while (able_to_make_effect_is_effect_exist(actor, effect_id))
        actor_remove_effect(actor, effect_id);
}

static __inline void actor_show_number(struct GameActor *actor, int value, unsigned colour)
{
    render_object_send(actor, NULL, ACTOR_MSG_SHOW_NUMBER, value, colour, 0x72, 0);
}

static __inline void actor_send(struct GameActor *actor, int msg)
{
    render_object_send(actor, NULL, msg, 0, 0, 0, 0);
}

char game_mode_launch_effect_at(struct GameMode *mode, int effect_id,
                                        struct GameActor *caster, struct GameActor *target, int level);

static __inline void send_quake(void)
{
    game_send_message(MODE_MSG_QUAKE, 0, 0, 0, 0);
}

static __inline void play_skill_wave_at(const char *wave, float x, float y, float z)
{
    sound_play_wave(wave, x, y, z, 250, 40, 1.0f);
}

static __inline Vec3 actor_pos(const struct GameActor *actor)
{
    Vec3 at;

    at.x = actor->pos.x;
    at.y = actor->pos.y;
    at.z = actor->pos.z;
    return at;
}

static __inline void effect_place(struct RagEffect *effect, Vec3 at)
{
    if (effect)
        effect_set_pos(effect, &at);
}

static __inline Vec3 aid_as_pos(const struct GameActor *actor)
{
    Vec3 at;

    at.x = 0.0f;
    at.y = (float)(unsigned)actor->aid;
    at.z = 0.0f;
    return at;
}

void game_mode_show_item_use_effect(struct GameMode *self, int item_id, unsigned aid);

#endif
