#include "session/skill_commands.h"
#include "effects/rag/rag_effect.h"
#include "render/math3d.h"
#include "session/state.h"
#include "game/context.h"
#include <stdlib.h>

#include "game/skill_use.h"
#include "session/session_skills.h"

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
    rag_effect_send_msg(effect, NULL, 0x2c, (__int64)level, 0, 0);
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

static struct Effect *spawn_at(int aid, int effect_id, Vec3 at, int target_aid, int level)
{
    return effect_mgr_spawn(g_effectMgr, aid, effect_id, at, target_aid, level, 0, 0);
}

static void spawn_delayed(int aid, int effect_id, int delay_ms)
{
    spawn_on_delayed(aid, effect_id, delay_ms);
}

static struct Player *own_player(void)
{
    return game_current_player();
}

static float caster_heading(struct GameActor *caster)
{
    return actor_screen_heading(game_current_mode(), caster);
}

static int heading_quadrant(float heading, int first, int second, int third, int rest)
{
    if (heading > 0.0f && heading < 90.0f)
        return first;
    if (heading >= 90.0f && heading < 180.0f)
        return second;
    if (heading >= 180.0f && heading < 270.0f)
        return third;
    return rest;
}

static int heading_octant(float heading, const int ids[8], int none)
{
    if (heading >= 0.0f && heading < 23.0f)
        return ids[0];
    if (heading >= 23.0f && heading < 67.0f)
        return ids[1];
    if (heading >= 67.0f && heading < 112.0f)
        return ids[2];
    if (heading >= 112.0f && heading < 157.0f)
        return ids[3];
    if (heading >= 157.0f && heading < 202.0f)
        return ids[4];
    if (heading >= 202.0f && heading < 247.0f)
        return ids[5];
    if (heading >= 247.0f && heading < 292.0f)
        return ids[6];
    if (heading >= 292.0f && heading < 337.0f)
        return ids[7];
    if (heading >= 337.0f && heading <= 360.0f)
        return ids[0];
    return none;
}

static Vec3 turned_offset(const float m[12], float radius, Vec3 at)
{
    float zero = 0.0f;
    Vec3 p;

    p.x = radius * m[0] + m[3] * zero + m[6] * zero + m[9] + at.x;
    p.y = radius * m[1] + m[4] * zero + m[7] * zero + m[10] + at.y;
    p.z = radius * m[2] + m[5] * zero + m[8] * zero + m[11] + at.z;
    return p;
}

static int caster_delay(const struct GameActor *caster, float base)
{
    return (int)(caster->motion_delay * 100.0f + base);
}

static void spawned_effect_spin(struct Effect *effect, float degrees)
{
    effect->ops->set_spin(effect, degrees);
}

static const char WAVE_QIGONG[] = "effect\\기공포.wav";
static const char WAVE_MAENGHO[] = "effect\\맹호경파산.wav";

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

static void look_cross_ripper_slasher(struct GameActor *caster, struct GameActor *target)
{
    static const int ids[8] = { 0x527, 0x526, 0x525, 0x524, 0x523, 0x52a, 0x529, 0x528 };
    int effect_id = heading_octant(caster_heading(caster), ids, 0);
    struct RagEffect *effect;
    Vec3 pos;

    if (g_session1.map_rule_flags[0] == 1)
        effect_id += 0xa3;
    spawn_on(caster->aid, effect_id);
    spawn_on(target->aid, g_session1.map_rule_flags[0] == 1 ? 0x5c5 : 0x522);
    effect = actor_launch_effect(caster, 0x301);
    if (effect) {
        pos = aid_as_pos(target);
        effect_set_pos(effect, &pos);
    }
    on(target, 0x226);
}

static void look_aimed_bolt(struct GameActor *caster, struct GameActor *target)
{
    spawn_on(target->aid, 0x672);
    spawn_on(caster->aid, heading_quadrant(caster_heading(caster), 0x66e, 0x66d, 0x670, 0x66f));
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

static void look_judex(struct GameActor *caster, struct GameActor *target)
{
    float m[12];
    int n, radius, delay;

    play_skill_wave("effect\\AB_judex.wav");
    spawn_on(caster->aid, 0x65b);
    spawn_on(caster->aid, 0x65c);
    spawn_on(target->aid, 0x65f);
    spawn_on(target->aid, 0x660);
    for (n = 12; n != 0; n--) {
        Vec3 at = actor_pos(target);
        Vec3 p;
        int angle;

        radius = rand() % 15;
        if (radius < 2)
            radius = 2;
        angle = rand() % 36;
        if (angle < 0)
            angle = 0;
        mat_set_rotate_y(m, (float)angle * 10.0f);
        p = turned_offset(m, (float)radius, at);
        delay = rand() % 10;
        if (delay < 0)
            delay = 0;
        delay *= 50;
        {
            struct Effect *effect = spawn_at(0, 0x661, p, 0, -1);
            if (effect)
                spawned_effect_delay(effect, delay);
            effect = spawn_at(0, 0x662, p, 0, -1);
            if (effect)
                spawned_effect_delay(effect, delay);
        }
    }
}

static void spawn_pair_on_target(struct GameActor *target, int first, int second, int level)
{
    spawn_at(target->aid, first, actor_pos(target), target->aid, level);
    spawn_at(target->aid, second, actor_pos(target), target->aid, level);
}

static void look_hell_inferno(struct GameActor *target)
{
    float m[12];
    int i;

    spawn_on(target->aid, 0x60e);
    spawn_on(target->aid, 0x60f);
    for (i = 0; i < 4; i++) {
        Vec3 p;

        mat_set_rotate_y(m, ((float)i * 90.0f + 45.0f) / 180.0f * 3.141592f);
        p = turned_offset(m, 15.0f, actor_pos(target));
        spawn_at(0, 0x611, p, 0, -1);
        spawn_at(0, 0x612, p, 0, -1);
    }
}

static void look_banishing_point(struct GameActor *caster, struct GameActor *target)
{
    int effect_id = heading_quadrant(caster_heading(caster), 0x55e, 0x55d, 0x560, 0x55f);
    int delay = caster_delay(caster, 100.0f);

    if (g_session1.map_rule_flags[0] == 1) {
        spawn_delayed(caster->aid, effect_id + 0x86, delay);
        spawn_delayed(target->aid, 0x5df, delay);
        spawn_delayed(target->aid, 0x5e0, delay);
    } else {
        spawn_delayed(caster->aid, effect_id, delay);
        spawn_delayed(target->aid, 0x559, delay);
        spawn_delayed(target->aid, 0x55a, delay);
    }
    if (g_session1.map_rule_flags[0] == 1) {
        spawn_on(caster->aid, 0x5e1);
        spawn_on(caster->aid, 0x5e2);
    } else {
        spawn_on(caster->aid, 0x55b);
        spawn_on(caster->aid, 0x55c);
    }
    play_skill_wave("effect\\LG_BANISHINGPOINT.wav");
}

static void look_cannon_spear(struct GameActor *caster, struct GameActor *target)
{
    static const int ids[8] = { 0x57c, 0x57b, 0x57a, 0x579, 0x578, 0x57f, 0x57e, 0x57d };
    int sector = heading_octant(caster_heading(caster), ids, 0);
    int first = g_session1.map_rule_flags[0] == 1 ? 0x5ef : 0x580;

    if (effect_mgr_find_effect(g_effectMgr, caster->aid, first) == 0) {
        int delay;

        spawn_on(caster->aid, first);
        delay = caster_delay(caster, 100.0f);
        spawn_delayed(caster->aid, g_session1.map_rule_flags[0] == 1 ? sector + 0x6f : sector, delay);
    }
    on_placed(caster, 0x581, target);
    play_skill_wave("effect\\LG_CANNONSPEAR.wav");
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

static void look_triangle_shot(struct GameActor *caster, struct GameActor *target)
{
    int delay = caster_delay(caster, 100.0f);
    struct Effect *effect;

    if (g_session1.map_rule_flags[0] == 1) {
        spawn_delayed(target->aid, 0x5b3, delay);
        spawn_delayed(target->aid, 0x5b4, delay);
    } else {
        spawn_delayed(target->aid, 0x510, delay);
        spawn_delayed(target->aid, 0x511, delay);
    }
    effect = spawn_at(0, 0x52c, actor_pos(caster), target->aid, -1);
    if (effect)
        spawned_effect_delay(effect, delay);
}

static void look_vulcan_arm(struct GameActor *caster, struct GameActor *target)
{
    struct Effect *effect;

    spawn_on(caster->aid, 0x6a7);
    spawn_on(caster->aid, 0x6a8);
    effect = spawn_on(target->aid, 0x6a9);
    if (effect) {
        int turn = rand() % 36;
        if (turn < 0)
            turn = 0;
        spawned_effect_spin(effect, (float)turn * 10.0f);
    }
    on(caster, 0x308);
}

static void look_axe_tornado(struct GameActor *caster)
{
    int first = g_session1.map_rule_flags[0] == 1 ? 0x5d6 : 0x553;

    if (effect_mgr_find_effect(g_effectMgr, caster->aid, first) != 0)
        return;
    spawn_on(caster->aid, first);
    spawn_on(caster->aid, g_session1.map_rule_flags[0] == 1 ? 0x5d7 : 0x554);
}

static void look_power_swing(struct GameActor *caster, struct GameActor *target)
{
    spawn_on(target->aid, heading_quadrant(caster_heading(caster), 0x6a5, 0x6a4, 0x6a3, 0x6a2));
    spawn_on(target->aid, 0x6a6);
}

static void spin_move_once(struct GameActor *caster, const char *wave)
{
    if (able_to_make_effect_is_effect_exist(caster, 0x37a))
        return;
    on(caster, 0x376);
    on(caster, 0x37a);
    play_skill_wave(wave);
}

static void look_rampage_blaster(struct GameActor *caster)
{
    int effect_id;

    if (g_session1.map_rule_flags[0] == 1) {
        spawn_on(caster->aid, 0x5b5);
        spawn_on(caster->aid, 0x5b6);
        effect_id = 0x609;
    } else {
        spawn_on(caster->aid, 0x51e);
        spawn_on(caster->aid, 0x51f);
        effect_id = 0x376;
    }
    on(caster, effect_id);
    play_skill_wave("effect\\SR_RAMPAGEBLASTER.wav");
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
    case 0x1a5: case 0x7e7:
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
    case 0x2d8: case 0x8b9: look_arrow_storm(caster, target); break;
    case 0x2db: case 0x7d8: case 0x138c:
        play_skill_wave("dragon_breath.wav");
        break;
    case 0x2dc: case 0x8ec: look_fatal_menace(caster, target); break;
    case 0x2e6: case 0x990:
        play_skill_wave("effect\\S포이즌버스터.wav");
        on(target, 0x39b);
        break;
    case 0x3ec: mode_effect(0x258, caster, target); break;
    case 0x1a3: case 0xa14:
        on(target, 0x19f);
        on_placed(target, 0x1ae, caster);
        break;

    case 0x7d2:
        play_skill_wave(WAVE_QIGONG);
        on_placed(caster, 0x340, target);
        on_placed(caster, 0x68e, target);
        break;
    case 0x7d4:
        on_placed(caster, 0x2d3, target);
        on_placed(caster, 0x343, target);
        break;
    case 0x7dc:
        send_quake();
        on(target, 0x24d);
        play_skill_wave("_enemy_hit_wind2.wav");
        break;
    case 0x7e6:
        spawn_on(target->aid, 0x61e);
        on_placed(caster, 0x620, target);
        break;
    case 0x7ee: on_placed(target, 0x302, target); break;
    case 0x7ef:
        on(caster, 0x300);
        on(target, 0x24d);
        play_skill_wave("effect\\EF_PoisonAttack.wav");
        break;
    case 0x7f2:
        on(caster, 0x300);
        play_skill_wave("effect\\mon_맹룡과강.wav");
        on(caster, 0x305);
        break;
    case 0x7f5: look_cross_ripper_slasher(caster, target); break;
    case 0x7f6: look_judex(caster, target); break;
    case 0x7f8:
        play_skill_wave("effect\\AB_adoramus.wav");
        if (g_session1.map_rule_flags[0] == 1)
            spawn_pair_on_target(target, 0x594, 0x595, level);
        else
            spawn_pair_on_target(target, 0x4ea, 0x4eb, level);
        break;
    case 0x807: case 0x808:
        effect = on_placed(caster, 0x109, target);
        if (effect == 0)
            return;
        effect_set_level(effect, 3);
        break;
    case 0x8a3:
        play_skill_wave_at("crimson_r.wav", target->pos.x - caster->pos.x, 0.0f, target->pos.z - caster->pos.z);
        if (g_session1.map_rule_flags[0] == 1)
            spawn_pair_on_target(target, 0x5a1, 0x5a2, level);
        else
            spawn_pair_on_target(target, 0x4fa, 0x4fb, level);
        break;
    case 0x8a4: look_hell_inferno(target); break;
    case 0x8a6: case 0x8a7: spawn_on(target->aid, 0x666); break;
    case 0x8aa: on_placed(caster, 0x324, target); break;
    case 0x8b1: on(target, 0x31); break;
    case 0x8b2: on(target, 0x34); break;
    case 0x8b3: on(target, 0x33); break;
    case 0x8b4: on(target, 0x93); break;
    case 0x8bc: look_aimed_bolt(caster, target); break;
    case 0x8bd: on(caster, 0x2ee); break;
    case 0x8c3: play_skill_wave("wug_strike.wav"); break;
    case 0x8c4: play_skill_wave("wug_bite.wav"); break;
    case 0x8c6: play_skill_wave("were_wolf_attack.wav"); break;
    case 0x8d0:
        play_skill_wave(WAVE_MAENGHO);
        on_placed(caster, 0x326, target);
        break;

    case 0x8d1:
        send_quake();
        play_skill_wave(WAVE_MAENGHO);
        break;
    case 0x8d2: look_vulcan_arm(caster, target); break;
    case 0x8de:
        on(caster, 0x30c);
        on(caster, 0x31a);
        break;
    case 0x8df: on(target, 0x312); break;
    case 0x8e0: on(caster, 0x30d); break;
    case 0x8e6:
        spawn_at(caster->aid, 0x679, actor_pos(caster), target->aid, -1);
        spawn_on(target->aid, 0x67c);
        break;
    case 0x8e7: look_power_swing(caster, target); break;
    case 0x8e8: look_axe_tornado(caster); break;
    case 0x8f0: look_triangle_shot(caster, target); break;
    case 0x903: look_cannon_spear(caster, target); break;
    case 0x904: look_banishing_point(caster, target); break;
    case 0x905:
        send_quake();
        on(caster, 0x389);
        play_skill_wave("effect\\LG_TRAMPLE.wav");
        break;
    case 0x906: on(caster, 0x38a); break;
    case 0x908: on_placed(caster, 0x38b, target); break;
    case 0x90a:
        on(caster, 0x3a);
        on(caster, 0x179);
        play_skill_wave("effect\\LG_RAGEBURST.wav");
        break;
    case 0x910:
        spin_move_once(caster, "effect\\LG_MOONSLASHER.wav");
        on(caster, 0x3a);
        on(caster, 0x375);
        break;
    case 0x912: on(target, 0x5b); break;
    case 0x913: play_skill_wave("effect\\LG_EARTHDRIVE.wav"); break;
    case 0x914:
        on(caster, 0x376);
        on(target, 0x376);
        on(target, 0x383);
        on(target, 0x37e);
        on(target, 0x38a);
        play_skill_wave("effect\\LG_HESPERUSLIT.wav");
        break;
    case 0x916:
        on(caster, 0x375);
        on(caster, 0x376);
        play_skill_wave("effect\\SR_DRAGONCOMBO.wav");
        break;
    case 0x917:
        spin_move_once(caster, "effect\\SR_SKYNETBLOW.wav");
        on(target, 0x224);
        on(target, 0x226);
        break;
    case 0x91a:
        spawn_on(caster->aid, g_session1.map_rule_flags[0] == 1 ? 0x5fc : 0x52b);
        on(target, 0x379);
        play_skill_wave("effect\\SR_TIGERCANNON.wav");
        break;
    case 0x91c: look_rampage_blaster(caster); break;
    case 0x920:
        on(caster, 0x1c9);
        on(caster, 0x376);
        on_placed(caster, 0x37d, target);
        on(target, 0x1ca);
        play_skill_wave("effect\\SR_KNUCKLEARROW.wav");
        break;
    case 0x924:
        on(caster, 0x376);
        play_skill_wave("effect\\흡기.wav");
        break;
    case 0x927:
        spawn_on(target->aid, 0x626);
        on_placed(caster, 0x627, target);
        play_skill_wave("effect\\SR_GATEOFHELL.wav");
        break;
    case 0x928:
        on(caster, 0x375);
        on(caster, 0x376);
        play_skill_wave("effect\\mon_탄지신통.wav");
        break;
    case 0x96d:
        if (on_placed(caster, 0x630, target) == 0)
            return;
        play_skill_wave("effect\\metalic.wav");
        break;
    case 0x96e:
        on(target, 0x621);
        on(target, 0x63a);
        play_skill_wave("effect\\REVERBERATION.wav");
        break;
    case 0x97a:
        if (on_placed(caster, 0x35f, target))
            send_quake();
        on(target, 0x1d2);
        play_skill_wave("effect\\LG_EARTHDRIVE.wav");
        break;
    case 0x97d:
        on_placed(target, 0x365, target);
        on(target, 0x1a9);
        play_skill_wave("effect\\metalic.wav");
        break;

    case 0xa21:
        on(target, 0x4c8);
        on(target, 0x1ad);
        break;
    case 0xa2c:
        on(target, 0x1a9);
        effect = actor_launch_effect(caster, 0x4b6);
        if (effect) {
            Vec3 pos = aid_as_pos(target);
            effect_set_pos(effect, &pos);
        }
        on(target, 0x1af);
        on(target, 0x1a1);
        break;
    case 0x13a2:
        spawn_at(0, 0x44d, actor_pos(caster), 0, -1);
        spawn_at(caster->aid, 0x44c, actor_pos(caster), target->aid, -1);
        break;
    case 0x13b6:
        spawn_at(caster->aid, 0x468, actor_pos(caster), target->aid, -1);
        spawn_at(target->aid, 0x469, actor_pos(target), target->aid, -1);
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
    case 0x1f58: on(target, 0x3c0); break;
    case 0x1f59: on_placed(caster, 0x43a, target); break;
    case 0x1f5c: on(target, 0x3c5); break;
    case 0x1f5e: on(target, 0x3c7); break;
    case 0x1f64: case 0x1f65: case 0x1f66: on(target, 0x3c6); break;
    case 0x2031: on(target, 0x407); break;
    case 0x20d1:
        on(target, 0xfc);
        on(target, 0x150);
        break;
    case 0x20d4:
        on(caster, 0x1b9);
        on(target, 0x1b9);
        break;
    case 0x20d7: on(target, 0x185); break;
    case 0x20da:
        on(caster, 0xd6);
        on(target, 0xd6);
        break;
    case 0x20dd:
        play_skill_wave("effect\\EF_FireWall.wav");
        on(caster, 0x3a8);
        on(target, 0x3bc);
        break;
    case 0x20de:
        play_skill_wave("effect\\ef_firehit.wav");
        on(caster, 0x3a8);
        on(target, 0x3be);
        break;
    case 0x20df:
        play_skill_wave("effect\\EF_FireArrow1.wav");
        on(caster, 0x3a8);
        send_quake();
        on(target, 0x3bb);
        break;
    case 0x20e0:
        play_skill_wave("effect\\EF_FrostDiver2.wav");
        on(caster, 0x376);
        on(target, 0x3b5);
        break;
    case 0x20e1:
        play_skill_wave("effect\\EF_FrostDiver.wav");
        on(caster, 0x376);
        on(target, 0x3ba);
        break;
    case 0x20e2:
        play_skill_wave("effect\\EF_IceArrow1.wav");
        on(caster, 0x376);
        on(caster, 0x37e);
        on(target, 0x3b8);
        break;
    case 0x20e3:
        play_skill_wave("effect\\EF_ThunderStorm.wav");
        on(caster, 0x3a9);
        on(target, 0x3b3);
        break;
    case 0x20e4:
        play_skill_wave("effect\\EF_LightBolt1.wav");
        on(caster, 0x3a9);
        on(target, 0x3b4);
        break;
    case 0x20e5:
        play_skill_wave("effect\\EF_LightBolt3.wav");
        on(caster, 0x3a9);
        send_quake();
        on(target, 0x3b7);
        break;
    case 0x20e6:
        play_skill_wave("_stonecurse.wav");
        on(caster, 0x3aa);
        on(target, 0x3bd);
        break;
    case 0x20e7:
        play_skill_wave("evil_cloud_hermit_attack.wav");
        on(caster, 0x3aa);
        on(target, 0x3b9);
        break;
    case 0x20e8:
        play_skill_wave("effect\\wizard_earthspike.wav");
        on(caster, 0x3aa);
        send_quake();
        on(target, 0x3b6);
        break;
    case 0x20e9:
        effect = actor_launch_effect(target, 0x1f);
        if (effect)
            effect_set_level(effect, 1);
        break;
    case 0x20ea:
        play_skill_wave("effect\\화염진.wav");
        effect = actor_launch_effect(caster, 0x18);
        if (effect == 0)
            return;
        on(target, 0x11);
        {
            Vec3 pos = actor_pos(target);
            effect_set_pos(effect, &pos);
        }
        break;
    case 0x20ec:
        on(target, 0x385);
        on(target, 0x11);
        play_skill_wave("effect\\폭염룡.wav");
        break;
    case 0x20ee:
        effect = actor_launch_effect(target, 0x1a);
        if (effect)
            effect_set_level(effect, 1);
        break;
    case 0x20ef: on(target, 0x3ad); break;
    case 0x20f1:
        on(target, 0x3ae);
        on_placed(caster, 0x1b7, target);
        break;
    case 0x20f2:
        play_skill_wave(WAVE_QIGONG);
        on_placed(caster, 0x3af, target);
        break;
    case 0x20f3:
        on(target, 0x1d2);
        if (!able_to_make_effect_is_effect_exist(target, 0x1cc))
            on(target, 0x1cc);
        break;
    case 0x20f5:
        play_skill_wave(WAVE_MAENGHO);
        on_placed(caster, 0x3a7, target);
        break;
    case 0x20fa: on(target, 0x3b0); break;
    default:
        break;
    }
}
