#include "session/state.h"
#include "core/memory.h"
#include "game/context.h"
#include "game/skill_use.h"
#include "effects/rag/rag_effect.h"
#include "effects/skills/skill_view.h"
#include "world/world.h"

enum {
    ACTOR_MSG_CAST_EFFECT = 0x55,
};

static __inline void effect_set_time(struct Effect *effect, float value)
{
    effect->ops->set_stop_frame(effect, value);
}

static void cast_timed(struct GameActor *caster, int effect_id, float time)
{
    struct Effect *effect = spawn_on(caster->aid, effect_id);

    if (effect)
        effect_set_time(effect, time);
}

static void cast_on_cell(int x, int y, int effect_id, unsigned cast_time)
{
    Vec3 at;
    struct Effect *effect;
    float world_x = 0.0f, world_z = 0.0f;
    struct GameMode *gm = game_current_mode();

    if (!gm || !gm->world || !gm->world->attr)
        return;
    at.y = 0.0f;
    game_mode_cell_to_world(gm, (short)x, (short)y, &world_x, &world_z);
    at.y = c3d_attr_get_height(gm->world->attr, world_x, world_z);
    at.x = world_x;
    at.z = world_z;
    effect = spawn_effect(0, effect_id, at);
    if (effect)
        effect_set_time(effect, (float)cast_time);
}

static void cast_by_heading(struct GameActor *caster)
{
    float heading = actor_screen_heading(game_current_mode(), caster);
    int effect_id;

    if (heading > 0.0f && heading < 90.0f)
        effect_id = 0x545;
    else if (heading >= 90.0f && heading < 180.0f)
        effect_id = 0x543;
    else if (heading >= 180.0f && heading < 270.0f)
        effect_id = 0x549;
    else
        effect_id = 0x547;
    if (g_session1.map_rule_flags[0] == 1) {
        if (effect_id == 0x545)
            effect_id = 0x5cf;
        else if (effect_id == 0x543)
            effect_id = 0x5ce;
        else if (effect_id == 0x549)
            effect_id = 0x5d1;
        else if (effect_id == 0x547)
            effect_id = 0x5d0;
        spawn_on(caster->aid, effect_id);
        spawn_on(caster->aid, 0x5d4);
        spawn_on(caster->aid, 0x5d5);
    } else {
        spawn_on(caster->aid, effect_id);
        spawn_on(caster->aid, effect_id + 1);
        spawn_on(caster->aid, 0x568);
        spawn_on(caster->aid, 0x569);
    }
}

static const struct CastPair {
    unsigned short skill_id;
    unsigned short first;
    unsigned short second;
} cast_pairs[] = {
    { 0x7d6, 0x65a, 0 },      { 0x7f6, 0x65d, 0x65e },  { 0x804, 0x617, 0 },      { 0x89a, 0x633, 0x634 },
    { 0x8a4, 0x610, 0 },      { 0x8bc, 0x671, 0 },      { 0x2e1, 0x591, 0x592 },  { 0x911, 0x591, 0x592 },
    { 0x927, 0x625, 0 },      { 0x96d, 0x62d, 0x62e },  { 0x97d, 0x66a, 0x66b },  { 0x9bc, 0x669, 0 },
    { 0x9d5, 0x62a, 0x62b },
};

void skill_cast_play_cast_look(struct GameActor *caster, int x, int y, int skill_id, unsigned cast_time,
                                    int cast_effect)
{
    unsigned short skill = (unsigned short)skill_id;
    float time;
    struct Effect *effect;
    int i;

    if (skill == 0x3e || skill == 0x39)
        return;
    if (effect_mgr_is_registered(g_effectMgr, cast_effect))
        spawn_effect(caster->aid, cast_effect, actor_pos(caster));
    else
        render_object_send(caster, NULL, ACTOR_MSG_CAST_EFFECT, cast_effect, cast_time >> 4, 0, 0);

    time = (float)cast_time;
    switch (skill) {
    case 0x7f0:
        actor_launch_effect(caster, 0x300);
        return;
    case 0x89e:
        actor_launch_effect(caster, 0x323);
        return;
    case 0x8fe:
        actor_launch_effect(caster, 0x10);
        return;
    case 0x89c:
        cast_timed(caster, g_session1.map_rule_flags[0] == 1 ? 0x607 : 0x5a6, time);
        return;
    case 0x8a5:
        cast_timed(caster, g_session1.map_rule_flags[0] == 1 ? 0x600 : 0x587, time);
        cast_timed(caster, g_session1.map_rule_flags[0] == 1 ? 0x603 : 0x58d, time);
        return;
    case 0x8a9:
        cast_timed(caster, g_session1.map_rule_flags[0] == 1 ? 0x5a0 : 0x4fd, time);
        cast_timed(caster, g_session1.map_rule_flags[0] == 1 ? 0x597 : 0x4ed, time);
        return;
    case 0x9ad:
        cast_timed(caster, g_session1.map_rule_flags[0] == 1 ? 0x5db : 0x58e, time);
        cast_timed(caster, g_session1.map_rule_flags[0] == 1 ? 0x5dc : 0x58f, time);
        return;
    case 0x996:
        if (g_session1.map_rule_flags[0] == 0)
            cast_timed(caster, 0x61b, time);
        cast_timed(caster, 0x61c, time);
        return;
    case 0x90d:
        cast_timed(caster, 0x697, time);
        if (g_session1.map_rule_flags[0] == 0)
            cast_timed(caster, 0x698, time);
        return;
    case 0x9b2:
        cast_timed(caster, 0x654, time);
        if (g_session1.map_rule_flags[0] == 0)
            cast_timed(caster, 0x655, time);
        return;
    case 0x8a6:
        spawn_on(caster->aid, 0x663);
        cast_timed(caster, 0x664, time);
        return;
    case 0x8d5:
        cast_by_heading(caster);
        return;
    case 0x9a0:
        play_skill_wave("_heal_effect.wav");
        {
            struct RagEffect *launched = actor_launch_effect(caster, 0xd8);

            if (launched) {
                Vec3 at = actor_pos(caster);

                effect_set_pos(launched, &at);
            }
        }
        return;
    case 0x98f:
        cast_on_cell(x, y, g_session1.map_rule_flags[0] == 1 ? 0x5ac : 0x504, cast_time);
        return;
    case 0x2e0:
    case 0x991:
        cast_on_cell(x, y, g_session1.map_rule_flags[0] == 1 ? 0x5b0 : 0x50a, cast_time);
        return;
    case 0x1f79:
        effect = spawn_on(caster->aid, 0x91d);
        if (effect)
            effect_set_time(effect, (float)(cast_time + 50));
        return;
    }
    for (i = 0; i < (int)(sizeof cast_pairs / sizeof cast_pairs[0]); i++) {
        if (cast_pairs[i].skill_id == skill) {
            cast_timed(caster, cast_pairs[i].first, time);
            if (cast_pairs[i].second)
                cast_timed(caster, cast_pairs[i].second, time);
            return;
        }
    }
}
