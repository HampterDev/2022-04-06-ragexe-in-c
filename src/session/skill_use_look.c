#include "session/skill_commands.h"
#include "effects/rag/rag_effect.h"
#include "render/math3d.h"
#include "session/state.h"
#include "game/context.h"
#include <stdlib.h>

#include "game/skill_use.h"
#include "session/session_skills.h"
#include <stdint.h>

static void on(struct GameActor *actor, int effect_id)
{
    actor_launch_effect(actor, effect_id);
}

static struct RagEffect *on_placed(struct GameActor *actor, int effect_id, const struct GameActor *at)
{
    struct RagEffect *effect = actor ? actor_launch_effect(actor, effect_id) : NULL;
    Vec3 pos;

    if (effect) {
        pos = actor_pos(at);
        effect_set_pos(effect, &pos);
    }
    return effect;
}

static void effect_set_level(struct RagEffect *effect, int level)
{
    rag_effect_send_msg(effect, NULL, 0x2c, (int64_t)level, 0, 0);
}

static void on_target_with_level(struct GameActor *target, int effect_id, int level)
{
    struct RagEffect *effect = actor_launch_effect(target, effect_id);

    if (effect)
        effect_set_level(effect, level);
}

static void mode_effect(int effect_id, struct GameActor *caster, struct GameActor *target)
{
    if (game_current_mode() == 0)
        return;
    game_mode_launch_effect_at(game_current_mode(), effect_id, caster, target, -1);
}

static void spawn_delayed(int aid, int effect_id, int delay_ms)
{
    spawn_on_delayed(aid, effect_id, delay_ms);
}

static struct Player *own_player(void)
{
    return game_current_player();
}

static int caster_delay(const struct GameActor *caster, float base)
{
    return (int)(caster->motion_delay * 100.0f + base);
}

static void look_arrow_ring(struct GameActor *caster, struct GameActor *target)
{
    static const int ring[8][2] = {
        { 1, 0 }, { 1, 1 }, { 1, -1 }, { -1, 0 }, { -1, 1 }, { -1, -1 }, { 0, 1 }, { 0, -1 },
    };
    struct GameMode *mode = game_current_mode();
    const float *from = &caster->pos.x;
    int cell_x, cell_y, i;

    game_mode_launch_arrow_to_point(mode, from, &target->pos.x, 18.0f);
    game_mode_world_to_cell(mode, target->pos.x, target->pos.z, &cell_x, &cell_y);
    for (i = 0; i < 8; i++) {
        Vec3 to = cell_on_ground(mode, cell_x + ring[i][0], cell_y + ring[i][1]);
        game_mode_launch_arrow_to_point(mode, from, &to.x, 18.0f);
    }
}

static void look_arrow_storm(struct GameActor *caster, struct GameActor *target)
{
    int effect_id, delay;

    if (caster == target)
        return;
    play_skill_wave("arrowstorm.wav");
    effect_id = g_session1.map_rule_flags[0] == 1 ? 0x5b1 : 0x50d;
    spawn_on(target->aid, effect_id);
    for (delay = 600; delay < 1200; delay += 200)
        spawn_on_delayed(target->aid, effect_id + 1, delay);
}

static void look_fatal_menace(struct GameActor *caster, struct GameActor *target)
{
    int delay;

    spawn_on(caster->aid, 0x574);
    spawn_on(caster->aid, 0x575);
    delay = caster_delay(caster, 150.0f);
    spawn_delayed(target->aid, 0x572, delay);
    spawn_delayed(target->aid, 0x573, delay);
}

void game_mode_zc_use_skill_(void *session, int skill, int level,
                                         struct GameActor *caster, struct GameActor *target)
{
    struct RagEffect *effect;

    (void)session;

    if (caster == 0 || target == 0)
        return;
    if (skill_effect_on_target(skill, target) == 1)
        return;

    switch (skill) {
    case 0xe:  on_target_with_level(target, 0x1a, level); break;
    case 0x13: on_target_with_level(target, 0x1f, level); break;
    case 0x14: on_target_with_level(target, 0x1d, level); break;
    case 0xf:  on_placed(caster, 0x1b, target); break;
    case 0x11: on_placed(caster, 0x18, target); break;
    case 0x15: on(target, 0x1e); break;
    case 0x1c: on(target, 0x140); break;
    case 0x2f: look_arrow_ring(caster, target); break;
    case 0x38:
        effect = actor_launch_effect(caster, 0x94);
        if (effect == 0)
            return;
        effect_set_level(effect, level);
        {
            Vec3 pos = actor_pos(caster);
            effect_set_pos(effect, &pos);
        }
        break;
    case 0x39: on_placed(target, 0x46, target); break;
    case 0x3a: on_placed(caster, 0x96, target); break;
    case 0x3b:
        on_placed(caster, 0x50, target);
        on_placed(caster, 0x97, caster);
        break;
    case 0x3e: on_placed(caster, 0x95, caster); break;
    case 0x4d: on_placed(caster, 0x52, target); break;
    case 0x54: on_placed(caster, 0x5d, target); break;
    case 0x56: on_placed(caster, 0x75, target); break;
    case 0x5a: on_placed(target, 0x4f, target); break;
    case 0x88:
        on_placed(caster, 0x79, target);
        on_placed(caster, 0x8f, caster);
        break;
    case 0x89: on_placed(caster, 0x7b, target); break;
    case 0x95: on(target, 0x136); break;
    case 0x98: mode_effect(0x134, caster, target); break;
    case 0x99: on(caster, 0xaa); break;
    case 0xa0: on_placed(caster, 0x50, target); break;
    case 0xd4: on_placed(caster, 0x113, target); break;
    case 0xdb: on_placed((struct GameActor *)own_player(), 0xe3, caster); break;
    case 0xe6: mode_effect(0x12a, caster, target); break;
    case 0xfa: on(target, 0xf6); break;
    case 0xfb: on_placed(caster, 0xf9, target); break;
    case 0xfe: on(caster, 0xe2); break;
    case 0x107: on_placed(caster, 0x149, target); break;
    case 0x10a:
        on_placed(caster, 0x108, target);
        on_placed(caster, 0x10b, target);
        break;
    case 0x10b:
        effect = on_placed(caster, 0x109, target);
        if (effect == 0)
            return;
        effect_set_level(effect, level);
        break;
    case 0x10f:
        send_quake();
        play_skill_wave("effect\\mon_아수라 패황권.wav");
        on(target, 0x10a);
        break;

    case 0xfd: case 0x152: on(target, 0xf5); break;
    case 0x151: on_placed(caster, 0x1ee, target); break;
    case 0x153: on(caster, 0x1c2); break;
    case 0x155: on_placed(caster, 0x5d, target); break;
    case 0x158: on(target, 0x10f); break;
    case 0x159: on(target, 0x110); break;
    case 0x15a: on(target, 0x10e); break;
    case 0x16d: on(target, 0x17c); break;
    case 0x16f: on(target, 0x16d); break;
    case 0x170: on(caster, 0x16e); break;
    case 0x17b: on_placed(caster, 0x169, target); break;
    case 0x17d: on(target, 0x183); break;
    case 0x17e: on_placed(caster, 0x184, target); break;
    case 0x18a: on_placed(caster, 0x189, target); break;
    case 0x18d: on(target, 0x153); break;
    case 0x18e: on(caster, 0x18f); break;
    case 0x18f: on(caster, 0x190); break;
    case 0x190: on(target, 0x191); break;
    case 0x19f:
        on(target, 0x19d);
        on(target, 0x1af);
        break;
    case 0x1a1:
        on(target, 0x1d2);
        on(target, 0x1ad);
        break;
    case 0x1a5:
        on(caster, 0x1c9);
        on_placed(caster, 0x1b7, target);
        on(target, 0x1ca);
        break;
    case 0x1d3:
        on_placed(caster, 0x223, target);
        on(target, 0x226);
        break;
    case 0x1d4:
        on_placed(caster, 0x22b, target);
        on(target, 0x1ad);
        break;
    case 0x1d5:
        on(target, 0x1a9);
        effect = actor_launch_effect(caster, 0x229);
        if (effect) {
            Vec3 pos = aid_as_pos(target);
            effect_set_pos(effect, &pos);
        }
        on(target, 0x1af);
        on(target, 0x22a);
        break;
    case 0x1e0: on(target, 0x208); break;
    case 0x1e5: on(target, 0x206); break;
    case 0x1ea:
        {
            struct GameMode *mode = game_current_mode();
            Vec3 pos = actor_pos(target);
            game_mode_launch_dummy_effect(mode, &pos, 0x219, 0x64, 0);
        }
        break;
    case 0x1f5: on(target, 0x224); break;
    case 0x1f6: on(target, 0x288); break;
    case 0x1f7: on(target, 0x289); break;
    case 0x1fb: on(target, 0x284); break;
    case 0x200: on(target, 0x287); break;
    case 0x202: on_placed(caster, 0x26f, target); break;
    case 0x203: on(target, 0x283); break;
    case 0x206: on(target, 0x274); break;
    case 0x207: on_placed(caster, 0x248, target); break;
    case 0x208: on_placed(caster, 0x285, target); break;
    case 0x20b: mode_effect(0x265, caster, target); break;
    case 0x20c: mode_effect(0x266, caster, target); break;
    case 0x20d: mode_effect(0x267, caster, target); break;
    case 0x20e: mode_effect(0x268, caster, target); break;
    case 0x210: on(target, 0x278); break;
    case 0x212: on(target, 0x276); break;
    case 0x216: on(target, 0x26a); break;
    case 0x219: on_placed(target, 0x26b, target); break;
    case 0x21c:
        effect = actor_launch_effect(caster, 0x26d);
        if (effect == 0)
            return;
        {
            Vec3 pos = aid_as_pos(target);
            effect_set_pos(effect, &pos);
        }
        effect_set_level(effect, level);
        break;
    case 0x21e: on_placed(caster, 0x270, target); break;
    case 0x220: on(target, 0x279); break;
    case 0x28d: case 0x2ee: on(caster, 0x29a); break;

    case 0x28e: play_skill_wave("effect\\EF_FireWall.wav"); break;
    case 0x293: on(caster, 0x29c); break;
    case 0x295: case 0x2c8: on(caster, 0x199); break;
    case 0x299: case 0x2f4: on(caster, 0x29d); break;
    case 0x29a: case 0x2f7: play_skill_wave("_stonecurse.wav"); break;
    case 0x29b: case 0x2fa:
        play_skill_wave("amon_ra_die01.wav");
        on(caster, 0x29e);
        break;
    case 0x29c: case 0x2f8: play_skill_wave("effect\\widesleep.wav"); break;
    case 0x29d: on(caster, 0x16); break;
    case 0x2a0: on(target, 0x2a4); break;
    case 0x2a1: on(target, 0x2a5); break;
    case 0x2a2: on_placed((struct GameActor *)own_player(), 0xe3, target); break;
    case 0x2a3: on(caster, 0xd6); break;
    case 0x2a4: on(caster, 0x2a3); break;
    case 0x2ca: on(target, 0x405); break;
    case 0x2cb:
        {
            struct RagEffect *first = actor_launch_effect(target, 0x406);
            struct RagEffect *second = actor_launch_effect(target, 0x406);
            Vec3 pos;

            if (first) {
                pos = actor_pos(caster);
                effect_set_pos(first, &pos);
            }
            if (second) {
                pos = actor_pos(caster);
                effect_set_pos(second, &pos);
            }
            play_skill_wave("effect\\LG_CANNONSPEAR.wav");
        }
        break;
    case 0x2cc: on(caster, 0x408); break;
    case 0x2d8: look_arrow_storm(caster, target); break;
    case 0x2db:
        play_skill_wave("dragon_breath.wav");
        break;
    case 0x2dc: look_fatal_menace(caster, target); break;
    case 0x2e6:
        play_skill_wave("effect\\S포이즌버스터.wav");
        on(target, 0x39b);
        break;
    case 0x3ec: mode_effect(0x258, caster, target); break;
    case 0x1a3:
        on(target, 0x19f);
        on_placed(target, 0x1ae, caster);
        break;

    case 0x1f49:
        if (level == 1)
            on(target, 0x235);
        else if (level == 2)
            on(target, 0x236);
        else
            on(target, 0x237);
        break;
    case 0x1f4c:
        on(target, 0x237);
        on(target, 0x1a9);
        break;
    case 0x2031: on(target, 0x407); break;
    default:
        break;
    }
}
