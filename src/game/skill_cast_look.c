#include "skills/skill_action.h"
#include "skills/skill_action_registry.h"
#include "session/state.h"
#include "core/memory.h"
#include "game/context.h"
#include "platform/win_types.h"
#include "game/skill_use.h"
#include "effects/rag/rag_effect.h"
#include "effects/skills/skill_view.h"
#include "std/std.h"
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

static void cast_ground_scope(struct GameActor *caster, int skill_id, unsigned cast_time)
{
    void *scope;
    int cell_x, cell_y;
    struct GameMode *mode = game_current_mode();

    if (!caster || !mode || !mode->world || !mode->world->attr)
        return;
    scope = ground_scope_new();
    if (!scope)
        return;
    game_mode_world_to_cell(mode, caster->pos.x, caster->pos.z, &cell_x, &cell_y);
    render_object_send(scope, NULL, 0x16, (unsigned)caster->aid, 0, 0, 0);
    render_object_send(scope, NULL, 0xe, cell_x, cell_y, 0, 0);
    render_object_send(scope, NULL, 0x73, (unsigned)(platform_ticks_ms() + cast_time), 0, 0, 0);
    render_object_send(scope, NULL, 0x75, (unsigned short)skill_id, 0, 0, 0);
    list_push_back(&mode->world->gameObjects, &scope, sizeof scope);
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
    { 0x9d5, 0x62a, 0x62b },  { 0x145e, 0x6ca, 0x6cb }, { 0x1460, 0x6d9, 0x6da }, { 0x1461, 0x6ed, 0x6ee },
    { 0x1462, 0x6e1, 0x6e2 }, { 0x1464, 0x6ce, 0x6cf }, { 0x1465, 0x762, 0x763 }, { 0x1466, 0x704, 0x705 },
    { 0x1469, 0x740, 0x741 }, { 0x146b, 0x6f2, 0x6f3 }, { 0x146d, 0x70d, 0x70e }, { 0x146e, 0x6e9, 0x6ea },
    { 0x1471, 0x6d3, 0x6d4 }, { 0x1473, 0x6dd, 0x6de }, { 0x1475, 0x6f6, 0x6f7 }, { 0x147e, 0x7b8, 0 },
    { 0x147f, 0x7bb, 0 },     { 0x1486, 0x7bf, 0 },     { 0x1495, 0x711, 0 },     { 0x1497, 0x714, 0 },
    { 0x1498, 0x716, 0 },     { 0x149b, 0x718, 0 },     { 0x149f, 0x7a6, 0 },     { 0x14a1, 0x745, 0 },
    { 0x14a5, 0x74c, 0 },     { 0x14a6, 0x723, 0x724 }, { 0x14b6, 0x78f, 0 },     { 0x14b7, 0x77c, 0 },
    { 0x14b8, 0x76d, 0 },     { 0x14b9, 0x774, 0 },     { 0x14c4, 0x7de, 0x7df }, { 0x14c6, 0x84f, 0x850 },
    { 0x14d0, 0x7a9, 0 },     { 0x14d1, 0x7b1, 0 },     { 0x14d2, 0x7b4, 0 },     { 0x14d3, 0x751, 0 },
    { 0x14d4, 0x758, 0x759 }, { 0x14d5, 0x755, 0 },     { 0x14d6, 0x79e, 0 },     { 0x14d7, 0x781, 0 },
    { 0x14e7, 0x7d1, 0 },     { 0x14e8, 0x7d7, 0x7d8 }, { 0x14f7, 0x7d4, 0x7d5 }, { 0x14f8, 0x7c9, 0x7ca },
    { 0x1500, 0x7e5, 0x7e6 }, { 0x1503, 0x7da, 0x7db },
};

static const struct CastMapEntry {
    unsigned short skill_id;
    unsigned short first;
    unsigned short second;
} cast_map_entries[] = {
    { 0x14f9, 0x7ec, 0x7ed }, { 0x14e9, 0x7fe, 0x7ff }, { 0x14fb, 0x7f0, 0x7f1 }, { 0x14ff, 0x7f5, 0x7f6 },
    { 0x14fa, 0x7fb, 0x7fc }, { 0x14fc, 0x809, 0x80a }, { 0x14ed, 0x80d, 0x80e }, { 0x14f2, 0x812, 0x813 },
    { 0x14f4, 0x815, 0x816 }, { 0x14eb, 0x817, 0x818 }, { 0x1501, 0x834, 0x835 }, { 0x1502, 0x839, 0x83a },
    { 0x14c7, 0x829, 0x82a }, { 0x14f0, 0x84d, 0 },     { 0x14ee, 0x845, 0x846 }, { 0x14ef, 0x848, 0x849 },
    { 0x14fd, 0x857, 0x858 }, { 0x1504, 0x85a, 0 },     { 0x14c3, 0x86b, 0 },     { 0x14c9, 0x86b, 0 },
    { 0x1492, 0x86f, 0 },     { 0x14db, 0x873, 0x874 }, { 0x1493, 0x877, 0x878 }, { 0x1487, 0x87a, 0x87b },
    { 0x148c, 0x87d, 0x87e }, { 0x148f, 0x882, 0x883 }, { 0x148e, 0x884, 0 },     { 0x14da, 0x88b, 0 },
    { 0x1490, 0x891, 0x892 }, { 0x1489, 0x896, 0x897 }, { 0x14e4, 0x8bc, 0x8bd }, { 0x14df, 0x89c, 0x89d },
    { 0x14dc, 0x8a4, 0x8a5 }, { 0x14de, 0x8ac, 0x8ad }, { 0x14dd, 0x8b4, 0x8b5 }, { 0x14e3, 0x8c2, 0x8c3 },
    { 0x14e0, 0x8c7, 0x8c8 }, { 0x14e1, 0x8ce, 0x8d0 }, { 0x14e2, 0x8c7, 0x8c8 },
};

static void cast_from_map(struct Tree *map, struct GameActor *caster, long long key, unsigned cast_time)
{
    struct Int64Pair *entry = tree_find(map, &key, tree_less_int64);

    if (entry) {
        struct Effect *effect = spawn_on(caster->aid, (int)entry->value);

        if (effect)
            effect_set_time(effect, (float)cast_time);
    }
}

static void set_cast_effect(struct Tree *map, long long key, long long effect)
{
    struct Int64Pair *entry = tree_get(map, &key, sizeof key, sizeof *entry, tree_less_int64);

    entry->value = effect;
}

static void cast_look_by_table(struct GameActor *caster, unsigned short skill, unsigned cast_time)
{
    struct Tree first, second;
    long long key;
    int i;

    if (skill_action_mgr_cast(skill_action_mgr_instance(), skill, cast_time, (SkillActor *)(caster)) == 1)
        return;
    tree_init(&first);
    tree_init(&second);
    for (i = 0; i < (int)(sizeof cast_map_entries / sizeof cast_map_entries[0]); i++) {
        set_cast_effect(&first, cast_map_entries[i].skill_id, cast_map_entries[i].first);
        if (cast_map_entries[i].second)
            set_cast_effect(&second, cast_map_entries[i].skill_id, cast_map_entries[i].second);
    }
    key = skill;
    cast_from_map(&first, caster, key, cast_time);
    cast_from_map(&second, caster, key, cast_time);
    tree_free(&second, NULL);
    tree_free(&first, NULL);
}

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
    case 0x14a0:
        if (g_session1.map_rule_flags[0] == 0)
            cast_timed(caster, 0x785, time);
        cast_timed(caster, 0x786, time);
        return;
    case 0x14a2:
        if (g_session1.map_rule_flags[0] == 0)
            cast_timed(caster, 0x760, time);
        cast_timed(caster, 0x761, time);
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
    case 0x1499:
        cast_timed(caster, 0x71f, time);
        if (g_session1.map_rule_flags[0] == 0)
            cast_timed(caster, 0x71e, time);
        return;
    case 0x8a6:
        spawn_on(caster->aid, 0x663);
        cast_timed(caster, 0x664, time);
        return;
    case 0x14ad:
        spawn_on(caster->aid, 0x746);
        spawn_on(caster->aid, 0x747);
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
    case 0x145f:
    case 0x1472:
        cast_timed(caster, 0x6c6, time);
        effect = spawn_on(caster->aid, 0x6c7);
        if (effect) {
            effect_set_time(effect, time);
            effect_add_sound_cue(effect, 0, "effect\\EF_BeginSpell.wav", 0, 0.0f);
        }
        return;
    case 0x1470:
        spawn_on(caster->aid, 0x70a);
        effect = spawn_on(caster->aid, 0x70b);
        if (effect) {
            spawned_effect_delay(effect, 200);
            effect_set_time(effect, (float)(cast_time - 200));
        }
        return;
    case 0x149e:
        if (g_session1.map_rule_flags[0] == 0) {
            effect = spawn_effect(caster->aid, 0x73b, actor_pos(caster));
            if (effect)
                effect_set_time(effect, time);
        }
        cast_ground_scope(caster, skill_id, cast_time);
        return;
    case 0x14f3:
        cast_timed(caster, 0x81c, time);
        effect = spawn_on(caster->aid, 0x81d);
        if (effect)
            effect->duration = cast_time;
        return;
    case 0x14f1:
    case 0x1f79:
        effect = spawn_on(caster->aid, skill == 0x14f1 ? 0x84b : 0x91d);
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
    cast_look_by_table(caster, skill, cast_time);
}
