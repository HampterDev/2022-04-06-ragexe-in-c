#include "render/math3d.h"
#include "session/state.h"
#include "game/context.h"
#include <math.h>
#include <stdlib.h>

#include "game/skill_fx.h"

static const char WAVE_DRAGON[] = "effect\\폭염룡.wav";

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static int caster_hit_delay(struct GameActor *caster)
{
    return (int)(caster->motion_delay * 100.0f + 100.0f);
}

static void spawned_effect_direction(struct Effect *effect, float degrees)
{
    effect->ops->set_spin(effect, degrees);
}

static void face_target_effect(struct GameActor *caster, struct GameActor *target)
{
    struct GameMode *mode = game_current_mode();
    float caster_x = caster->pos.x, caster_z = caster->pos.z;
    float target_x = target->pos.x, target_z = target->pos.z;
    float view = game_mode_get_view_heading(mode);
    float angle = (float)atan2((double)(target_z - caster_z), (double)(target_x - caster_x));
    float direction;
    struct Effect *effect;

    angle = angle / 3.141592f * 180.0f;
    direction = (float)(360.0 - ((double)angle + view));
    effect = spawn_on(caster->aid, 0x91f);
    if (effect)
        spawned_effect_direction(effect, direction);
}

void skill_hit_caster_effects(struct GameMode *mode, int skill_id, struct GameActor *caster,
                              struct GameActor *target)
{
    int delay;

    switch (skill_id) {
    case 0x296:
    case 0x300:
        if (caster != target)
            actor_launch_effect(target, 0x174);
        break;
    case 0x2c4:
        actor_launch_effect(target, 0x5c);
        actor_set_state(caster, 7, 1);
        break;
    case 0x2ed:
        if (caster == (struct GameActor *)mode->world->player)
            actor_launch_effect(caster, 0x461);
        if (target == (struct GameActor *)mode->world->player)
            actor_launch_effect(target, 0x461);
        break;
    case 0x7d3:
        actor_set_state(caster, 2, 1);
        actor_launch_effect(target, 0xb7);
        break;
    case 0x8a5:
        if (g_session1.map_rule_flags[0] == 1) {
            spawn_on(target->aid, 0x5fe);
            spawn_on(target->aid, 0x5ff);
        } else {
            spawn_on(target->aid, 0x585);
            spawn_on(target->aid, 0x586);
        }
        actor_set_state(caster, 7, 1);
        break;
    case 0x2e1:
    case 0x911:
        spawn_on_delayed(target->aid, 0x593, caster_hit_delay(caster));
        break;
    case 0x96d:
        spawn_on_delayed(target->aid, 0x62f, caster_hit_delay(caster));
        break;
    case 0x996:
        spawn_on(target->aid, 0x619);
        spawn_on(target->aid, 0x61a);
        break;
    case 0x9ad:
        if (g_session1.map_rule_flags[0] == 1) {
            spawn_on_delayed(caster->aid, 0x5da, 200);
            spawn_on_delayed(target->aid, 0x5d8, 400);
            spawn_on_delayed(target->aid, 0x5d9, 400);
        } else {
            spawn_on_delayed(caster->aid, 0x550, 200);
            spawn_on_delayed(caster->aid, 0x551, 200);
            spawn_on_delayed(target->aid, 0x54e, 400);
            spawn_on_delayed(target->aid, 0x54f, 400);
        }
        break;
    case 0x1452:
        spawn_on(target->aid, 0x6a1);
        break;
    case 0x1454: {
        struct Effect *running = effect_mgr_find_effect(g_effectMgr, target->aid, 0x6ae);

        delay = caster_hit_delay(caster);
        if (running) {
            spawn_on(target->aid, 0x6b1);
            spawn_on_delayed(target->aid, 0x69f, delay);
            spawn_on_delayed(target->aid, 0x6a0, delay);
        } else {
            spawn_on_delayed(target->aid, 0x6b4, delay);
            spawn_on_delayed(target->aid, 0x6b5, delay);
        }
        break;
    }
    case 0x1455:
        spawn_on(target->aid, rand_below(2) ? 0x6b9 : 0x6b8);
        delay = caster_hit_delay(caster);
        spawn_on_delayed(caster->aid, 0x6b6, delay);
        spawn_on_delayed(target->aid, 0x69f, delay);
        spawn_on_delayed(target->aid, 0x6a0, delay);
        break;
    case 0x145b:
        delay = caster_hit_delay(caster);
        spawn_on_delayed(target->aid, 0x6c0, delay);
        spawn_on_delayed(target->aid, 0x6c1, delay);
        break;
    case 0x14c3:
    case 0x14eb: {
        Vec3 at;

        at.x = caster->pos.x;
        at.y = caster->pos.y;
        at.z = caster->pos.z;
        effect_mgr_spawn(g_effectMgr, caster->aid, skill_id == 0x14c3 ? 0x86a : 0x81a, at, target->aid, -1, 0, 0);
        break;
    }
    case 0x1f78:
        face_target_effect(caster, target);
        break;
    }
}

struct DragonLook {
    unsigned short by_heading[4];
    unsigned short option_offset;
    unsigned short option_hit;
    unsigned short hit[2];
};

static const struct DragonLook dragon_2db = { { 0x565, 0x564, 0x567, 0x566 }, 0x8d, 0x5f0, { 0x562, 0x563 } };
static const struct DragonLook dragon_138c = { { 0x56d, 0x56c, 0x56f, 0x56e }, 0x8b, 0x5f6, { 0x56a, 0x56b } };

static void dragon_strike(const struct DragonLook *look, struct GameActor *caster, struct GameActor *target)
{
    struct GameMode *mode = game_current_mode();
    float heading;
    int effect_id;

    if (!mode || !caster)
        return;
    heading = actor_screen_heading(mode, caster);
    if (heading > 0.0f && heading < 90.0f)
        effect_id = look->by_heading[0];
    else if (heading >= 90.0f && heading < 180.0f)
        effect_id = look->by_heading[1];
    else if (heading >= 180.0f && heading < 270.0f)
        effect_id = look->by_heading[2];
    else
        effect_id = look->by_heading[3];

    if (g_session1.map_rule_flags[0] == 1) {
        effect_id += look->option_offset;
        if (!effect_mgr_find_effect(g_effectMgr, caster->aid, effect_id))
            spawn_on_delayed(caster->aid, effect_id, 200);
        spawn_on_delayed(target->aid, look->option_hit, 350);
    } else {
        if (!effect_mgr_find_effect(g_effectMgr, caster->aid, effect_id))
            spawn_on_delayed(caster->aid, effect_id, 200);
        spawn_on_delayed(target->aid, look->hit[0], 350);
        spawn_on_delayed(target->aid, look->hit[1], 350);
    }
    play_skill_wave(WAVE_DRAGON);
}

static void fan_around(struct GameActor *target)
{
    float step = (float)(rand_below(5) + 8) * 10.0f;
    float turn[12];
    Vec3 at;
    int i, pick, effect_id;

    for (i = 0; i < 3; i++) {
        float distance = (float)rand_below(10) + 5.0f;

        mat_set_rotate_y(turn, (float)i * step / 180.0f * 3.141592f);
        at.x = distance * turn[0] + turn[3] * 0.0f + turn[6] * 0.0f + turn[9] + target->pos.x;
        at.y = distance * turn[1] + turn[4] * 0.0f + turn[7] * 0.0f + turn[10] + target->pos.y;
        at.z = distance * turn[2] + turn[5] * 0.0f + turn[8] * 0.0f + turn[11] + target->pos.z;
        pick = rand_below(3);
        effect_id = pick == 0 ? 0x5a3 : (pick != 1) + 0x5a4;
        if (g_session1.map_rule_flags[0] == 1)
            effect_id += 0x61;
        spawn_effect(0, effect_id, at);
    }
}

static void fire_scatter(struct GameActor *target)
{
    int delay;

    for (delay = 90; delay < 450; delay += 90) {
        float target_x = target->pos.x, target_y = target->pos.y, target_z = target->pos.z;
        int dx = rand_below(3) * 5;
        int dz = rand_below(3) * 5;
        int quarter = rand_below(4);
        Vec3 at;
        struct Effect *effect;
        int second;

        if (dx == 10 && dz == dx) {
            if (rand_below(2) == 0)
                dx = 5;
            else
                dz = 5;
        }
        if (quarter == 0) {
            dx = -dx;
        } else if (quarter == 1) {
            dz = -dz;
        } else if (quarter == 2) {
            dx = -dx;
            dz = -dz;
        }
        at.x = (float)dx + target_x;
        at.y = target_y;
        at.z = (float)dz + target_z;
        if (g_session1.map_rule_flags[0] == 1) {
            effect = spawn_effect(0, 0x5d2, at);
            if (effect)
                spawned_effect_delay(effect, delay);
            second = 0x5d3;
        } else {
            effect = spawn_effect(0, 0x54b, at);
            if (effect)
                spawned_effect_delay(effect, delay);
            second = 0x54c;
        }
        effect = spawn_effect(0, second, at);
        if (effect)
            spawned_effect_delay(effect, delay);
    }
    if (g_session1.map_rule_flags[0] == 1) {
        spawn_on(target->aid, 0x5d2);
        spawn_on(target->aid, 0x5d3);
    } else {
        spawn_on(target->aid, 0x54b);
        spawn_on(target->aid, 0x54c);
    }
    play_skill_wave(WAVE_DRAGON);
}

static const struct {
    unsigned short skill_id;
    unsigned short effects[3];
} target_effects[] = {
    { 0x9d5, { 0x62c } },
    { 0x1499, { 0x720, 0x721 } },
    { 0x14ae, { 0x748, 0x749 } },
    { 0x1f6f, { 0x919 } },
    { 0x1f70, { 0x916, 0x917 } },
    { 0x1f72, { 0x921 } },
    { 0x1f75, { 0x912 } },
    { 0x1f76, { 0x913, 0x914 } },
    { 0x20fd, { 0x7f3, 0x7f4 } },
    { 0x2100, { 0x7e3, 0x7e4 } },
    { 0x2103, { 0x832, 0x833 } },
    { 0x2106, { 0x838 } },
    { 0x2109, { 0x7d9 } },
    { 0x2199, { 0x78e, 0x791 } },
    { 0x219a, { 0x77b, 0x77d } },
    { 0x219d, { 0x772, 0x773, 0x775 } },
};

void skill_hit_target_effects(int skill_id, int level, struct GameActor *caster, struct GameActor *target)
{
    unsigned i, j;

    switch (skill_id) {
    case 0x8d:
        if (level == -1)
            actor_launch_effect(target, 0x81);
        return;
    case 0x2db:
    case 0x7d8:
        dragon_strike(&dragon_2db, caster, target);
        return;
    case 0x138c:
        dragon_strike(&dragon_138c, caster, target);
        return;
    case 0x7d5:
        if (target != caster)
            spawn_on(target->aid, 0x69d);
        return;
    case 0x89c:
        fan_around(target);
        return;
    case 0x8d5:
        fire_scatter(target);
        return;
    case 0x8be:
        play_skill_wave("effect\\hunter_shockwavetrap.wav");
        actor_launch_effect(target, 0x92);
        return;
    case 0x8bf:
        actor_launch_effect(target, 0x249);
        play_skill_wave("effect\\hunter_landmine.wav");
        return;
    case 0x8cd:
        actor_launch_effect(target, 0x247);
        play_skill_wave(WAVE_DRAGON);
        return;
    case 0x8d3:
        actor_launch_effect(target, 0x247);
        actor_launch_effect(target, 0x31);
        play_skill_wave(WAVE_DRAGON);
        return;
    case 0x8ce:
        actor_launch_effect(target, 0x87);
        play_skill_wave("effect\\ef_frostdiver.wav");
        return;
    case 0x900:
        if (g_session1.map_rule_flags[0] == 1) {
            spawn_on(target->aid, 0x5b7);
            spawn_on(target->aid, 0x5b8);
        } else {
            spawn_on(target->aid, 0x520);
            spawn_on(target->aid, 0x521);
        }
        return;
    case 0x919:
        spawn_on_delayed(target->aid, 0x624, 100);
        return;
    case 0x9b1:
        play_skill_wave(WAVE_DRAGON);
        actor_launch_effect(target, 0x385);
        return;
    case 0x9ba:
        spawn_on(target->aid, 0x551);
        play_skill_wave("spore_attack.wav");
        return;
    case 0x14d2: {
        Vec3 at;

        at.x = target->pos.x;
        at.y = target->pos.y;
        at.z = target->pos.z;
        effect_mgr_spawn(g_effectMgr, target->aid, 0x7b5, at, 0, level, 0, 0);
        return;
    }
    }
    for (i = 0; i < sizeof target_effects / sizeof target_effects[0]; i++) {
        if (target_effects[i].skill_id != skill_id)
            continue;
        for (j = 0; j < 3 && target_effects[i].effects[j]; j++)
            spawn_on(target->aid, target_effects[i].effects[j]);
        return;
    }
}
