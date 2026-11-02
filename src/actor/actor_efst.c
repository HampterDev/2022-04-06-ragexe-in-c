#include "actor/actor_ops.h"
#include "core/object_type.h"
#include "effects/rag/rag_effect.h"
#include "contents/sound.h"
#include "session/state.h"
#include "game/context.h"
#include "game/gamemode.h"
#include "actor/actor.h"
#include "actor/weather.h"
#include "actor/game_actor.h"
#include "render/render_api.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "ui/character/gauge.h"
#include "actor/attr_state.h"
#include <stdint.h>

static const char blind_wave[] = "effect\\_blind.wav";
static const char manhole_wave[] = "effect\\manhole.wav";
static const char word_flee_up[] = "FLEE UP!";
static const char word_atk_up[] = "ATK UP!";
static const char word_dex_up[] = "DEX UP!";
static const char word_hp_up[] = "HP UP!";
static const char word_sp_up[] = "SP UP!";
static const char charging_pierce_wave[] = "effect\\DK_CHARGINGPIERCE3.wav";

static struct RagEffect *ef_launch(struct GameActor *a, int id)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };
    return game_actor_launch_effect(a, id, zero, 0.0f);
}

static void ef_remove(struct GameActor *a, int id)
{
    while (able_to_make_effect_is_effect_exist(a, id))
        render_object_send(a, NULL, 0x6d, id, 0, 0, 0);
}

static void ef_remove_once(struct GameActor *a, int id)
{
    if (able_to_make_effect_is_effect_exist(a, id) == 1)
        render_object_send(a, NULL, 0x6d, id, 0, 0, 0);
}

static void ef_toggle(struct GameActor *a, int id, int on)
{
    ef_remove(a, id);
    if (on)
        ef_launch(a, id);
}

static void ef_toggle_once(struct GameActor *a, int id, int on)
{
    ef_remove_once(a, id);
    if (on)
        ef_launch(a, id);
}

static void ef_set_tag(struct RagEffect *e, int value)
{
    if (e)
        rag_effect_send_msg(e, NULL, 0x2c, 0xffff, value, 0);
}

static void ef_toggle_tagged(struct GameActor *a, int id, int on, int value)
{
    ef_remove(a, id);
    if (on)
        ef_set_tag(ef_launch(a, id), value);
}

static struct Effect *ef_spawn_at(int aid, int type, const Vec3 *pos, int level)
{
    return effect_mgr_spawn(g_effectMgr, aid, type, *pos, 0, level, 0, 0);
}

static struct Effect *ef_spawn(int aid, int type)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };
    return ef_spawn_at(aid, type, &zero, -1);
}

static void ef_remove_type(int aid, int type)
{
    effect_mgr_remove_type(g_effectMgr, aid, type);
}

static void ef_spawn_or_remove(int aid, int type, int on)
{
    if (on)
        ef_spawn(aid, type);
    else
        ef_remove_type(aid, type);
}

static void ef_end(int aid, int type)
{
    struct Effect *e = effect_mgr_find_effect(g_effectMgr, aid, type);

    if (e)
        effect_mgr_remove(g_effectMgr, e, 1);
}

static void ef_spawn_or_end(int aid, int type, int on)
{
    if (on)
        ef_spawn(aid, type);
    else
        ef_end(aid, type);
}

static void ef_spawn_once_or_end(int aid, int type, int on)
{
    if (on) {
        if (!effect_mgr_find_effect(g_effectMgr, aid, type))
            ef_spawn(aid, type);
    } else {
        ef_end(aid, type);
    }
}

static void ef_respawn(int aid, int type, int on)
{
    if (effect_mgr_has_effect(g_effectMgr, aid, type) == 1)
        ef_remove_type(aid, type);
    if (on)
        ef_spawn(aid, type);
}

static void ef_wave(const char *name, const struct GameActor *at, const struct GameActor *player)
{
    if (player)
        sound_play_wave(name, at->pos.x - player->pos.x, 0.0f, at->pos.z - player->pos.z, 250, 40, 1.0f);
}

static struct GameActor *ef_actor(struct GameActor *self, struct GameMode *mode)
{
    if (self->aid == g_session1.ownAid)
        return self;
    return world_get_game_actor_by_aid(mode->world, self->aid);
}

static void ef_player_darkness(struct GameActor *self)
{
    struct GameActor *player = (struct GameActor *)game_current_player();

    if (player)
        ((struct Player *)player)->status_icons = ef_launch(self, 0xa4);
}

static int ef_unmounted_job(int job)
{
    switch (job) {
    case 0x1013: return 0x18;
    case 0x1014: return 0x1;
    case 0x1016: return 0xfab;
    case 0x1017: return 0x5;
    case 0x1018: return 0xfee;
    case 0x1019: return 0xfb3;
    case 0x101a: return 0x3;
    case 0x101b: return 0xfcf;
    case 0x101c: return 0x0;
    case 0x101d: return 0xf;
    case 0x101e: return 0x4;
    case 0x101f: return 0xfe6;
    case 0x1020: return 0x17;
    case 0x1021: return 0xfd9;
    case 0x1022: return 0x2;
    case 0x1023: return 0x10;
    case 0x1024: return 0xfe3;
    case 0x1025: return 0xfd7;
    case 0x1026: return 0x9;
    case 0x1027: return 0xfb1;
    case 0x1028: return 0xfaa;
    case 0x1029: return 0x12;
    case 0x102a: return 0xa;
    case 0x102b: return 0xfb0;
    case 0x102c: return 0xfdb;
    case 0x102d: return 0x6;
    case 0x102e: return 0x11;
    case 0x102f: return 0xfe8;
    case 0x1030: return 0xfb2;
    case 0x1031: return 0xc;
    case 0x1032: return 0xfad;
    case 0x1033: return 0x14;
    case 0x1034: return 0xfe4;
    case 0x1035: return 0x13;
    case 0x1036: return 0xfac;
    case 0x1037: return 0xfe5;
    case 0x1038: return 0xfb5;
    case 0x1039: return 0xfb4;
    case 0x103a: return 0xb;
    case 0x103b: return 0xfce;
    case 0x103c: return 0x8;
    case 0x103d: return 0xfa9;
    case 0x103e: return 0xfb7;
    case 0x103f: return 0xfb8;
    case 0x1040: return 0xfb9;
    case 0x1041: return 0xfba;
    case 0x1042: return 0xfbb;
    case 0x1043: return 0xfbc;
    case 0x1044: return 0xfc2;
    case 0x1045: return 0xfc3;
    case 0x1046: return 0xfc6;
    case 0x1047: return 0xfc7;
    case 0x1048: return 0xfc8;
    case 0x1049: return 0xfc9;
    case 0x104a: return 0xfca;
    case 0x104b: return 0xfcb;
    case 0x104c: return 0xfcd;
    case 0x104d: return 0x1001;
    case 0x104e: return 0x1003;
    case 0x104f: return 0x1005;
    case 0x1050: return 0x1007;
    case 0x1051: return 0x1008;
    case 0x1052: return 0x1009;
    case 0x1053: return 0x100a;
    case 0x1054: return 0x100b;
    case 0x1055: return 0xfbd;
    case 0x1056: return 0x100c;
    case 0x1057: return 0xfa1;
    case 0x1058: return 0xfa2;
    case 0x1059: return 0xfa3;
    case 0x105a: return 0xfa4;
    case 0x105b: return 0xfa5;
    case 0x105c: return 0xfa6;
    case 0x105d: return 0xfa7;
    case 0x1060: return 0x105e;
    case 0x1061: return 0x105f;
    case 0x1062: return 0xfbf;
    case 0x1063: return 0xfc0;
    case 0x1064: return 0xfc1;
    case 0x1065: return 0xfda;
    case 0x1066: return 0xfd8;
    case 0x1067: return 0x7;
    case 0x1068: return 0xfa8;
    case 0x1069: return 0xfe2;
    case 0x106a: return 0xfd6;
    case 0x106b: return 0xe;
    case 0x106c: return 0xfaf;
    case 0x106d: return 0x1004;
    case 0x106e: return 0x1002;
    case 0x106f: return 0xfbe;
    case 0x1070: return 0x1006;
    case 0x1071: return 0x1000;
    case 0x1072: return 0xfc5;
    case 0x1075: return 0x1073;
    case 0x1076: return 0x1074;
    case 0x1078: return 0x1077;
    case 0x107b: return 0x107a;
    case 0x107d: return 0x107c;
    case 0x1086: return 0x107e;
    case 0x1087: return 0x107f;
    case 0x1088: return 0x1080;
    case 0x1089: return 0x1081;
    case 0x108a: return 0x1082;
    case 0x108b: return 0x1083;
    case 0x108c: return 0x1084;
    case 0x108d: return 0x1085;
    case 0x1095: return 0x108f;
    case 0x1096: return 0x1090;
    case 0x1097: return 0x1091;
    case 0x1098: return 0x1092;
    case 0x10ab: return 0x109e;
    case 0x10ac: return 0x109f;
    case 0x10ad: return 0x10a0;
    case 0x10ae: return 0x10a1;
    case 0x10b1: return 0x10a4;
    case 0x10b2: return 0x10a5;
    case 0x10b3: return 0x10a6;
    case 0x10b4: return 0x10a7;
    case 0x10b5: return 0x10a8;
    case 0x10d5: return 0x10ce;
    case 0x10d6: return 0x10cf;
    case 0x10d7: return 0x10d0;
    case 0x10d8: return 0x10d1;
    case 0x10d9: return 0x10d2;
    case 0x10da: return 0x10d3;
    case 0x10db: return 0x10d4;
    }
    return -1;
}

static int ef_quiet(unsigned state)
{
    return state <= 0x1 || (state >= 0x3 && state <= 0x18) || (state >= 0x1a && state <= 0x1c) || state == 0x1e
        || state == 0x20 || state == 0x21 || state == 0x40 || state == 0x62 || state == 0x93 || state == 0x96
        || state == 0x97 || state == 0x10a || state == 0x129 || state == 0x12b || state == 0x13a || state == 0x161
        || state == 0x165 || state == 0x191 || state == 0x19e || state == 0x207 || state == 0x241 || state == 0x244
        || state == 0x245 || state == 0x248 || state == 0x25d || state == 0x25f || state == 0x260 || state == 0x26a
        || state == 0x288 || state == 0x289 || state == 0x494;
}

static void ef_blind(struct GameActor *self, int level)
{
    if ((unsigned)(level - 2) <= 9) {
        int id = level + 0x1d9;
        int other, n;

        if (!able_to_make_effect_is_effect_exist(self, id)) {
            ef_launch(self, 0x1e7);
            sound_play_wave(blind_wave, 0.0f, -100.0f, 0.0f, 250, 40, 1.0f);
        }
        for (other = 0x1db, n = 10; n != 0; other++, n--) {
            if (able_to_make_effect_is_effect_exist(self, other) == 1)
                render_object_send(self, NULL, 0x6d, other, 0, 0, 0);
        }
        ef_launch(self, id);
    }
    render_object_send(self, NULL, 0x6d, 0xa4, 0, 0, 0);
    ef_player_darkness(self);
}

static void ef_mount(struct GameActor *self, int on)
{
    if (on) {
        actor_set_mounted(self, 1);
        if (self->state == 0x19)
            actor_set_state(self, 0, 1);
    } else {
        int job;

        actor_set_mounted(self, 0);
        job = ef_unmounted_job(game_actor_get_job(self));
        if (job != -1)
            game_actor_set_job(self, job);
    }
    actor_set_spr_job(self, game_actor_get_job(self));
    actor_set_body_palette(self, self->body_palette);
    (actor_set_robe(self, 0, 1));
}

static void ef_2f3(struct GameActor *self)
{
    int aid = self->aid;

    if (!tree_find(&g_session1.memberPings, &aid, tree_less_int)) {
        if (effect_mgr_is_registered(g_effectMgr, 0x425))
            ef_spawn_at(self->aid, 0x425, &self->pos, -1);
    }
    if (effect_mgr_is_registered(g_effectMgr, 0x433))
        ef_spawn_at(self->aid, 0x433, &self->pos, -1);
}

static void ef_47d(struct GameActor *self, struct GameMode *mode, long long par2, long long par3)
{
    struct GameActor *who = ef_actor(self, mode);

    if (!who)
        return;
    if (par2 == 1) {
        if (par3 == 2) {
            actor_set_sprite_variant(who, 2);
            if (self->aid == g_session1.ownAid)
                g_session1.madogear = 2;
        } else {
            actor_set_sprite_variant(who, 1);
            if (self->aid == g_session1.ownAid)
                g_session1.madogear = 1;
        }
        render_object_send(who, NULL, 0x69, 0, 0, 0, 0);
    } else {
        actor_set_sprite_variant(who, 0);
        if (self->aid == g_session1.ownAid)
            g_session1.madogear = 0;
        render_object_send(who, NULL, 0x6a, 0, 0, 0, 0);
    }
}

static void ef_charging_pierce(struct GameActor *self, struct GameMode *mode, long long par3, long long par4)
{
    struct Effect *e = effect_mgr_find_effect(g_effectMgr, self->aid, 0x6e4);

    if (par4 != 0) {
        struct World *world = mode->world;
        struct GameActor *target;

        if (e)
            effect_mgr_remove(g_effectMgr, e, 1);
        if (par4 == (long long)(unsigned)g_session1.ownAid)
            target = (struct GameActor *)world->player;
        else
            target = world_get_game_actor_by_aid(world, (int)par4);
        if (!target)
            return;
        ef_spawn((int)par4, 0x6e7);
        ef_wave(charging_pierce_wave, target, (struct GameActor *)mode->world->player);
        return;
    }
    if (!e) {
        e = ef_spawn(self->aid, 0x6e4);
        if (!e)
            return;
    }
    charging_pierce_effect_set_charges(e, (uint64_t)par3);
}

void actor_effect_state(struct GameActor *self, struct GameMode *mode, long long par1, long long par2, long long par3,
                        long long par4)
{
    int on = par2 != 0;
    int state = (int)par1;
    int value = (int)par3;
    struct Pc *pc;

    if ((unsigned long long)par1 <= 0x53b && ef_quiet(state))
        return;
    switch ((unsigned long long)par1 <= 0x53b ? state : -1) {
    case 0x02: case 0xa1: ef_toggle_tagged(self, 0xa6, on, 0x82); break;
    case 0x19: case 0xbc: ef_toggle_tagged(self, 0xa6, on, 0x80); break;
    case 0x1d:
        if (on) {
            self->state_locked = 1;
            if (game_actor_get_transform_job(self) > 0)
                render_object_set_action((struct RenderObject *)self, 0x20, 1);
            else
                render_object_set_action((struct RenderObject *)self, game_actor_action_of(self, 3), 1);
            self->motion_held = 1;
        } else {
            self->state_locked = 0;
            self->state = 0;
            self->slant_depth = 0;
            self->pickable = 1;
            actor_set_state(self, 0, 1);
            self->motion_held = 0;
        }
        break;
    case 0x1f: ef_toggle_tagged(self, 0xa6, on, 0xa9); break;
    case 0x22:
        if (self->aid == g_session1.ownAid) {
            struct GameMode *m;
            Vec3 v;

            if (on) {
                g_wobbleFramesLeft = 10000;
                g_wobbleMode = 2;
                m = game_current_mode();
                if (!m)
                    break;
                v.y = -1.0f;
            } else {
                g_wobbleFramesLeft = 0;
                m = game_current_mode();
                if (!m)
                    break;
                v.y = 1.0f;
            }
            v.x = 0.0f;
            v.z = 0.0f;
            view_set_up(m->view, &v);
        }
        break;
    case 0x44: ef_toggle_tagged(self, 0xa6, on, 0xfa); break;
    case 0x46: case 0x4a: case 0x53: case 0x1c6: ef_toggle_once(self, 0x361, on); break;
    case 0x47: case 0x4d: ef_toggle_once(self, 0x369, on); break;
    case 0x48: ef_toggle_once(self, 0x36d, on); break;
    case 0x49: ef_toggle_once(self, 0x35d, on); break;
    case 0x4b: ef_toggle_once(self, 0x26, on); break;
    case 0x4c: case 0x50: ef_toggle_once(self, 0x367, on); break;
    case 0x4e: ef_toggle_once(self, 0x363, on); break;
    case 0x4f: case 0x52: case 0x1c0: ef_toggle_once(self, 0x360, on); break;
    case 0x51: case 0x54: ef_toggle_once(self, 0x36b, on); break;
    case 0x56:
        ef_toggle(self, 0xcb, on);
        ef_remove(self, 0xa6);
        if (on) {
            struct RagEffect *e = ef_launch(self, 0xa6);

            if (e)
                rag_effect_send_msg(e, NULL, 0x2c, 0xfffe, 0x80, 0);
        }
        break;
    case 0x57: case 0xeb:
        while (able_to_make_effect_is_effect_exist(self, 0xfe)) {
            render_object_send(self, NULL, 0x6d, 0xfe, 0, 0, 0);
            render_object_send(self, NULL, 0x6d, 0x46c, 0, 0, 0);
        }
        pc = object_as_type(self, OBJECT_PC);
        if (pc && (self->aid == g_session1.ownGid || session_is_pc_job(&g_session1, game_actor_get_job(self)))) {
            if (on)
                pc->virtue |= 0x10;
            else
                pc->virtue ^= 0x10;
            if (!(pc->virtue & 0x10) && !(pc->hat_flags & 0x10))
                break;
        } else if (!on) {
            break;
        }
        ef_launch(self, 0xfe);
        ef_launch(self, 0x46c);
        break;
    case 0x5f: ef_remove(self, 0x1c5); break;
    case 0x61:
        if (!on) {
            while (able_to_make_effect_is_effect_exist(self, 0x28f)) {
                render_object_send(self, NULL, 0x6d, 0x28f, 0, 0, 0);
                ef_launch(self, 0x290);
            }
        } else if (!able_to_make_effect_is_effect_exist(self, 0x28f)) {
            ef_launch(self, 0x28f);
        }
        break;
    case 0x67:
        ef_remove(self, 0x16f);
        ef_remove(self, 0x1da);
        if (on) {
            ef_launch(self, 0x16f);
            ef_launch(self, 0x1da);
        }
        break;
    case 0x69:
        render_object_send(self, NULL, 0x6d, 0x171, 0, 0, 0);
        if (on)
            ef_launch(self, 0x171);
        break;
    case 0x6b: case 0x1d0: ef_toggle(self, 0x170, on); break;
    case 0x6e: case 0x1d9: case 0x461: ef_toggle(self, 0x177, on); break;
    case 0x71: ef_toggle(self, 0x17e, on); break;
    case 0x79:
        render_object_send(self, NULL, 0x6d, 0x18b, 0, 0, 0);
        ef_toggle(self, 0x18c, on);
        break;
    case 0x7a: ef_toggle(self, 0x18c, on); break;
    case 0x7b: ef_toggle(self, 0x18a, on); break;
    case 0x85:
        if (able_to_make_effect_is_effect_exist(self, 0x1ba))
            ef_launch(self, 0x1bb);
        render_object_send(self, NULL, 0x6d, 0x1ba, 0, 0, 0);
        break;
    case 0x86: {
        struct GameActor *player = (struct GameActor *)game_current_player();

        ef_wave("effect\\T_벽튕김.wav", self, player);
        ef_launch(self, 0x1aa);
        if (able_to_make_effect_is_effect_exist(self, 0x1ba))
            render_object_send(self, NULL, 0x6d, 0x1ba, 0, 0, 0);
        break;
    }
    case 0x95: case 0x420: case 0x421: case 0x422: case 0x423:
        if (self->aid == g_session1.ownAid) {
            if (on)
                weather_set_dim(&g_weather);
            else
                weather_clear_dim(&g_weather);
        }
        if (!on)
            ef_remove(self, 0x1a8);
        break;
    case 0x98:
        if (self->aid == g_session1.ownAid)
            ef_blind(self, value);
        break;
    case 0x99: ef_toggle(self, 0x1a3, on); break;
    case 0x9f:
        if (on)
            ef_launch(self, 0x222);
        break;
    case 0xa0:
        if (self->aid == g_session1.ownAid) {
            if (on)
                weather_set_dim(&g_weather);
            else
                weather_clear_dim(&g_weather);
        }
        break;
    case 0xa5: case 0xa6: case 0xa7: case 0xe8:
        ef_remove(self, 0x1a2);
        ef_remove(self, 0x220);
        if (on) {
            ef_launch(self, 0x1a2);
            ef_launch(self, 0x220);
        }
        break;
    case 0xc6: ef_toggle(self, 0x259, on); break;
    case 0xc8: case 0xc9: ef_toggle(self, 0x25c, on); break;
    case 0xcf: ef_toggle(self, 0x269, on); break;

    case 0x107: ef_toggle(self, 0x24a, on); break;
    case 0x108:
        if (self->aid == g_session1.ownAid) {
            g_wobbleFramesLeft = 1;
            g_wobbleMode = 1;
        }
        break;
    case 0x115: case 0x116: case 0x117: case 0x118: case 0x119: {
        const char *word = state == 0x115 ? word_flee_up : state == 0x116 ? word_atk_up
                         : state == 0x117 ? word_hp_up : state == 0x118 ? word_sp_up : word_dex_up;

        render_object_send(self, NULL, 7, (int)word, 0, 0, 0);
        ef_launch(self, 0x1c8);
        break;
    }
    case 0x148: ef_toggle(self, 0x314, on); break;
    case 0x14a: ef_toggle(self, 0x2f3, on); break;
    case 0x14e: case 0x365: ef_toggle_tagged(self, 0x318, on, 0x318); break;
    case 0x153: {
        int id, n;

        if (par2 == 1) {
            if (par3 < 1 || par3 > 10)
                break;
            for (id = 0x5bb, n = 10; n != 0; id++, n--) {
                ef_remove_type(self->aid, id - 0xa7);
                ef_remove_type(self->aid, id);
            }
            ef_spawn(self->aid, g_session1.map_rule_flags[0] == 1 ? value + 0x5ba : value + 0x513);
        } else {
            for (id = 0x5bb, n = 10; n != 0; id++, n--) {
                ef_remove_type(self->aid, id - 0xa7);
                ef_remove_type(self->aid, id);
            }
        }
        break;
    }
    case 0x15e:
        ef_remove(self, 0x341);
        if (on) {
            struct RagEffect *e = ef_launch(self, 0x341);
            Vec3 v = { 2.0f, 0.0f, 0.0f };

            if (e) {
                long long p = (int)&v;

                rag_effect_send_msg(e, NULL, 0xe, p, 0, 0);
            }
        }
        break;
    case 0x15f: ef_toggle(self, 0x315, on); break;
    case 0x166: ef_toggle(self, 0x2f0, on); break;
    case 0x168:
        ef_toggle(self, 0x2e8, on);
        if (self->monster_gage_wnd)
            ui_pc_gage_set_two_bars((struct UIPcGage *)self->monster_gage_wnd, on ? 0 : 1);
        break;
    case 0x16b: case 0x16c: case 0x16d: case 0x16e: case 0x16f:
        game_actor_set_tetra_ball(self, state - 0x16b, (int)par2, value);
        break;
    case 0x17b: ef_toggle(self, 0x2e8, on); break;
    case 0x18a:
        while (able_to_make_effect_is_effect_exist(self, 0x329)) {
            render_object_send(self, NULL, 0x6d, 0x329, 0, 0, 0);
            render_object_send(self, NULL, 0x6d, 0x333, 0, 0, 0);
        }
        if (on)
            ef_launch(self, 0x329);
        break;
    case 0x195:
        if (on && !able_to_make_effect_is_effect_exist(self, 0x38d))
            ef_launch(self, 0x38d);
        break;
    case 0x1a0: case 0x1a1:
        if (on) {
            self->motion_held = 1;
            self->argb_locked = 1;
            self->argb = (self->argb & 0xff000000u) | 0x404040u;
        } else {
            self->motion_held = 0;
            self->argb_locked = 0;
            self->argb |= 0x00ffffffu;
        }
        break;
    case 0x1a6:
        ef_remove(self, 0x337);
        if (on) {
            sound_play_wave(manhole_wave, 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
            if (!job_is_monster(game_actor_get_job(self)))
                ef_launch(self, 0x337);
        }
        break;
    case 0x1b1: ef_toggle(self, 0x1c5, on); break;
    case 0x1b2:
        if (par2 == 1) {
            render_object_send(self, NULL, 0x6d, 0x36f, 0, 0, 0);
            ef_launch(self, 0x36f);
        } else {
            render_object_send(self, NULL, 0x6d, 0x36f, 0, 0, 0);
        }
        break;
    case 0x1b3:
        render_object_send(self, NULL, par2 == 1 ? 0x45 : 0x46, 0, 0, 0, 0);
        break;
    case 0x1b5: ef_toggle(self, 0x3ac, on); break;
    case 0x1b7: case 0x1b9: case 0x1c3:
        if (par2 == 1) {
            int id = state == 0x1b7 ? 0x361 : state == 0x1b9 ? 0x363 : 0x367;

            render_object_send(self, NULL, 0x6d, id, 0, 0, 0);
            ef_launch(self, id);
        }
        break;
    case 0x1bd:
        if (session_is_pc_job(&g_session1, game_actor_get_job(self)))
            ef_toggle_tagged(self, 0x39d, on, 0x39d);
        break;
    case 0x1c4: ef_toggle_once(self, 0x57, on); break;
    case 0x1c5: ef_toggle_once(self, 0x31f, on); break;

    case 0x23b: effect_mgr_spawn_level99_aura(g_effectMgr, self, (unsigned char)on, value); break;
    case 0x25e: case 0x28f: ef_toggle(self, 0x3c3, on); break;
    case 0x265: ef_mount(self, on); break;
    case 0x26d: case 0x28b: case 0x299: {
        int index = state == 0x26d ? 0 : state == 0x28b ? 1 : 2;

        if (on) {
            game_actor_set_transform_job(self, value, index);
        } else {
            game_actor_set_transform_job(self, -1, index);
            if (self->state == 3)
                render_object_set_action((struct RenderObject *)self, game_actor_action_of(self, 3), 1);
        }
        break;
    }
    case 0x285:
        ef_remove_type(self->aid, 0x3e6);
        if (on && effect_mgr_is_registered(g_effectMgr, 0x3e6))
            ef_spawn_at(self->aid, 0x3e6, &self->pos, -1);
        break;
    case 0x286:
        ef_remove(self, 0x18a);
        if (on) {
            struct RagEffect *e = ef_launch(self, 0x18a);

            if (e) {
                e->color_r = 0xb4;
                e->color_g = 0xb4;
                e->color_b = -1;
            }
        }
        break;
    case 0x290: ef_toggle(self, 0x3ec, on); break;
    case 0x291: ef_toggle(self, 0x3eb, on); break;
    case 0x293:
        ef_remove_type(self->aid, 0x3ed);
        if (on && effect_mgr_is_registered(g_effectMgr, 0x3ed)) {
            struct Effect *e = ef_spawn(self->aid, 0x3ed);

            if (e)
                energy_orb_effect_set_ring_count(e, 10);
        }
        ef_toggle(self, 0x3c3, on);
        break;
    case 0x294: ef_toggle(self, 0x220, on); break;
    case 0x2a0: self->name_masked = on ? 1 : 0; break;
    case 0x2a1:
        if (on) {
            if (self->mark_kind == par3)
                break;
            self->mark_kind = value;
            if (is_effect_state_burrow(self->effect_state) || is_effect_state_star2(self->effect_state)
                || effect_state_bit6(self->effect_state))
                break;
            render_object_send(self, NULL, 0x34, 0, 0, 0, 0);
            render_object_send(self, NULL, 0x33, 0, 0, 0, 0);
        } else {
            self->mark_kind = 0;
            render_object_send(self, NULL, 0x34, 0, 0, 0, 0);
        }
        break;
    case 0x2a2: game_actor_set_hat_effect_by_aid(self, self->aid, 0x3f4, (unsigned char)on); break;
    case 0x2a3: case 0x397:
        if (!on)
            ef_remove_type(self->aid, state == 0x2a3 ? 0x3f5 : 0x449);
        else if (state == 0x2a3)
            ef_spawn(self->aid, 0x3f5);
        else if (!effect_mgr_has_effect(g_effectMgr, self->aid, 0x449))
            ef_spawn_at(self->aid, 0x449, &self->pos, -1);
        break;
    case 0x2a4: ef_toggle(self, 0x3fb, on); break;
    case 0x2c6:
        if (on)
            render_object_send(self, NULL, 0x7f, 0x32, 0x96, 0xff, 0);
        break;
    case 0x2c9: ef_toggle(self, 0x40d, on); break;
    case 0x2cc: ef_toggle(self, 0x421, on); break;
    case 0x2cd: ef_toggle(self, 0x418, on); break;
    case 0x2ce: game_actor_set_hat_effect_by_aid(self, self->aid, 0x40a, (unsigned char)on); break;
    case 0x2cf: game_actor_set_hat_effect_by_aid(self, self->aid, 0x40b, (unsigned char)on); break;
    case 0x2d2:
        render_object_send(self, self, 0x6d, 0x420, 0, 0, 0);
        if (on && effect_mgr_is_registered(g_effectMgr, 0x420))
            ef_spawn_at(self->aid, 0x420, &self->pos, -1);
        break;
    case 0x2d8: game_actor_set_hat_effect_by_aid(self, self->aid, 0x40c, (unsigned char)on); break;
    case 0x2da:
        ef_remove(self, 0x410);
        if (on) {
            ef_launch(self, 0x410);
            ef_launch(self, 0x410);
            ef_launch(self, 0x410);
        }
        break;
    case 0x2de: ef_toggle(self, 0x419, on); break;
    case 0x2df: ef_toggle(self, 0x194, on); break;
    case 0x2e0: ef_toggle(self, 0x41f, on); break;
    case 0x2e1: ef_toggle(self, 0x41e, on); break;
    case 0x2f2: ef_spawn_at(0, 0x434, &self->pos, -1); break;
    case 0x2f3:
        render_object_send(self, self, 0x6d, 0x433, 0, 0, 0);
        if (on)
            ef_2f3(self);
        break;
    case 0x2f4:
        if (on)
            ef_spawn_at(self->aid, 0x473, &self->pos, -1);
        else
            ef_remove_type(self->aid, 0x473);
        break;
    case 0x2f5:
        if (on) {
            render_object_send(self, NULL, 0x6d, 0x425, 0, 0, 0);
            ef_spawn_at(0, 0x432, &self->pos, -1);
        }
        break;
    case 0x2f8:
        if (on)
            ef_spawn_at(self->aid, 0x470, &self->pos, -1);
        else
            ef_remove_type(self->aid, 0x470);
        break;
    case 0x30b: game_actor_set_hat_effect_by_aid(self, self->aid, 0x437, (unsigned char)on); break;

    case 0x336: game_actor_set_hat_effect_by_aid(self, self->aid, 0x43b, (unsigned char)on); break;
    case 0x337: game_actor_set_hat_effect_by_aid(self, self->aid, 0x43c, (unsigned char)on); break;
    case 0x339: game_actor_set_hat_effect_by_aid(self, self->aid, 0x43d, (unsigned char)on); break;
    case 0x33a: game_actor_set_hat_effect_by_aid(self, self->aid, 0x43e, (unsigned char)on); break;
    case 0x351: game_actor_set_hat_effect_by_aid(self, self->aid, 0x43f, (unsigned char)on); break;
    case 0x358:
        if (g_session1.ownAid == self->aid)
            g_session1.ownDressUp = value;
        self->mount_look = value;
        actor_set_spr_job(self, game_actor_get_job(self));
        actor_set_imf_name(self);
        actor_set_head_pal_name(self);
        actor_set_body_pal_name(self);
        break;
    case 0x359: game_actor_set_hat_effect_by_aid(self, self->aid, 0x441, (unsigned char)on); break;
    case 0x35b:
        if (on)
            ef_spawn_at(self->aid, 0x442, &self->pos, -1);
        else
            ef_remove_type(self->aid, 0x442);
        break;
    case 0x35f: game_actor_set_hat_effect_by_aid(self, self->aid, 0x444, (unsigned char)on); break;
    case 0x360: game_actor_set_hat_effect_by_aid(self, self->aid, 0x443, (unsigned char)on); break;
    case 0x368: game_actor_set_hat_effect_by_aid(self, self->aid, 0x447, (unsigned char)on); break;
    case 0x37d:
        if (on)
            actor_set_state(self, 0x2f, 1);
        else if (!self->pose_locked)
            actor_set_state(self, 0, 1);
        break;
    case 0x37f: game_actor_set_hat_effect_by_aid(self, self->aid, value + 0x950, (unsigned char)on); break;
    case 0x380:
        if (on)
            ef_spawn_at(self->aid, 0x456, &self->pos, -1);
        else
            ef_remove_type(self->aid, 0x456);
        break;
    case 0x395:
        if (on)
            ef_spawn_at(self->aid, 0x45f, &self->pos, -1);
        else
            ef_remove_type(self->aid, 0x45f);
        break;
    case 0x3a5:
        if (on) {
            actor_set_state(self, 0x30, 1);
            self->motion_held = 1;
            self->pose_locked = 1;
        } else {
            self->state = 0;
            self->slant_depth = 0;
            self->pickable = 1;
            actor_set_state(self, 0, 1);
            self->motion_held = 0;
            self->pose_locked = 0;
        }
        break;
    case 0x3a9:
        ef_remove_type(self->aid, 0x448);
        if (on)
            ef_spawn_at(self->aid, 0x448, &self->pos, -1);
        break;
    case 0x3d6:
        pc = object_as_type(self, OBJECT_PC);
        if (pc && session_is_doram_job(&g_session1, game_actor_get_job(self)))
            pc->soul_attack = (unsigned char)on;
        break;
    case 0x3f6:
        if (on) {
            ef_launch(self, 0x47c);
        } else {
            while (able_to_make_effect_is_effect_exist(self, 0x47c) == 1)
                render_object_send(self, NULL, 0x6d, 0x47c, 0, 0, 0);
        }
        break;
    case 0x3f8: case 0x3f9: case 0x3fa: case 0x3fb: case 0x3fc:
    case 0x3fd: case 0x3fe: case 0x3ff: case 0x400: case 0x401:
        if (on)
            ef_spawn(self->aid, state + 0x87);
        else if (effect_mgr_has_effect(g_effectMgr, self->aid, state + 0x87) == 1)
            ef_remove_type(self->aid, state + 0x87);
        break;
    case 0x40e: ef_respawn(self->aid, 0x4bf, on); break;
    case 0x40f: ef_respawn(self->aid, 0x4c1, on); break;
    case 0x410: ef_respawn(self->aid, 0x4be, on); break;
    case 0x411: ef_respawn(self->aid, 0x4c5, on); break;
    case 0x413: ef_respawn(self->aid, 0x4c0, on); break;
    case 0x41a: case 0x41f: {
        struct GameActor *player;

        ef_respawn(self->aid, state == 0x41a ? 0x4b1 : 0x4c6, on);
        if (!on)
            break;
        player = (struct GameActor *)game_current_player();
        ef_wave(state == 0x41a ? "effect\\t_캐스팅.wav" : "effect\\t_바람방출.wav", self, player);
        break;
    }

    case 0x465:
        if (par2 == 1) {
            if (effect_mgr_find_effect(g_effectMgr, self->aid, 0x552))
                break;
            ef_spawn(self->aid, 0x552);
            sound_play_wave("effect\\s포이즌버스터.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
        } else {
            ef_end(self->aid, 0x552);
        }
        break;
    case 0x466: case 0x48e: {
        struct GameActor *who = ef_actor(self, mode);
        int type = state == 0x466 ? 0x66c : 0x63f;

        if (!who)
            break;
        ef_remove_type(who->aid, type);
        if (on)
            ef_spawn(who->aid, type);
        break;
    }
    case 0x473: case 0x475: case 0x476: case 0x47c:
        ef_toggle_once(ef_actor(self, mode), state == 0x473 ? 0x576 : state == 0x475 ? 0x58c
                                             : state == 0x476 ? 0x18c : 0x5f5, on);
        break;
    case 0x47d: ef_47d(self, mode, par2, par3); break;
    case 0x47e: ef_spawn_or_end(self->aid, 0x6cd, on); break;
    case 0x480:
        ef_remove_type(self->aid, 0x708);
        if (on)
            ef_spawn_at(self->aid, 0x708, &self->pos, value);
        break;
    case 0x488: if (on) ef_launch(self, 0x8f6); break;
    case 0x48a: case 0x4ed: if (on) ef_launch(self, 0x8f8); break;
    case 0x48d: case 0x48f: {
        struct GameActor *who = ef_actor(self, mode);

        if (who)
            ef_toggle_once(who, state == 0x48d ? 0x64a : 0x641, on);
        break;
    }
    case 0x495:
        if (on) {
            struct Effect *e = ef_spawn(self->aid, 0x6ae);

            if (e)
                e->ops->set_start_delay(e, 700);
            e = ef_spawn(self->aid, 0x8e8);
            if (e)
                e->ops->set_start_delay(e, 700);
        } else {
            ef_remove_type(self->aid, 0x6ae);
            ef_remove_type(self->aid, 0x8e8);
        }
        break;
    case 0x496: {
        struct Effect *e = effect_mgr_find_effect(g_effectMgr, self->aid, 0x6e4);

        if (on) {
            if (!e)
                ef_spawn(self->aid, 0x6e4);
        } else {
            effect_mgr_remove(g_effectMgr, e, 1);
        }
        break;
    }
    case 0x497: ef_charging_pierce(self, mode, par3, par4); break;
    case 0x498: {
        struct GameActor *who = ef_actor(self, mode);

        if (!who)
            break;
        ef_remove_type(who->aid, 0x6be);
        ef_remove_type(who->aid, 0x6bf);
        if (on) {
            ef_spawn(who->aid, 0x6be);
            ef_spawn(who->aid, 0x6bf);
        }
        break;
    }
    case 0x49a: ef_spawn_or_remove(self->aid, 0x6d7, on); break;
    case 0x4a0:
        if (on) {
            if (!effect_mgr_find_effect(g_effectMgr, self->aid, 0x743))
                ef_spawn(self->aid, 0x743);
        } else {
            ef_remove_type(self->aid, 0x743);
        }
        break;
    case 0x4a6: ef_spawn_or_remove(self->aid, 0x7c7, on); break;
    case 0x4a9: ef_spawn_once_or_end(self->aid, 0x725, on); break;
    case 0x4ac:
        if (on)
            ef_spawn_at(self->aid, 0x779, &self->pos, value);
        else
            ef_remove_type(self->aid, 0x779);
        break;
    case 0x4ad:
        if (on) {
            if (g_session1.map_rule_flags[0] != 0 || effect_mgr_find_effect(g_effectMgr, self->aid, 0x712))
                break;
            ef_spawn(self->aid, 0x712);
        } else {
            ef_remove_type(self->aid, 0x712);
        }
        break;
    case 0x4b0: if (on) ef_launch(self, 0x8f0); break;
    case 0x4b1:
        if (on) {
            if (!effect_mgr_find_effect(g_effectMgr, self->aid, 0x73d))
                ef_spawn(self->aid, 0x73d);
            if (g_session1.map_rule_flags[0] == 0 && !effect_mgr_find_effect(g_effectMgr, self->aid, 0x73c))
                ef_spawn(self->aid, 0x73c);
            ef_launch(self, 0x8fc);
        } else {
            ef_end(self->aid, 0x73d);
            if (g_session1.map_rule_flags[0] == 0)
                ef_end(self->aid, 0x73c);
        }
        break;
    case 0x4b4: if (on) ef_spawn(self->aid, 0x88e); break;
    case 0x4b5:
        if (self->aid != g_session1.ownAid)
            break;
        if (par2 == 1) {
            if (!able_to_make_effect_is_effect_exist(self, 0x14e))
                ef_launch(self, 0x14e);
            g_playerBlind = 1;
            render_object_send(self, NULL, 0x6d, 0xa4, 0, 0, 0);
            ef_player_darkness(self);
        } else {
            ef_remove_once(self, 0x14e);
            g_playerBlind = 0;
        }
        game_actor_set_instant_var(self, 0x10, on);
        break;
    case 0x4b6: render_object_send(self, NULL, par2 == 1 ? 0x9c : 0x9d, 0, 0, 0, 0); break;
    case 0x4b7:
        if (par2 == 1) {
            render_object_send(self, NULL, 0x98, 0, 0, 0, 0);
            ef_spawn(self->aid, 0x8c4);
        } else {
            render_object_send(self, NULL, 0x99, 0, 0, 0, 0);
        }
        break;
    case 0x4b8:
        if (on) {
            ef_spawn(self->aid, 0x8de);
        } else if (effect_mgr_has_effect(g_effectMgr, self->aid, 0x8de)) {
            ef_remove_type(self->aid, 0x8de);
            ef_spawn(self->aid, 0x8df);
        }
        if (self->aid == g_session1.ownAid)
            ef_spawn_or_remove(self->aid, 0x8e0, on);
        break;
    case 0x4b9:
        if (par2 == 1) {
            render_object_send(self, NULL, 0x9c, 0, 0, 0, 0);
            ef_remove_type(self->aid, 0x8c5);
            ef_spawn(self->aid, 0x8c5);
        } else {
            render_object_send(self, NULL, 0x9d, 0, 0, 0, 0);
            ef_remove_type(self->aid, 0x8c5);
        }
        break;
    case 0x4ba:
        ef_remove_type(self->aid, 0x8dc);
        if (on)
            ef_spawn(self->aid, 0x8dc);
        break;
    case 0x4bb: ef_spawn_or_remove(self->aid, 0x8e1, on); break;
    case 0x4bc:
        if (on) {
            render_object_send(self, NULL, 0x7f, 0xff, 0, 0, 0);
            ef_remove_type(self->aid, 0x8dd);
            ef_spawn(self->aid, 0x8dd);
        } else {
            render_object_send(self, NULL, 0x80, 0, 0, 0, 0);
            ef_remove_type(self->aid, 0x8dd);
        }
        game_actor_set_instant_var(self, 0xd, on);
        break;
    case 0x4bd: render_object_send(self, NULL, on ? 0x9a : 0x9b, 0, 0, 0, 0); break;
    case 0x4be:
        if (on) {
            render_object_send(self, NULL, 0x7f, 0xaa, 0, 0xaa, 0);
            ef_spawn(self->aid, 0x8e2);
        } else {
            render_object_send(self, NULL, 0x80, 0, 0, 0, 0);
            ef_remove_type(self->aid, 0x8e2);
        }
        game_actor_set_instant_var(self, 0xe, on);
        break;
    case 0x4bf:
        if (on)
            render_object_send(self, NULL, 0x7f, 0, 0, 0xff, 0);
        else
            render_object_send(self, NULL, 0x80, 0, 0, 0, 0);
        game_actor_set_instant_var(self, 0xf, on);
        break;
    case 0x4c0:
        if (on) {
            render_object_send(self, NULL, 0x7f, 0xff, 0, 0, 0);
            ef_spawn(self->aid, 0x8c6);
        } else {
            render_object_send(self, NULL, 0x80, 0, 0, 0, 0);
        }
        game_actor_set_instant_var(self, 0xc, on);
        break;
    case 0x4c4:
        if (on) {
            struct Effect *e;

            ef_spawn(self->aid, 0x885);
            e = ef_spawn(self->aid, 0x886);
            if (e)
                e->ops->set_start_delay(e, 0x220);
        } else {
            ef_remove_type(self->aid, 0x886);
        }
        break;
    case 0x4c5: if (on) ef_spawn(self->aid, 0x89a); break;
    case 0x4c6: ef_spawn_or_remove(self->aid, 0x881, on); break;
    case 0x4ce: case 0x4cf:
        if (on) {
            struct Effect *e1, *e2;

            ef_spawn(self->aid, state == 0x4ce ? 0x806 : 0x821);
            e1 = ef_spawn(self->aid, state == 0x4ce ? 0x803 : 0x81e);
            e2 = ef_spawn(self->aid, state == 0x4ce ? 0x804 : 0x81f);
            if (e1)
                e1->ops->set_start_delay(e1, 1000);
            if (e2)
                e2->ops->set_start_delay(e2, 1000);
        } else {
            ef_remove_type(self->aid, state == 0x4ce ? 0x803 : 0x81e);
            ef_remove_type(self->aid, state == 0x4ce ? 0x804 : 0x81f);
        }
        break;
    case 0x4d0: case 0x4d1:
        if (on) {
            ef_spawn(self->aid, state == 0x4d0 ? 0x7ba : 0x7be);
            ef_spawn(self->aid, state == 0x4d0 ? 0x7bd : 0x7c1);
        } else {
            ef_remove_type(self->aid, state == 0x4d0 ? 0x7ba : 0x7be);
            ef_remove_type(self->aid, state == 0x4d0 ? 0x7bd : 0x7c1);
        }
        break;
    case 0x4d2: ef_spawn_or_remove(self->aid, 0x7b7, on); break;
    case 0x4d4: ef_spawn_or_remove(self->aid, 0x72f, on); break;
    case 0x4d5:
        if (on) {
            ef_spawn(self->aid, 0x719);
            ef_launch(self, 0x8f8);
        } else {
            ef_end(self->aid, 0x719);
        }
        break;
    case 0x4dd: ef_spawn_or_remove(self->aid, 0x84e, on); break;
    case 0x4e1: if (on) ef_spawn(self->aid, 0x8be); break;
    case 0x4e2: ef_spawn_or_remove(self->aid, 0x76c, on); break;
    case 0x4e4: ef_spawn_or_end(self->aid, 0x7aa, on); break;
    case 0x4e8: ef_spawn_or_remove(self->aid, 0x7d2, on); break;
    case 0x4ea: ef_spawn_or_end(self->aid, 0x80c, on); break;
    case 0x4eb: if (on) ef_launch(self, 0x8f5); break;
    case 0x4ec: if (on) ef_launch(self, 0x8f9); break;
    case 0x4ee: if (on) ef_launch(self, 0x8fa); break;
    case 0x4ef: if (on) ef_launch(self, 0x8f6); break;
    case 0x4f7: if (on) ef_launch(self, 0x8fa); break;
    case 0x515:
        if (on)
            ef_launch(self, 0x2e8);
        else
            ef_remove_once(self, 0x2e8);
        break;
    case 0x539: case 0x53a: {
        int type = state == 0x539 ? 0x901 : 0x902;

        if (on) {
            ef_spawn(self->aid, type);
        } else {
            struct Effect *e = effect_mgr_find_effect(g_effectMgr, self->aid, type);

            if (e)
                curse_of_cube_effect_shock(e);
        }
        break;
    }
    case 0x53b:
        if (on) {
            ef_spawn(self->aid, 0x8fe);
        } else {
            ef_remove_type(self->aid, 0x8fe);
            ef_spawn(self->aid, 0x8ff);
        }
        break;

    default:
        if (on == 1)
            skill_action_mgr_efst_on(skill_action_mgr_instance(), state, value, (SkillActor *)(self));
        else
            skill_action_mgr_efst_off(skill_action_mgr_instance(), state, value, (SkillActor *)(self));
        break;
    }
}
