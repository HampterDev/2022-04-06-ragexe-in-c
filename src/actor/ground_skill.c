#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "contents/sound.h"
#include "effects/durations.h"
#include "session/state.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "game/gamemode.h"
#include "session/session_skills.h"

static void gs_dummy(struct GameMode *mode, Vec3 *at, int kind)
{
    game_mode_launch_dummy_effect(mode, at, kind, g_effectDurations[kind] + 10, 0);
}

static struct Effect *gs_spawn(Vec3 pos, int type, int level)
{
    return effect_mgr_spawn(g_effectMgr, 0, type, pos, 0, level, 0, 0);
}

static void gs_wave(const char *name, float y)
{
    sound_play_wave(name, 0.0f, y, 0.0f, 250, 40, 1.0f);
}

static struct RagEffect *gs_launch(struct GameActor *self, int effect)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };
    return game_actor_launch_effect(self, effect, zero, 0.0f);
}

static void gs_min_or_dummy(struct GameMode *mode, Vec3 *at, int min_type, int kind)
{
    if (g_session1.map_rule_flags[0] != 0)
        gs_spawn(*at, min_type, -1);
    else
        gs_dummy(mode, at, kind);
}

void game_actor_am_groundskill(struct GameActor *self, const struct GroundSkillInfo *rec)
{
    struct GameMode *mode = game_current_mode();
    int level = rec->level;
    int skill = rec->skill_id;
    int motion, state;
    float x, z, h;
    Vec3 at;
    int forced;

    session_get_use_effect(&g_session1, skill, &motion, &state);
    game_mode_cell_to_world(mode, rec->x, rec->y, &x, &z);
    h = c3d_attr_get_height(mode->world->attr, x, z);
    self->attack_target_aid = 0;

    switch (skill) {
    case 0x53: case 0x301: case 0x306: case 0x13a4: case 0x13b2:
        forced = 1;
        break;
    default:
        forced = 0;
        break;
    }
    if (forced) {
        void *act = render_object_get_act_res((struct RenderObject *)self);
        self->action = 0x58;
        self->motion = actres_frame_count(act, 0x58) - 1;
    } else {
        render_object_turn_toward((struct RenderObject *)self, x, z);
        actor_set_state(self, state, 1);
    }

    at.x = x;
    at.y = h;
    at.z = z;
    if (session_play_ground_skill_effect(&g_session1, skill, at) == 1)
        return;

    switch (skill) {
    case 0x15:   gs_dummy(mode, &at, 0x1e); break;
    case 0x45:
    case 0x2b6:  gs_dummy(mode, &at, 0x5b); break;
    case 0x46:   gs_dummy(mode, &at, 0x53); break;
    case 0x4f:   gs_dummy(mode, &at, 0x71); break;
    case 0x53:   gs_min_or_dummy(mode, &at, 0x48b, 0x5c); break;
    case 0x55:   gs_min_or_dummy(mode, &at, 0x48a, 0x5a); break;
    case 0x57:   gs_dummy(mode, &at, 0x4a); break;
    case 0x59:
    case 0x2d3:
        game_mode_launch_dummy_effect(mode, &at, g_session1.map_rule_flags[0] != 0 ? 0x2c4 : 0x59, g_effectDurations[0x59] + 10, 0);
        break;
    case 0x5b:   gs_dummy(mode, &at, 0x8e); break;
    case 0x5c:   gs_min_or_dummy(mode, &at, 0x489, 0x5f); break;
    case 0x6e:
    case 0x2dd:
    case 0x138e: gs_dummy(mode, &at, 0x66); break;
    case 0x6f:
    case 0x1cb:  gs_dummy(mode, &at, 0x62); break;
    case 0x73:   gs_dummy(mode, &at, 0x45); break;
    case 0x82: {
        void *obj;
        gs_dummy(mode, &at, 0x77);
        obj = (void *)self->falcon;
        if (obj != NULL)
            render_object_send(obj, NULL, 0x1d, MSG_ARG_PTR(&at), 0, 0, 0);
        break;
    }
    case 0x8c:   gs_dummy(mode, &at, 0x7c); break;
    case 0x11d:  gs_dummy(mode, &at, 0xe1); break;
    case 0x11e:
    case 0x21d:  gs_dummy(mode, &at, 0xec); break;
    case 0x11f:  gs_dummy(mode, &at, 0xed); break;
    case 0x120:  gs_dummy(mode, &at, 0xee); break;
    case 0x1de:
        gs_wave("assulter_attack.wav", -150.0f);
        if (level < 6)
            gs_dummy(mode, &at, 0x1f1);
        else if (level < 10)
            gs_dummy(mode, &at, 0x1f2);
        else
            gs_dummy(mode, &at, 0x1f3);
        break;
    case 0x1e3:
        gs_wave("effect\\EF_FireWall.wav", 0.0f);
        gs_dummy(mode, &at, 0xdf);
        break;
    case 0x1e4:
        gs_wave("effect\\wizard_earthspike.wav", 0.0f);
        game_mode_launch_dummy_effect(mode, &at, 0x20a, 0x104, 0);
        break;
    case 0x1eb:  gs_launch(self, 0x3a); break;
    case 0x209:  gs_wave("effect\\그라운드.wav", 0.0f); break;
    case 0x21a:  gs_wave("effect\\수둔.wav", 0.0f); break;
    case 0x2c6:
        gs_launch(self, 0x87);
        gs_wave("effect\\ef_frostdiver.wav", 0.0f);
        break;
    case 0x2cd:  gs_launch(self, 0x408); break;
    case 0x2e1:
    case 0x911:
        gs_spawn(at, 0x590, -1);
        gs_wave("effect\\LG_RAYOFGENESIS.wav", 0.0f);
        break;
    case 0x301: {
        Vec3 above = { at.x, at.y + 25.0f, at.z };
        gs_spawn(above, 0x4e7, -1);
        gs_spawn(above, 0x4e6, -1);
        gs_wave("effect\\wizard_meteor.wav", 0.0f);
        break;
    }
    case 0x306: {
        struct Effect *e = gs_spawn(at, 0x426, -1);
        if (e != NULL)
            e->ops->skip(e, 0.4f);
        break;
    }
    case 0x8d4: {
        struct RagEffect *e = gs_launch(self, 0x317);
        if (e != NULL)
            rag_effect_send_msg(e, NULL, 0xe, MSG_ARG_PTR(&at), 0, 0);
        break;
    }
    case 0x98f:
        gs_wave("effect\\wl_jackfrost.wav", 0.0f);
        if (g_session1.map_rule_flags[0] == 1) {
            gs_spawn(at, 0x5aa, level);
            gs_spawn(at, 0x5ab, level);
        } else {
            gs_spawn(at, 0x502, level);
            gs_spawn(at, 0x503, level);
        }
        break;
    case 0xa0b:  gs_spawn(at, 0x426, -1); break;
    case 0x13a4: gs_spawn(at, 0x452, -1); break;
    case 0x13b2: gs_spawn(at, 0x458, -1); break;
    default:
        break;
    }
}
