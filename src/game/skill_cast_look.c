#include "session/state.h"
#include "core/memory.h"
#include "game/context.h"
#include "game/skill_use.h"
#include "effects/rag/rag_effect.h"
#include "effects/skills/skill_view.h"
#include "world/world.h"
#include "platform/clock.h"

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

static const struct CastPair {
    unsigned short skill_id;
    unsigned short first;
    unsigned short second;
} cast_pairs[] = {
    { 0x2e1, 0x591, 0x592 },
};

void skill_cast_play_cast_look(struct GameActor *caster, int x, int y, int skill_id, unsigned cast_time,
                                    int cast_effect)
{
    unsigned short skill = (unsigned short)skill_id;
    float time;
    int i;

    if (skill == 0x3e || skill == 0x39)
        return;
    if (effect_mgr_is_registered(g_effectMgr, cast_effect))
        spawn_effect(caster->aid, cast_effect, actor_pos(caster));
    else
        render_object_send(caster, NULL, ACTOR_MSG_CAST_EFFECT, cast_effect, cast_time >> 4, 0, 0);

    time = (float)cast_time;
    switch (skill) {
    case 0x2e0:
        cast_on_cell(x, y, g_session1.map_rule_flags[0] == 1 ? 0x5b0 : 0x50a, cast_time);
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
