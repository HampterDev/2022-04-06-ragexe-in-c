#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "contents/client_lua_calls.h"
#include <windows.h>
#include <mmsystem.h>
#include "actor/render_object.h"
#include "resources/attr_queries.h"
#include "session/state.h"
#include "game/effect_notify.h"
#include "game/cells.h"
#include "game/ui_bridge.h"
#include "effects/effect_controls.h"
#include "actor/actor_common.h"
#include "actor/game_actor.h"
#include "effects/skills/emitter_effect.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "session/session_windows.h"
#include "world/world.h"

#pragma pack(push, 1)
struct PacketNotifyEffect {
    unsigned short packet_type;
    int aid;
    int effect;
};
#pragma pack(pop)

void game_mode_on_notify_effect(struct GameMode *self, const struct PacketNotifyEffect *packet)
{
    static const short effect_ids[10] = {
        0x173, 0x09e, 0x09b, 0x09a, 0, 0x131, 0x132, 0x152, 0x151, 0x246,
    };
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);
    Vec3 at = { 0.0f, 0.0f, 0.0f };

    if (actor == 0 || (unsigned)packet->effect > 9)
        return;
    if (packet->effect == 4)
        g_session1.game_over = 1;
    else
        game_actor_launch_effect(actor, effect_ids[packet->effect], at, 0.0f);
}

enum {
    EF_SMOKE_SOURCE = 0x3fd,
    EF_SMOKE_EMITTER = 0x3ce,
    EF_HOMUNCULUS_COUNTER = 0x238,
};

#pragma pack(push, 1)
struct PacketNotifyEffect2 {
    unsigned short packet_type;
    int aid;
    int effect_id;
};
#pragma pack(pop)

static void start_smoke(struct GameActor *actor)
{
    Vec3 at;
    struct EmitterEffect *smoke;
    struct String texture;

    at.x = actor->pos.x;
    at.y = actor->pos.y;
    at.z = actor->pos.z;
    smoke = (struct EmitterEffect *)effect_mgr_spawn(g_effectMgr, 0, EF_SMOKE_EMITTER, at, 0, -1, 0, 0);
    smoke->velocity_min.x = -2.0f; smoke->velocity_min.y = 2.0f;  smoke->velocity_min.z = -2.0f;
    smoke->velocity_max.x = 0.0f;  smoke->velocity_max.y = -1.0f; smoke->velocity_max.z = 0.0f;
    smoke->acceleration.x = 0.0f;  smoke->acceleration.y = -7.5f; smoke->acceleration.z = 0.0f;
    smoke->spawn_box.x = -1.0f;    smoke->spawn_box.y = 2.0f;     smoke->spawn_box.z = -1.0f;
    smoke->r = 0xff;
    smoke->g = 0xff;
    smoke->b = 100;
    smoke->alpha = 100;
    smoke->rate_min = 20.0f;
    smoke->rate_max = 100.0f;
    smoke->size_min = 30.0f;
    smoke->size_max = 30.0f;
    smoke->life_min = 5.0f;
    smoke->life_max = 10.0f;
    smoke->src_blend = 5;
    smoke->dest_blend = 2;
    emitter_effect_set_particle_speed(smoke, 3.0f);
    emitter_effect_set_count(smoke, 30);

    string_init(&texture);
    string_assign_cstr(&texture, "smoke2.bmp");
    string_assign(&smoke->texture, string_cstr(&texture), texture.size);
    string_free(&texture);
    smoke->max_frames = 0x4e2;
}

void game_mode_on_notify_effect2(struct GameMode *self, const struct PacketNotifyEffect2 *packet)
{
    struct GameActor *actor = world_get_actor_or_player(self->world, packet->aid);
    int effect_id;
    Vec3 at;

    if (actor == 0)
        return;
    effect_id = packet->effect_id;
    if (effect_id == EF_SMOKE_SOURCE) {
        start_smoke(actor);
        effect_id = packet->effect_id;
    }
    if (g_session1.map_rule_flags[0]) {
        if (effect_id == 0x5a)
            effect_id = 0x48a;
        else if (effect_id == 0x5c)
            effect_id = 0x48b;
        else if (effect_id == 0x5f)
            effect_id = 0x489;
    }

    at.x = actor->pos.x;
    at.y = actor->pos.y;
    at.z = actor->pos.z;
    if (effect_mgr_spawn(g_effectMgr, actor->aid, effect_id, at, 0, -1, 0, 0) == 0) {
        Vec3 here = { 0.0f, 0.0f, 0.0f };
        game_actor_launch_effect(actor, packet->effect_id, here, 0.0f);
    }

    if (session_is_homunculus_job(&g_session1, game_actor_get_job(actor)) && packet->effect_id == EF_HOMUNCULUS_COUNTER) {
        actor->level++;
        game_actor_stop_aura_effects(actor);
        game_actor_show_level_aura(actor);
    }
}

enum {
    EF_HEAL_HP = 0x14b,
    EF_HEAL_SP = 0x14c,
    EF_NUMBER_FIRST = 0x291,
    EF_NUMBER_LAST = 0x298,
    ACTOR_MSG_SHOW_NUMBER = 0x58,
};

static void show_number(struct GameActor *actor, __int64 num, unsigned color)
{
    render_object_send(actor, NULL, ACTOR_MSG_SHOW_NUMBER, num, color, 0x72, 0);
}

void game_mode_notify_effect_with_num(struct GameMode *self, int aid, int effect_id, __int64 num)
{
    static const unsigned number_colors[EF_NUMBER_LAST - EF_NUMBER_FIRST + 1] = {
        0xff00ff00, 0xff0000ff, 0xffff0000, 0xffff00ff, 0xff000000, 0xffffffff, 0xffffff00, 0xffff55b1,
    };
    struct GameActor *actor;
    Vec3 at;
    int value;

    if (aid == g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, aid);
    if (actor == 0)
        return;

    if (effect_id == EF_HEAL_HP) {
        if (aid == g_session1.ownAid) {
            value = g_session1.hp + (int)num;
            if (g_session1.hp != value) {
                g_session1.hp = value;
                session_refresh_basic_info(&g_session1);
            }
        }
        show_number(actor, num, 0xff00ff00);
    } else if (effect_id == EF_HEAL_SP) {
        if (aid == g_session1.ownAid) {
            value = g_session1.sp + (int)num;
            if (g_session1.sp != value) {
                g_session1.sp = value;
                session_refresh_basic_info(&g_session1);
            }
        }
        show_number(actor, num, 0xff0000ff);
    } else if (effect_id >= EF_NUMBER_FIRST && effect_id <= EF_NUMBER_LAST) {
        show_number(actor, num, number_colors[effect_id - EF_NUMBER_FIRST]);
    }

    at.x = actor->pos.x;
    at.y = actor->pos.y;
    at.z = actor->pos.z;
    if (effect_mgr_spawn(g_effectMgr, actor->aid, effect_id, at, 0, -1, 0, 0) == 0) {
        Vec3 here = { 0.0f, 0.0f, 0.0f };
        game_actor_launch_effect(actor, effect_id, here, 0.0f);
    }
}

#pragma pack(push, 1)
struct PacketNotifyEffect3 {
    unsigned short packet_type;
    int aid;
    int effect_id;
    int num;
};
#pragma pack(pop)

void game_mode_on_notify_effect3(struct GameMode *self, const struct PacketNotifyEffect3 *packet)
{
    game_mode_notify_effect_with_num(self, packet->aid, packet->effect_id, packet->num);
}

#pragma pack(push, 1)
struct PacketNotifyEffect4 {
    unsigned short packet_type;
    int aid;
    int effect_id;
    __int64 num;
};
#pragma pack(pop)

void game_mode_on_notify_effect4(struct GameMode *self, const struct PacketNotifyEffect4 *packet)
{
    game_mode_notify_effect_with_num(self, packet->aid, packet->effect_id, packet->num);
}

#pragma pack(push, 1)
struct PacketClearEffect {
    unsigned short packet_type;
    int aid;
    int effect_id;
};
#pragma pack(pop)

void game_mode_on_clear_effect(struct GameMode *self, const struct PacketClearEffect *packet)
{
    struct GameActor *actor = world_get_actor_or_player(self->world, packet->aid);

    if (actor != 0)
        while (able_to_make_effect_is_effect_exist(actor, packet->effect_id))
            render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, packet->effect_id, 0, 0, 0);
    while (effect_mgr_find_effect(g_effectMgr, packet->aid, packet->effect_id) != 0)
        effect_mgr_remove_type(g_effectMgr, packet->aid, packet->effect_id);
}

enum {
    EF_CONGRATULATION = 0x15b,
};

#pragma pack(push, 1)
struct PacketCongratulation {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)

void game_mode_on_congratulation(struct GameMode *self, const struct PacketCongratulation *packet)
{
    struct GameActor *actor;
    Vec3 here = { 0.0f, 0.0f, 0.0f };

    if (g_session1.effectOn == 0)
        return;
    if (packet->aid == g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (actor == 0 || packet->aid == 0)
        return;
    game_actor_launch_effect(actor, EF_CONGRATULATION, here, 0.0f);
}

enum {
    EF_WARNING_PLANE = 0x45a,
};

void game_mode_on_ef_warning_plane(struct GameMode *self, int aid, unsigned short skill_id, int level,
                                          short x, short y, int duration)
{
    struct GameActor *actor = session_get_actor_by_aid(&g_session1, aid);
    struct Effect *effect;
    Vec3 at;
    int width, height, i, j;

    if (!actor || !self->world || !self->world->attr)
        return;
    width = 0;
    height = 0;

    if (!client_lua_call(g_session1.luaState, "GetSkillScale", "dd>dd", (int)skill_id, level, &width, &height))
        return;
    at.x = 0.0f;
    at.y = 0.0f;
    at.z = 0.0f;
    effect = effect_mgr_spawn(g_effectMgr, actor->aid, EF_WARNING_PLANE, at, 0, -1, 0, 0);
    if (effect == 0)
        return;
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            float wx = 0.0f, wz = 0.0f;

            game_mode_cell_to_world(self, x - width / 2 + i, y - height / 2 + j, &wx, &wz);
            at.x = wx;
            at.y = c3d_attr_get_height(self->world->attr, wx, wz);
            at.z = wz;
            warning_plane_effect_add_plane(effect, at);
            warning_plane_effect_set_end_time(effect, timeGetTime() + duration);
        }
    }
}

#pragma pack(push, 1)
struct PacketNpcShowEfstUpdate {
    unsigned short packet_type;
    int aid;
    int effect_state;
    int level;
    int show_efst;
};
#pragma pack(pop)

void game_mode_on_npc_show_efst_update(struct GameMode *self, const struct PacketNpcShowEfstUpdate *packet)
{
    game_mode_game_mode_show_efst(self, packet->aid, packet->effect_state, packet->level, packet->show_efst);
}

enum {
    OPT3_QUICKEN = 0x1,
    OPT3_OVERTHRUST = 0x2,
    OPT3_ENERGYCOAT = 0x4,
    OPT3_EXPLOSIONSPIRITS = 0x8,
    OPT3_STEELBODY = 0x10,
    OPT3_BLADESTOP = 0x20,
    OPT3_AURABLADE = 0x40,
    OPT3_BERSERK = 0x80,
    OPT3_LIGHTBLADE = 0x100,
    OPT3_MOONLIT = 0x200,
    OPT3_MARIONETTE = 0x400,
    OPT3_ASSUMPTIO = 0x800,
    OPT3_WARM = 0x1000,
    OPT3_KAITE = 0x2000,
    OPT3_BUNSIN = 0x4000,
    OPT3_SOULLINK = 0x8000,
    OPT3_UNDEAD = 0x10000,
    OPT3_CONTRACT = 0x20000,
    OPT3_ELEMENTAL_VEIL = 0x40000,
};

#define EFFECT_MSG_TINT         0x2c
#define BLADE_STOP_DIM          0x40

static struct RagEffect *efst_launch(struct GameActor *actor, int effect_id)
{
    Vec3 here = { 0.0f, 0.0f, 0.0f };

    return game_actor_launch_effect(actor, effect_id, here, 0.0f);
}

static void efst_add(struct GameActor *actor, int opt3, int bit, int effect_id)
{
    if ((opt3 & bit) && !able_to_make_effect_is_effect_exist(actor, effect_id))
        efst_launch(actor, effect_id);
}

static void efst_drop(struct GameActor *actor, int effect_id)
{
    render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, effect_id, 0, 0, 0);
}

static void efst_tinted(struct GameActor *actor, int colour)
{
    struct RagEffect *effect;

    if (able_to_make_effect_is_effect_exist(actor, 0xa6))
        return;
    effect = efst_launch(actor, 0xa6);
    if (effect)
        rag_effect_send_msg(effect, NULL, EFFECT_MSG_TINT, 0xffff, colour, 0);
}

static int efst_bladestop_holds(int job)
{
    return job == 0xf || job == 0xfb0 || job == 0xfc6 || job == 0xfe6 || job == 0xfed || job == 0x100a;
}

static void efst_update_player(struct GameActor *actor, int opt3)
{
    if (is_effect_state_burrow(actor_get_effect_state(actor)) || is_effect_state_star2(actor_get_effect_state(actor))) {
        efst_drop(actor, 0xcb);
        efst_drop(actor, 0xfe);
        efst_drop(actor, 0x46c);
        efst_drop(actor, 0x1a2);
        efst_drop(actor, 0x24a);
        return;
    }
    if (opt3 & OPT3_QUICKEN)
        efst_tinted(actor, 0x82);
    else if (opt3 & OPT3_OVERTHRUST)
        efst_tinted(actor, 0x80);
    else if (opt3 & OPT3_ENERGYCOAT)
        efst_tinted(actor, 0xa9);
    else
        efst_add(actor, opt3, OPT3_BUNSIN, 0x269);
    efst_add(actor, opt3, OPT3_EXPLOSIONSPIRITS, 0xcb);
    if ((opt3 & OPT3_STEELBODY) && !able_to_make_effect_is_effect_exist(actor, 0xfe)) {
        efst_launch(actor, 0xfe);
        efst_launch(actor, 0x46c);
    }
    if ((opt3 & OPT3_WARM) && !able_to_make_effect_is_effect_exist(actor, 0x1a2)) {
        efst_launch(actor, 0x1a2);
        efst_launch(actor, 0x220);
    }
    efst_add(actor, opt3, OPT3_KAITE, 0x1a3);
    efst_add(actor, opt3, OPT3_SOULLINK, 0x1a8);
    efst_add(actor, opt3, OPT3_CONTRACT, 0x24a);
    efst_add(actor, opt3, OPT3_UNDEAD, 0x28f);
    efst_add(actor, opt3, OPT3_AURABLADE, 0x1da);
    efst_add(actor, opt3, OPT3_LIGHTBLADE, 0x17e);
    efst_add(actor, opt3, OPT3_BERSERK, 0x170);
    efst_add(actor, opt3, OPT3_MOONLIT, 0x18a);
    efst_add(actor, opt3, OPT3_MARIONETTE, 0x18c);
    if (opt3 & OPT3_BLADESTOP) {
        if (efst_bladestop_holds(game_actor_get_job(actor)))
            render_object_play_motion_range((struct RenderObject *)actor, 0x60, 4, 4, 0x60, 4, 1, 99999999, 0);
        else
            actor->motion_held = 1;
        actor->argb_locked = 1;
        render_object_set_argb((struct RenderObject *)actor, -1, BLADE_STOP_DIM, BLADE_STOP_DIM, BLADE_STOP_DIM);
    }
    efst_add(actor, opt3, OPT3_ASSUMPTIO, 0x177);
    efst_add(actor, opt3, OPT3_ELEMENTAL_VEIL, 0x2e8);
}

static void efst_switch(struct GameActor *actor, int opt3, int bit, int effect_id, int with, int with_off,
                        int after_off)
{
    char exists = able_to_make_effect_is_effect_exist(actor, effect_id);

    if (opt3 & bit) {
        if (!exists) {
            efst_launch(actor, effect_id);
            if (with)
                efst_launch(actor, with);
        }
        return;
    }
    if (!exists)
        return;
    do {
        efst_drop(actor, effect_id);
        if (with_off)
            efst_drop(actor, with_off);
        if (after_off)
            efst_launch(actor, after_off);
    } while (able_to_make_effect_is_effect_exist(actor, effect_id));
}

static void efst_update_other(struct GameActor *actor, int opt3)
{
    char exists;

    if (is_effect_state_burrow(actor_get_effect_state(actor)) || is_effect_state_star2(actor_get_effect_state(actor))) {
        efst_drop(actor, 0xcb);
        efst_drop(actor, 0xfe);
        efst_drop(actor, 0x46c);
        efst_drop(actor, 0x1a2);
        return;
    }
    if (opt3 & OPT3_BLADESTOP) {
        actor->motion_held = 1;
        actor->argb_locked = 1;
        render_object_set_argb((struct RenderObject *)actor, -1, BLADE_STOP_DIM, BLADE_STOP_DIM, BLADE_STOP_DIM);
    }
    efst_switch(actor, opt3, OPT3_OVERTHRUST, 0x1a4, 0, 0, 0x21c);
    efst_switch(actor, opt3, OPT3_ENERGYCOAT, 0x1ab, 0, 0, 0);
    efst_switch(actor, opt3, OPT3_STEELBODY, 0xfe, 0x46c, 0x46c, 0);
    efst_switch(actor, opt3, OPT3_UNDEAD, 0x28f, 0, 0, 0x290);
    efst_switch(actor, opt3, OPT3_SOULLINK, 0x29b, 0, 0, 0);
    efst_switch(actor, opt3, OPT3_ASSUMPTIO, 0x177, 0, 0, 0);
    exists = able_to_make_effect_is_effect_exist(actor, 0x2e8);
    if (opt3 & OPT3_ELEMENTAL_VEIL) {
        if (!exists)
            efst_launch(actor, 0x2e8);
    } else if (exists == 1) {
        do
            efst_drop(actor, 0x2e8);
        while (able_to_make_effect_is_effect_exist(actor, 0x2e8) == 1);
    }
}

void game_mode_game_mode_show_efst(struct GameMode *mode, int aid, int state,
                                            int level, int show)
{
    struct GameActor *actor = world_get_game_actor_by_aid(mode->world, aid);

    if (!actor)
        return;
    actor->level = (unsigned short)level;
    if (!effect_state_bit6(state))
        game_actor_show_level_aura(actor);
    if (game_actor_is_pc_job(game_actor_get_job(actor)))
        efst_update_player(actor, show);
    else
        efst_update_other(actor, show);
}

#pragma pack(push, 1)
struct PacketDevotionList {
    unsigned short packet_type;
    unsigned aid;
    unsigned targets[5];
    short range;
};
#pragma pack(pop)

enum {
    EF_DEVOTION_LINE = 0xe8,
    EFFECT_MSG_SET_ENDS = 0xe,
    DEVOTION_SLOTS = 5,
};
#define DEVOTION_OVERFLOW_COLOR 0x3232fa

void game_mode_zc_devotion_list(struct GameMode *self, const struct PacketDevotionList *packet)
{
    struct GameActor *actor;
    int i;

    if (g_session1.effectOn == 0)
        return;
    if ((int)packet->aid == g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (actor)
        for (i = 0; i < DEVOTION_SLOTS; i++)
            render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, EF_DEVOTION_LINE, 0, 0, 0);

    for (i = 0; i < DEVOTION_SLOTS; i++) {
        float ends[3];
        Vec3 here = { 0.0f, 0.0f, 0.0f };
        struct RagEffect *line;

        if (packet->targets[i] == 0)
            continue;
        ends[0] = (float)packet->aid;
        ends[1] = (float)packet->targets[i];
        ends[2] = 0.0f;
        if (ends[1] != ends[1] || ends[0] != ends[0]) {
            char text[0x100];

            sprintf(text, "DEVOTION OVERFLOW : %d -- %d , %d , %d , %d , %d", packet->aid, packet->targets[0],
                        packet->targets[1], packet->targets[2], packet->targets[3], packet->targets[4]);
            ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, DEVOTION_OVERFLOW_COLOR, 0, 0);
            strcpy(text, "DEVOTION OVERFLOW : \xea\xb7\xb8\xeb\x9d\xbc\xeb\xb9\x84\xed\x8b\xb0\xec\x97\x90 \xeb\xa9\x94\xec\x9d\xbc\xeb\xa1\x9c " /* "DEVOTION OVERFLOW : 그라비티에 메일로 " */
                              "\xeb\xac\xb8\xec\x9d\x98\xed\x95\xb4\xec\xa3\xbc\xec\x8b\xad\xec\x8b\x9c\xec\x9a\x94" /* "문의해주십시요" */);
            ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, DEVOTION_OVERFLOW_COLOR, 0, 0);
            continue;
        }
        if (!actor)
            continue;
        line = game_actor_launch_effect(actor, EF_DEVOTION_LINE, here, 0.0f);
        if (line)
            rag_effect_send_msg(line, NULL, EFFECT_MSG_SET_ENDS, (int)ends, 0, 0);
    }
}

#pragma pack(push, 1)
struct PacketDispel {
    unsigned short packet_type;
    unsigned aid;
};
#pragma pack(pop)

enum {
    ACTOR_MSG_PROGRESS_STOP = 0x53,
    ACTOR_MSG_CAST_EFFECT_STOP = 0x54,
    ACTOR_MSG_TARGET_EFFECT_STOP = 0x57,
    ACTOR_STATE_CASTING = 8,
};

static void dispel_messages(struct GameActor *actor)
{
    render_object_send(actor, NULL, ACTOR_MSG_CAST_EFFECT_STOP, 0, 0, 0, 0);
    render_object_send(actor, NULL, ACTOR_MSG_TARGET_EFFECT_STOP, 0, 0, 0, 0);
    render_object_send(actor, NULL, ACTOR_MSG_PROGRESS_STOP, 0, 0, 0, 0);
}

static void leave_state_8(struct GameActor *actor)
{
    if (actor->state == ACTOR_STATE_CASTING)
        actor_set_state(actor, 0, 1);
}

void game_mode_zc_dispel(struct GameMode *self, const struct PacketDispel *packet)
{
    struct GameActor *actor;

    if ((int)packet->aid == g_session1.ownAid) {
        actor = (struct GameActor *)self->world->player;
        dispel_messages(actor);
        leave_state_8(actor);
        return;
    }
    actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (!actor)
        return;
    dispel_messages(actor);
    effect_mgr_remove_type(g_effectMgr, packet->aid, EF_WARNING_PLANE);
    leave_state_8(actor);
}
