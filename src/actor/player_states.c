#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "game/own_states.h"
#include "contents/sound.h"
#include "game/client_state.h"
#include "session/state.h"
#include "core/object_type.h"
#include "actor/actor.h"
#include "skills/skill_info.h"
#include "actor/pc.h"
#include "actor/weather.h"
#include "effects/skills/skill_view.h"
#include "actor/attr_state.h"

struct SkillActor;

static struct RagEffect *ps_launch(struct Player *self, int type)
{
    Vec3 none = { 0.0f, 0.0f, 0.0f };

    return game_actor_launch_effect((struct GameActor *)self, type, none, 0.0f);
}

static void ps_launch_tagged(struct Player *self, int type, int arg)
{
    struct RagEffect *effect = ps_launch(self, type);

    if (effect)
        rag_effect_send_msg(effect, NULL, 0x2c, 0xffff, arg, 0);
}

static void ps_relaunch(struct Player *self, int type)
{
    if (able_to_make_effect_is_effect_exist(self, type) == 1)
        render_object_send(self, NULL, 0x6d, type, 0, 0, 0);
    ps_launch(self, type);
}

static struct RagEffect *ps_drain_launch(struct Player *self, void *sender, int type)
{
    while (able_to_make_effect_is_effect_exist(self, type))
        render_object_send(self, sender, 0x6d, type, 0, 0, 0);
    return ps_launch(self, type);
}

static void *ps_spawn(struct Player *self, int type)
{
    Vec3 none = { 0.0f, 0.0f, 0.0f };

    return effect_mgr_spawn(g_effectMgr, self->aid, type, none, 0, -1, 0, 0);
}

static void *ps_spawn_here(struct Player *self, int type, int level)
{
    return effect_mgr_spawn(g_effectMgr, self->aid, type, self->pos, 0, level, 0, 0);
}

static void ps_respawn(struct Player *self, int type)
{
    effect_mgr_remove_type(g_effectMgr, self->aid, type);
    ps_spawn(self, type);
}

static void ps_respawn_here(struct Player *self, int type)
{
    effect_mgr_remove_type(g_effectMgr, self->aid, type);
    if (effect_mgr_is_registered(g_effectMgr, type) == 1)
        ps_spawn_here(self, type, -1);
}

static void ps_replace(struct Player *self, int type)
{
    if (effect_mgr_has_effect(g_effectMgr, self->aid, type) == 1)
        effect_mgr_remove_type(g_effectMgr, self->aid, type);
    ps_spawn(self, type);
}

static void ps_spawn_once(struct Player *self, int type)
{
    if (!effect_mgr_has_effect(g_effectMgr, self->aid, type))
        ps_spawn(self, type);
}

static void ps_spawn_once_here(struct Player *self, int type, int value)
{
    if (!effect_mgr_has_effect(g_effectMgr, self->aid, type))
        ps_spawn_here(self, type, value);
}

static void ps_play_wave(const char *name, float y)
{
    sound_play_wave(name, 0.0f, y, 0.0f, 250, 40, 1.0f);
}

static void ps_blind(struct Player *self)
{
    struct SkillInfo skill, extra;
    int level;

    skill_table_find(&g_session1.skills, &skill, 0x1b9);
    level = skill.level;
    if (level > 0) {
        int effect, type, i;

        skill_table_find(&g_session1.skills, &extra, 0xa1d);
        if (extra.level > 0)
            level = 5;
        effect = level + 0x1da;
        if (!able_to_make_effect_is_effect_exist(self, effect)) {
            ps_launch(self, 0x1e7);
            ps_play_wave("effect\\_blind.wav", -100.0f);
        }
        for (type = 0x1db, i = 10; i; type++, i--) {
            if (able_to_make_effect_is_effect_exist(self, type) == 1)
                render_object_send(self, NULL, 0x6d, type, 0, 0, 0);
        }
        ps_launch(self, effect);
        skill_info_destruct(&extra);
    }
    render_object_send(self, NULL, 0x6d, 0xa4, 0, 0, 0);
    self->status_icons = ps_launch(self, 0xa4);
    skill_info_destruct(&skill);
}

static void ps_tetra(struct Player *self, void *had, int id, int value)
{
    static const int types[4] = { 0x4ee, 0x4f0, 0x4ef, 0x4f1 };
    void *effect = NULL;
    int type;

    if (had)
        return;
    for (type = 0x4ee; type <= 0x4f1; type++) {
        effect = object_as_type(effect_mgr_find_effect(g_effectMgr, self->aid, type), OBJECT_TETRA_BALL_EFFECT);
        if (effect)
            break;
    }
    if ((unsigned)value > 3)
        return;
    if (effect)
        tetra_ball_effect_add_ball(effect, types[value], id - 0x16b);
    else
        tetra_ball_effect_move_first_to(object_as_type(ps_spawn(self, types[value]), OBJECT_TETRA_BALL_EFFECT), id - 0x16b);
}

static void ps_other(struct Player *self, int id, int value)
{
    if (id >= 0x2a2 && id <= 0x37f) {
        switch (id) {
        case 0x2a2: case 0x2ce: case 0x2cf: case 0x2d8: case 0x30b: case 0x336: case 0x337:
        case 0x339: case 0x33a: case 0x351: case 0x359: case 0x35f: case 0x360: case 0x368:
            render_object_send(self, NULL, 0x6e, id, 1, 0, 0);
            return;
        case 0x37f:
            game_actor_set_hat_effect((struct GameActor *)self, value + 0x950, 1);
            return;
        }
    }
    skill_action_mgr_efst_reapply(skill_action_mgr_instance(), id, value, (SkillActor *)(self));
}

void player_refresh_status_effects(struct Player *self)
{
    size_t state_index;
    void *tetra = NULL;
    int type;

    if (!self)
        return;
    weather_clear_dim(&g_weather);
    self->level = (unsigned short)g_session1.charBaseLevel;
    game_actor_show_level_aura((struct GameActor *)self);
    for (type = 0x4ee; type <= 0x4f1; type++) {
        tetra = object_as_type(effect_mgr_find_effect(g_effectMgr, self->aid, type), OBJECT_TETRA_BALL_EFFECT);
        if (tetra)
            break;
    }

    for (state_index = 0; state_index < own_states_count(); ++state_index) {
        const struct OwnStateEntry *rec = own_states_at(state_index);
        int id = rec->index;
        int value = rec->value;
        struct RagEffect *effect;

        switch (id) {
        case 0x2: case 0xa1:        ps_launch_tagged(self, 0xa6, 0x82); break;
        case 0x19: case 0xbc:       ps_launch_tagged(self, 0xa6, 0x80); break;
        case 0x1f:                  ps_launch_tagged(self, 0xa6, 0xa9); break;
        case 0x44:                  ps_launch_tagged(self, 0xa6, 0xfa); break;
        case 0xcf:                  ps_launch_tagged(self, 0x269, 0x269); break;
        case 0x46: case 0x4a: case 0x53: ps_relaunch(self, 0x361); break;
        case 0x47: case 0x4d:       ps_relaunch(self, 0x369); break;
        case 0x48:                  ps_relaunch(self, 0x36d); break;
        case 0x49:                  ps_relaunch(self, 0x35d); break;
        case 0x4b:                  ps_relaunch(self, 0x26); break;
        case 0x4c: case 0x50:       ps_relaunch(self, 0x367); break;
        case 0x4e:                  ps_relaunch(self, 0x363); break;
        case 0x4f: case 0x52:       ps_relaunch(self, 0x360); break;
        case 0x51: case 0x54:       ps_relaunch(self, 0x36b); break;
        case 0x56:                  ps_launch(self, 0xcb); break;
        case 0x57: case 0xeb:
            ps_launch(self, 0xfe);
            ps_launch(self, 0x46c);
            break;
        case 0x61:                  ps_launch(self, 0x28f); break;
        case 0x67:                  ps_launch(self, 0x1da); break;
        case 0x69:                  ps_launch(self, 0x171); break;
        case 0x6b:                  ps_launch(self, 0x170); break;
        case 0x6e: case 0x1d9: case 0x461: ps_launch(self, 0x177); break;
        case 0x71:                  ps_launch(self, 0x17e); break;
        case 0x7b:                  ps_launch(self, 0x18a); break;
        case 0x95: case 0x420: case 0x421: case 0x422: case 0x423:
            weather_set_dim(&g_weather);
            ps_launch(self, 0x1a8);
            break;
        case 0x98:                  ps_blind(self); break;
        case 0x99:                  ps_launch(self, 0x1a3); break;
        case 0xa0:                  weather_set_dim(&g_weather); break;
        case 0xa5: case 0xa6: case 0xa7: case 0xe8:
            ps_launch(self, 0x1a2);
            ps_launch(self, 0x220);
            break;
        case 0x107:                 ps_launch(self, 0x24a); break;

        case 0x10a:                 ps_launch(self, 0x2a1); break;
        case 0x148:                 ps_launch(self, 0x314); break;
        case 0x14a:                 ps_launch(self, 0x2f3); break;
        case 0x14e: case 0x365:
            effect = ps_drain_launch(self, NULL, 0x318);
            if (effect)
                rag_effect_send_msg(effect, NULL, 0x2c, 0xffff, 0x318, 0);
            break;
        case 0x153:
            if (value < 10)
                ps_spawn(self, g_session1.map_rule_flags[0] == 1 ? value + 0x5ba : value + 0x513);
            break;
        case 0x15e:
            effect = ps_drain_launch(self, NULL, 0x341);
            {
                float scale[3];

                scale[0] = 2.0f;
                scale[1] = 0.0f;
                scale[2] = 0.0f;
                if (effect)
                    rag_effect_send_msg(effect, NULL, 0xe, (intptr_t)scale, 0, 0);
            }
            break;
        case 0x15f:                 ps_launch(self, 0x315); break;
        case 0x168: case 0x17b:     ps_launch(self, 0x2e8); break;
        case 0x16b: case 0x16c: case 0x16d: case 0x16e: case 0x16f:
            ps_tetra(self, tetra, id, value);
            break;
        case 0x18a:                 ps_launch(self, 0x329); break;
        case 0x1a6:
            ps_play_wave("effect\\manhole.wav", 0.0f);
            ps_launch(self, 0x337);
            break;
        case 0x1b5:                 ps_launch(self, 0x3ac); break;
        case 0x1bd:
            effect = ps_drain_launch(self, NULL, 0x39d);
            if (effect)
                rag_effect_send_msg(effect, NULL, 0x2c, 0xffff, 0x39d, 0);
            break;
        case 0x1c0:                 ps_relaunch(self, 0x360); break;
        case 0x1c4:                 ps_relaunch(self, 0x57); break;
        case 0x1c5:                 ps_relaunch(self, 0x31f); break;
        case 0x1c6:                 ps_relaunch(self, 0x361); break;
        case 0x1d0:                 ps_launch(self, 0x170); break;
        case 0x23b:                 render_object_send(self, NULL, 0x6e, 0x23b, 1, value, 0); break;

        case 0x265:
            actor_set_mounted(self, 1);
            actor_set_spr_job(self, game_actor_get_job((struct GameActor *)self));
            actor_set_body_palette(self, self->body_palette);
            break;
        case 0x288: case 0x289:     render_object_send(self, NULL, 0x6e, id, 1, 0, 0); break;
        case 0x290:                 ps_drain_launch(self, NULL, 0x3ec); break;
        case 0x291:                 ps_drain_launch(self, self, 0x3eb); break;
        case 0x293:
            effect_mgr_remove_type(g_effectMgr, self->aid, 0x3ed);
            if (effect_mgr_is_registered(g_effectMgr, 0x3ed)) {
                void *orb = ps_spawn(self, 0x3ed);

                if (orb)
                    energy_orb_effect_set_ring_count(orb, 10);
            }
            ps_drain_launch(self, NULL, 0x3c3);
            break;
        case 0x2a1:                 render_object_send(self, NULL, 0x6e, id, 1, g_session1.cartType, 0); break;
        case 0x2a3:                 ps_respawn_here(self, 0x3f5); break;
        case 0x2cc:                 ps_drain_launch(self, self, 0x421); break;
        case 0x2cd:                 ps_drain_launch(self, self, 0x418); break;
        case 0x2d2:
            render_object_send(self, self, 0x6d, 0x420, 0, 0, 0);
            if (effect_mgr_is_registered(g_effectMgr, 0x420))
                ps_spawn_here(self, 0x420, -1);
            break;
        case 0x2da:
            ps_drain_launch(self, NULL, 0x410);
            ps_launch(self, 0x410);
            ps_launch(self, 0x410);
            break;
        case 0x2df:                 ps_drain_launch(self, self, 0x194); break;
        case 0x2e0:                 ps_drain_launch(self, self, 0x41f); break;
        case 0x2e1:                 ps_drain_launch(self, self, 0x41e); break;
        case 0x2f8:                 ps_respawn_here(self, 0x470); break;

        case 0x380:                 ps_respawn_here(self, 0x456); break;
        case 0x395:                 ps_respawn_here(self, 0x45f); break;
        case 0x397:                 ps_respawn_here(self, 0x449); break;
        case 0x3a5:
            actor_set_state(self, 0x30, 1);
            self->motion_held = 1;
            break;
        case 0x3a9:                 ps_respawn_here(self, 0x448); break;
        case 0x3d6:                 self->soul_attack = 1; break;
        case 0x3f6:                 ps_launch(self, 0x47c); break;
        case 0x3f8: case 0x3f9: case 0x3fa: case 0x3fb: case 0x3fc:
        case 0x3fd: case 0x3fe: case 0x3ff: case 0x400: case 0x401:
            ps_spawn(self, id + 0x87);
            break;
        case 0x40e:                 ps_replace(self, 0x4bf); break;
        case 0x40f:                 ps_replace(self, 0x4c1); break;
        case 0x410:                 ps_replace(self, 0x4be); break;
        case 0x411:                 ps_replace(self, 0x4c5); break;
        case 0x413:                 ps_replace(self, 0x4c0); break;
        case 0x41a:
            ps_replace(self, 0x4b1);
            ps_play_wave("effect\\t_캐스팅.wav", 0.0f);
            break;
        case 0x41f:
            ps_replace(self, 0x4c6);
            ps_play_wave("effect\\t_바람방출.wav", 0.0f);
            break;
        case 0x466:                 ps_respawn(self, 0x66c); break;
        case 0x47e:                 ps_spawn_once(self, 0x6cd); break;

        case 0x480:                 ps_spawn_once_here(self, 0x708, value); break;
        case 0x48d:
            if (!able_to_make_effect_is_effect_exist(self, 0x64a))
                ps_launch(self, 0x64a);
            break;
        case 0x494:                 ps_respawn(self, 0x69e); break;
        case 0x495:
            ps_respawn(self, 0x6ae);
            ps_respawn(self, 0x8e8);
            break;
        case 0x498:
            effect_mgr_remove_type(g_effectMgr, self->aid, 0x6be);
            effect_mgr_remove_type(g_effectMgr, self->aid, 0x6bf);
            ps_spawn(self, 0x6be);
            ps_spawn(self, 0x6bf);
            break;
        case 0x49a:                 ps_respawn(self, 0x6d7); break;
        case 0x4a6: case 0x4ce: case 0x4cf: break;
        case 0x4a8:
            while (able_to_make_effect_is_effect_exist(self, 0x3ec) == 1)
                render_object_send(self, NULL, 0x6d, 0x3ec, 0, 0, 0);
            ps_launch(self, 0x3ec);
            break;
        case 0x4a9:                 ps_spawn_once(self, 0x725); break;
        case 0x4ac:                 ps_spawn_once_here(self, 0x779, value); break;
        case 0x4ad:
            if (g_session1.map_rule_flags[0] == 0)
                ps_spawn_once(self, 0x712);
            break;
        case 0x4b1:
            if (g_session1.map_rule_flags[0] == 0)
                ps_spawn_once(self, 0x73c);
            ps_spawn_once(self, 0x73d);
            break;
        case 0x4b5:
            ps_relaunch(self, 0x14e);
            g_playerBlind = 1;
            render_object_send(self, NULL, 0x6d, 0xa4, 0, 0, 0);
            self->status_icons = ps_launch(self, 0xa4);
            break;
        case 0x4b6: case 0x4b9:     render_object_send(self, NULL, 0x9c, 0, 0, 0, 0); break;
        case 0x4b7:                 render_object_send(self, NULL, 0x98, 0, 0, 0, 0); break;
        case 0x4b8:
            ps_spawn(self, 0x8de);
            ps_spawn(self, 0x8e0);
            break;
        case 0x4ba:                 ps_respawn(self, 0x8dc); break;
        case 0x4bb:                 ps_spawn(self, 0x8e1); break;
        case 0x4bc:
            render_object_send(self, NULL, 0x7f, 0xff, 0, 0, 0);
            ps_respawn(self, 0x8dd);
            break;
        case 0x4bd:                 render_object_send(self, NULL, 0x9a, 0, 0, 0, 0); break;
        case 0x4be:                 render_object_send(self, NULL, 0x7f, 0xaa, 0, 0xaa, 0); break;
        case 0x4bf:                 render_object_send(self, NULL, 0x7f, 0, 0, 0xff, 0); break;
        case 0x4c0:                 render_object_send(self, NULL, 0x7f, 0xff, 0, 0, 0); break;
        case 0x4c4:                 ps_spawn_once(self, 0x886); break;
        case 0x4c6:                 ps_spawn(self, 0x881); break;
        case 0x4d0:
            ps_spawn(self, 0x7ba);
            ps_spawn(self, 0x7bd);
            break;
        case 0x4d1:
            ps_spawn(self, 0x7be);
            ps_spawn(self, 0x7c1);
            break;
        case 0x4d2:                 ps_spawn(self, 0x7b7); break;
        case 0x4d4:                 ps_spawn_here(self, 0x72f, value); break;
        case 0x4d5:                 ps_spawn_once_here(self, 0x719, value); break;
        case 0x4dc:
            if (!effect_mgr_has_effect(g_effectMgr, self->aid, 0x83d)) {
                void *abyss = ps_spawn(self, 0x83d);

                if (abyss)
                    from_the_abyss_effect_set_ball_count(abyss, value);
            }
            break;
        case 0x4dd:                 ps_spawn_once_here(self, 0x84e, value); break;
        case 0x4e2:                 ps_spawn_once(self, 0x76c); break;
        case 0x4e4:                 ps_spawn_once_here(self, 0x7aa, value); break;
        case 0x4e8:                 ps_spawn_once(self, 0x7d2); break;
        case 0x4ea:                 ps_spawn_once(self, 0x80c); break;
        default:
            ps_other(self, id, value);
            break;
        }
    }
    g_mapPropertyQuiet = 1;
}
