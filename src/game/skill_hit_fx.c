#include "render/math3d.h"
#include "session/state.h"
#include "game/context.h"
#include <math.h>
#include <stdlib.h>

#include "game/skill_fx.h"

static const char WAVE_DRAGON[] = "effect\\폭염룡.wav";

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
    case 0x2e1:
        spawn_on_delayed(target->aid, 0x593, caster_hit_delay(caster));
        break;
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

static const struct {
    unsigned short skill_id;
    unsigned short effects[3];
} target_effects[] = {
    { 0x1f6f, { 0x919 } },
    { 0x1f70, { 0x916, 0x917 } },
    { 0x1f72, { 0x921 } },
    { 0x1f75, { 0x912 } },
    { 0x1f76, { 0x913, 0x914 } },
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
        dragon_strike(&dragon_2db, caster, target);
        return;
    }
    for (i = 0; i < sizeof target_effects / sizeof target_effects[0]; i++) {
        if (target_effects[i].skill_id != skill_id)
            continue;
        for (j = 0; j < 3 && target_effects[i].effects[j]; j++)
            spawn_on(target->aid, target_effects[i].effects[j]);
        return;
    }
}
