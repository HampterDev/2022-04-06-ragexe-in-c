#include "game/packet_handlers.h"
#include "game/cells.h"
#include "game/actor_packets.h"
#include "actor/actor_ops.h"
#include "session/abr.h"
#include "game/world_objects.h"
#include "world/object_lookup.h"
#include "actor/render_object.h"
#include "resources/attr_queries.h"
#include "std/integer_map.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <mmsystem.h>
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "game/packets.h"
#include "ui/shops/merchant_shop.h"
#include "platform/language.h"
#include "actor/actor_common.h"
#include "game/packet_data.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "session/session_windows.h"
#include "ui/misc/rolling_cutter.h"
#include "world/world.h"
#include "session/session_skills.h"

#define ACTOR_MSG_DEAD          0x1c
#define ACTOR_MSG_OUT_OF_SIGHT  0x63
#define ACTOR_MSG_STOP          0x13
#define ACTOR_MSG_PLAYER_GONE   0xd
#define SKILL_MSG_SPRITE        0x2f

#define MM_ACTOR_GONE           0xd
#define MM_VENDOR_GONE          0xf5
#define MM_PET_ACT             0x94

#define WND_RESURRECTION        0x9b
#define SERVICE_TYPE_TAIWAN          4

enum { VANISH_OUT_OF_SIGHT = 0, VANISH_DIED = 1, VANISH_LOGGED_OUT = 2, VANISH_TELEPORT = 3 };

static void forget_if_kept(struct GameMode *self, unsigned aid)
{
    if (world_remove_actor_by_aid(self->world, aid) == 1)
        mode_send_msg(self, MM_ACTOR_GONE, aid, 0, 0, 0);
}

static void we_died(struct GameMode *self)
{
    struct GameActor *player = (struct GameActor *)self->world->player;
    static const int closed[] = { 0x30, 0x10, 0x11, 0x19, 0x16, 0x17, 0x18 };
    int i;

    actor_send(player, ACTOR_MSG_PLAYER_GONE, 0, 0);
    actor_send((struct GameActor *)self->world->player, ACTOR_MSG_DEAD, 0, 0);
    self->npc_dialog_open = 1;
    self->player_dead = 1;
    for (i = 0; i < (int)(sizeof closed / sizeof closed[0]); i++)
        ui_window_mgr_post_quit1(&g_windowMgr, closed[i]);
    if (world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.aid))) {
        int hunger = session_pet_hunger_level(g_session1.petFullness);
        int intimacy = session_pet_intimacy_level(g_session1.petIntimacy);
        int emotion = session_get_pet_emotion(&g_session1, hunger, intimacy, 3);

        mode_send_msg(self, MM_PET_ACT, emotion, 0, 0, 0);
    }
    if (g_serviceType == SERVICE_TYPE_TAIWAN) {
        message_box(0x2b7);
    } else if (!ui_window_mgr_post_quit1(&g_windowMgr, WND_RESURRECTION)) {
        ui_window_open(WND_RESURRECTION);
    }
    session_notify_messenger_group_and_plates(&g_session1);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_VANISH {
    unsigned short id;
    unsigned aid;
    unsigned char type;
};
struct PACKET_ZC_SPRITE_CHANGE {
    unsigned short id;
    unsigned aid;
    unsigned char type;
    unsigned char value;
};
struct PACKET_ZC_FASTMOVE {
    unsigned short id;
    unsigned aid;
    short x;
    short y;
};
struct PACKET_ZC_BLADESTOP {
    unsigned short id;
    unsigned source_aid;
    unsigned target_aid;
    int on;
};
struct PACKET_ZC_PAR_CHANGE_USER {
    unsigned short id;
    unsigned aid;
    short type;
    int value;
};
struct PACKET_ZC_NOTIFY_MOVE {
    unsigned short id;
    unsigned aid;
    unsigned char move_data[6];
    DWORD start_tick;
};
struct PACKET_ZC_NOTIFY_PLAYERMOVE {
    unsigned short id;
    DWORD start_tick;
    unsigned char move_data[6];
};
struct PACKET_ZC_CHANGE_DIRECTION {
    unsigned short id;
    unsigned aid;
    short head_direction;
    unsigned char body_direction;
};
struct PACKET_ZC_HIGHJUMP {
    unsigned short id;
    unsigned aid;
    short x;
    short y;
};
struct PACKET_ZC_EMOTION {
    unsigned short id;
    unsigned aid;
    unsigned char type;
};
struct PACKET_ZC_NOTIFY_FONT {
    unsigned short id;
    unsigned aid;
    unsigned short font;
};
struct PACKET_ZC_PROGRESS_ACTOR {
    unsigned short id;
    unsigned aid;
    unsigned color;
    int seconds;
};
struct PACKET_ZC_MVP {
    unsigned short id;
    unsigned aid;
};
struct PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE {
    unsigned short id;
    unsigned target_aid;
    short target_x;
    short target_y;
    short own_x;
    short own_y;
    short range;
};
struct PACKET_ZC_ATTACK_RANGE {
    unsigned short id;
    short range;
};
struct PACKET_ZC_EQUIP_ARROW {
    unsigned short id;
    short index;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_VANISH, 7);
SIZE_ASSERT(PACKET_ZC_SPRITE_CHANGE, 8);
SIZE_ASSERT(PACKET_ZC_FASTMOVE, 10);
SIZE_ASSERT(PACKET_ZC_BLADESTOP, 14);
SIZE_ASSERT(PACKET_ZC_PAR_CHANGE_USER, 12);
SIZE_ASSERT(PACKET_ZC_NOTIFY_MOVE, 16);
SIZE_ASSERT(PACKET_ZC_NOTIFY_PLAYERMOVE, 12);
SIZE_ASSERT(PACKET_ZC_CHANGE_DIRECTION, 9);
SIZE_ASSERT(PACKET_ZC_HIGHJUMP, 10);
SIZE_ASSERT(PACKET_ZC_EMOTION, 7);
SIZE_ASSERT(PACKET_ZC_NOTIFY_FONT, 8);
SIZE_ASSERT(PACKET_ZC_PROGRESS_ACTOR, 14);
SIZE_ASSERT(PACKET_ZC_MVP, 6);
SIZE_ASSERT(PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE, 16);
SIZE_ASSERT(PACKET_ZC_ATTACK_RANGE, 4);
SIZE_ASSERT(PACKET_ZC_EQUIP_ARROW, 4);

void game_mode_zc_notify_vanish(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_VANISH *packet = (const struct PACKET_ZC_NOTIFY_VANISH *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    name_card_holder_erase(&self->actor_names, aid);
    session_erase_abr(&g_session1, aid);
    switch (packet->type) {
    case VANISH_OUT_OF_SIGHT:
        actor = world_get_game_actor_by_aid(self->world, aid);
        if (actor) {
            actor_send(actor, ACTOR_MSG_OUT_OF_SIGHT, 0, 0);
            mode_send_msg(self, MM_ACTOR_GONE, aid, 0, 0, 0);
        }
        break;
    case VANISH_DIED:
        actor = world_get_game_actor_by_aid(self->world, aid);
        if (actor) {
            struct Player *player = self->world->player;
            int job;

            if (aid == player->target_aid)
                player->target_aid = 0;
            actor_send(actor, ACTOR_MSG_DEAD, 0, 0);
            job = game_actor_get_job(actor);
            if (job_is_monster(job) || actor->object_type == 9 || session_is_homun_or_merc_job(&g_session1, job) ||
                game_actor_get_job(actor) == 0x235)
                mode_send_msg(self, MM_ACTOR_GONE, aid, 0, 0, 0);
        }
        if (aid == ((unsigned)g_session1.ownAid))
            we_died(self);
        return;
    case VANISH_LOGGED_OUT:
        actor = world_get_game_actor_by_aid(self->world, aid);
        if (actor)
            game_mode_launch_dummy_effect(self, &actor->pos, 0x13a, 0x258, 0);
        if (g_windowMgr.merchant_item_shop_wnd && ui_merchant_shop_seller(g_windowMgr.merchant_item_shop_wnd) == aid) {
            mode_send_msg(self, MM_VENDOR_GONE, 0, 0, 0, 0);
            session_clear_purchase_lists(&g_session1);
        }
        forget_if_kept(self, aid);
        return;
    case VANISH_TELEPORT:
        actor = world_get_game_actor_by_aid(self->world, aid);
        if (actor) {
            int state = actor_get_effect_state(actor);

            if (!effect_state_bit6(state))
                game_mode_launch_dummy_effect(self, &actor->pos, 0x130, 0x12c, 0);
        }
        break;
    default:
        world_get_game_actor_by_aid(self->world, aid);
        forget_if_kept(self, aid);
        return;
    }
    forget_if_kept(self, aid);
    if (g_windowMgr.messenger_group_wnd)
        wnd_send(g_windowMgr.messenger_group_wnd, 6, 0x22, aid, 0, 0);
}

enum {
    LOOK_BASE = 0, LOOK_HAIR = 1, LOOK_WEAPON = 2, LOOK_HEAD_BOTTOM = 3, LOOK_HEAD_TOP = 4,
    LOOK_HEAD_MID = 5, LOOK_HAIR_COLOR = 6, LOOK_CLOTHES_COLOR = 7, LOOK_SHIELD = 8,
};

void game_mode_zc_sprite_change(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SPRITE_CHANGE *packet = (const struct PACKET_ZC_SPRITE_CHANGE *)pkt;
    unsigned aid = packet->aid;
    int value = packet->value;
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid)) {
        actor = (struct GameActor *)self->world->player;
        switch (packet->type) {
        case LOOK_BASE:
            g_session1.clothesColor = g_session1.clothesColor < 2 ? 0 : g_session1.clothesColor;
            session_set_job(&g_session1, value);
            break;
        case LOOK_HAIR: g_session1.hairStyle = value; break;
        case LOOK_WEAPON: g_session1.weapon = value; break;
        case LOOK_HEAD_BOTTOM: g_session1.headBottom = value; break;
        case LOOK_HEAD_TOP: g_session1.headTop = value; break;
        case LOOK_HEAD_MID: g_session1.headMid = value; break;
        case LOOK_HAIR_COLOR: g_session1.hairColor = value; break;
        case LOOK_CLOTHES_COLOR: g_session1.clothesColor = value; break;
        case LOOK_SHIELD: g_session1.shield = value; break;
        }
        if (!actor)
            return;
    } else {
        actor = world_get_game_actor_by_aid(self->world, aid);
        if (!actor) {
            struct GameActor *skill = (struct GameActor *)world_get_skill_by_aid(self->world, packet->aid);

            if (skill && packet->type == LOOK_BASE)
                actor_send(skill, SKILL_MSG_SPRITE, value, 0);
            return;
        }
    }
    switch (packet->type) {
    case LOOK_BASE:
        game_actor_set_job(actor, value);
        actor_set_imf_name(actor);
        actor_set_head_pal_name(actor);
        actor_set_body_pal_name(actor);
        break;
    case LOOK_HAIR:
        actor_set_head(actor, value);
        actor_set_imf_name(actor);
        actor_set_head_pal_name(actor);
        actor_refresh_accessory3(actor);
        break;
    case LOOK_WEAPON:
        actor_set_weapon(actor, value);
        break;
    case LOOK_HEAD_BOTTOM: actor_set_accessory(actor, value); break;
    case LOOK_HEAD_TOP: actor_set_accessory2(actor, value); break;
    case LOOK_HEAD_MID: actor_set_accessory3(actor, value); break;
    case LOOK_HAIR_COLOR: actor_set_head_palette(actor, value); break;
    case LOOK_CLOTHES_COLOR: actor_set_body_palette(actor, value); break;
    case LOOK_SHIELD: actor_set_shield(actor, value); break;
    }
}

enum {
    LOOK_SHOES = 9, LOOK_FLOOR = 10, LOOK_RESET_COSTUMES = 11, LOOK_ROBE = 12, LOOK_BODY2 = 13,
};
#pragma pack(push, 1)
struct PACKET_ZC_SPRITE_CHANGE2 {
    unsigned short id;
    unsigned aid;
    unsigned char type;
    long long value;
};
#pragma pack(pop)

#define JOB_DOUBLE_WEAPON(job) ((job) == 12 || (job) == 4013 || (job) == 4035 || (job) == 4059 || \
                                (job) == 4065 || (job) == 4101)
#define JOB_CLOTHES_COLORED(v) ((unsigned long long)((v) - 4001) <= 6)

static void keep_own_look(const struct PACKET_ZC_SPRITE_CHANGE2 *packet)
{
    int value = (int)packet->value;

    switch (packet->type) {
    case LOOK_BASE:
        if (g_session1.clothesColor < 2) {
            if ((unsigned long long)(packet->value - 4001) <= 21) {
                g_session1.clothesColor = 0;
                if (JOB_CLOTHES_COLORED(packet->value))
                    g_session1.clothesColor = 1;
            } else {
                g_session1.clothesColor = 0;
            }
        }
        if ((long long)g_session1.job != packet->value)
            session_set_job(&g_session1, value);
        break;
    case LOOK_HAIR: g_session1.hairStyle = value; break;
    case LOOK_WEAPON:
        g_session1.weapon = packet->value;
        if (session_get_weapon_view_id((int)(packet->value >> 32)) == -1 && value != 0)
            g_session1.shield = (int)(packet->value >> 32);
        break;
    case LOOK_HEAD_BOTTOM: g_session1.headBottom = value; break;
    case LOOK_HEAD_TOP: g_session1.headTop = value; break;
    case LOOK_HEAD_MID: g_session1.headMid = value; break;
    case LOOK_HAIR_COLOR: g_session1.hairColor = value; break;
    case LOOK_CLOTHES_COLOR: g_session1.clothesColor = value; break;
    case LOOK_RESET_COSTUMES:
        g_session1.headBottom = 0;
        g_session1.headTop = 0;
        g_session1.headMid = 0;
        g_session1.robe = 0;
        break;
    case LOOK_ROBE: g_session1.robe = value; break;
    case LOOK_BODY2: g_session1.body = value; break;
    }
}

void game_mode_apply_sprite_change2(struct GameMode *self, const struct PACKET_ZC_SPRITE_CHANGE2 *packet)
{
    struct GameActor *actor;
    int value = (int)packet->value, second = (int)(packet->value >> 32);
    int shield, job;

    if (packet->aid == ((unsigned)g_session1.ownAid)) {
        actor = (struct GameActor *)self->world->player;
        keep_own_look(packet);
        if (!actor)
            return;
    } else {
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
        if (!actor) {
            struct GameActor *skill = (struct GameActor *)world_get_skill_by_aid(self->world, packet->aid);

            if (skill && packet->type == LOOK_BASE)
                actor_send(skill, SKILL_MSG_SPRITE, packet->value, 0);
            return;
        }
    }
    switch (packet->type) {
    case LOOK_BASE:
        game_actor_set_job(actor, value);
        if (packet->aid == ((unsigned)g_session1.ownAid) && JOB_CLOTHES_COLORED(packet->value))
            actor_set_body_palette(actor, g_session1.clothesColor);
        actor_set_imf_name(actor);
        actor_set_head_pal_name(actor);
        actor_set_body_pal_name(actor);
        break;
    case LOOK_HAIR:
        actor_set_head(actor, value);
        actor_set_imf_name(actor);
        actor_set_head_pal_name(actor);
        actor_refresh_accessory3(actor);
        break;
    case LOOK_WEAPON:
        shield = session_get_weapon_view_id(second);
        job = game_actor_get_job(actor);
        if (JOB_DOUBLE_WEAPON(job))
            actor_set_weapon_look(actor, packet->value);
        else
            actor_set_weapon_look(actor, value);
        if (shield == -1 || second == 0)
            actor_set_shield(actor, second);
        break;
    case LOOK_HEAD_BOTTOM: actor_set_accessory(actor, value); break;
    case LOOK_HEAD_TOP: actor_set_accessory2(actor, value); break;
    case LOOK_HEAD_MID: actor_set_accessory3(actor, value); break;
    case LOOK_HAIR_COLOR: actor_set_head_palette(actor, value); break;
    case LOOK_CLOTHES_COLOR: actor_set_body_palette(actor, value); break;
    case LOOK_SHOES: actor_set_appearance_value(actor, value); break;
    case LOOK_RESET_COSTUMES:
        actor_set_accessory(actor, 0);
        actor_set_accessory2(actor, 0);
        actor_set_accessory3(actor, 0);
        actor_set_robe(actor, 0, 0);
        break;
    case LOOK_ROBE: actor_set_robe(actor, value, 0); break;
    case LOOK_BODY2: actor_set_body_style(actor, value, 1); break;
    }
}

void game_mode_zc_stopmove(struct GameMode *self, unsigned aid, short x, short y, unsigned char dir)
{
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid))
        actor = (struct GameActor *)self->world->player;
    else if (!(actor = world_get_game_actor_by_aid(self->world, aid)))
        return;
    render_object_send(actor, NULL, ACTOR_MSG_STOP, x, y, 0, dir);
}

void game_mode_zc_sprite_change2(struct GameMode *self, const struct PACKET_ZC_SPRITE_CHANGE2 *pkt)
{
    struct PACKET_ZC_SPRITE_CHANGE2 copy;
    const unsigned *aids;
    unsigned count, i;

    game_mode_apply_sprite_change2(self, pkt);
    if (pkt->aid != ((unsigned)g_session1.ownAid))
        return;
    aids = (const unsigned *)self->own_double_aids.first;
    count = aids > (const unsigned *)self->own_double_aids.last ? 0
          : ((const char *)self->own_double_aids.last - (const char *)aids + 3) / 4;
    for (i = 0; i < count; i++) {
        copy.id = 0x1d7;
        copy.aid = aids[i];
        copy.type = pkt->type;
        copy.value = pkt->value;
        game_mode_apply_sprite_change2(self, &copy);
    }
}

#define ACTOR_MSG_FAST_MOVE     0x90

void game_mode_zc_fast_move(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_FASTMOVE *packet = (const struct PACKET_ZC_FASTMOVE *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid))
        actor = self->world ? (struct GameActor *)self->world->player : NULL;
    else
        actor = session_get_actor_by_aid(&g_session1, aid);
    if (actor)
        actor_send(actor, ACTOR_MSG_FAST_MOVE, packet->x, packet->y);
}

#define ACTOR_VALUE_BLADE_STOP  3
#define BLADE_STOP_DIM          0x40

static struct GameActor *actor_by_aid(struct GameMode *self, unsigned aid)
{
    if (aid == ((unsigned)g_session1.ownAid))
        return (struct GameActor *)self->world->player;
    return world_get_game_actor_by_aid(self->world, aid);
}

void game_mode_zc_blade_stop(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BLADESTOP *packet = (const struct PACKET_ZC_BLADESTOP *)pkt;
    struct GameActor *src, *dst;

    if (packet->on != 0) {
        src = actor_by_aid(self, packet->source_aid);
        dst = actor_by_aid(self, packet->target_aid);
        if (src == 0 || dst == 0)
            return;
        render_object_play_motion_range((struct RenderObject *)src, 0x60, 4, 4, 0x60, 4, 1, 99999999, 0);
        src->argb_locked = 1;
        render_object_set_argb((struct RenderObject *)src, -1, BLADE_STOP_DIM, BLADE_STOP_DIM, BLADE_STOP_DIM);
        game_actor_set_counter(src, ACTOR_VALUE_BLADE_STOP, 1);
        game_actor_set_counter(dst, ACTOR_VALUE_BLADE_STOP, 1);
        dst->motion_held = 1;
        dst->argb_locked = 1;
        render_object_set_argb((struct RenderObject *)dst, -1, BLADE_STOP_DIM, BLADE_STOP_DIM, BLADE_STOP_DIM);
    } else {
        src = actor_by_aid(self, packet->source_aid);
        dst = actor_by_aid(self, packet->target_aid);
        if (src != 0) {
            render_object_play_motion_range((struct RenderObject *)src, 0x60, 4, 4, 0x60, 4, 1, 1, 0);
            src->argb_locked = 0;
            render_object_set_argb((struct RenderObject *)src, -1, 0xff, 0xff, 0xff);
            game_actor_set_counter(src, ACTOR_VALUE_BLADE_STOP, 0);
        }
        if (dst != 0) {
            dst->motion_held = 0;
            dst->argb_locked = 0;
            render_object_set_argb((struct RenderObject *)dst, -1, 0xff, 0xff, 0xff);
            game_actor_set_counter(dst, ACTOR_VALUE_BLADE_STOP, 0);
        }
    }
}

void game_mode_zc_notify_actor_init(struct GameMode *self, const unsigned char *pkt)
{
    int key = 5;
    struct IntInt64Pair *var;

    (void)self;
    (void)pkt;
    var = tree_get(&g_session1.instantVars, &key, sizeof key, sizeof *var, tree_less_int);
    var->value = 0;
    connection_clear_held(rag_connection_transport(rag_connection_instance()));
}

void game_mode_zc_par_change_user(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PAR_CHANGE_USER *packet = (const struct PACKET_ZC_PAR_CHANGE_USER *)pkt;
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor && packet->type == 4)
        actor_set_honor(actor, packet->value);
}

#define ACTOR_MSG_MOVE          0
#define ACTOR_MSG_TURN          0x64

void game_mode_zc_notify_move(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_MOVE *packet = (const struct PACKET_ZC_NOTIFY_MOVE *)pkt;
    struct ActorMove move;
    struct GameActor *actor;

    move.start_tick = packet->start_tick;
    packet_decode_move_data(packet->move_data, &move.src_x, &move.src_y, &move.dst_x, &move.dst_y, &move.sub_x, &move.sub_y);
    actor = world_get_game_actor_by_aid(self->world, packet->aid);
    session_sync_server_time(&g_session1, move.start_tick);
    if (actor)
        actor_send(actor, ACTOR_MSG_MOVE, (int)&move, 0);
}

void game_mode_zc_notify_player_move(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_PLAYERMOVE *packet = (const struct PACKET_ZC_NOTIFY_PLAYERMOVE *)pkt;
    struct ActorMove move;

    move.start_tick = packet->start_tick;
    packet_decode_move_data(packet->move_data, &move.src_x, &move.src_y, &move.dst_x, &move.dst_y, &move.sub_x, &move.sub_y);
    session_sync_server_time(&g_session1, move.start_tick);
    actor_send((struct GameActor *)self->world->player, ACTOR_MSG_MOVE, (int)&move, 0);
}

void game_mode_zc_change_direction(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_CHANGE_DIRECTION *packet = (const struct PACKET_ZC_CHANGE_DIRECTION *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid)) {
        self->world->player->roty = (float)((packet->body_direction + 4) * 45);
        actor_send((struct GameActor *)self->world->player, ACTOR_MSG_TURN, packet->head_direction, 0);
        return;
    }
    actor = world_get_game_actor_by_aid(self->world, aid);
    if (!actor)
        return;
    actor->roty = (float)((packet->body_direction + 4) * 45);
    actor_send(actor, ACTOR_MSG_TURN, packet->head_direction, 0);
}

void game_mode_set_player_names(struct GameMode *self, unsigned aid, const char *name, const char *party,
                                        const char *guild, const char *position, int title_id)
{
    struct String title, a_npc_title, a_title, a_position, a_guild, a_party, a_name;

    g_session1.ownTitleId = title_id;
    session_get_title_string(&g_session1, &title, title_id);
    string_init(&a_npc_title);
    string_copy(&a_title, &title);
    string_init(&a_position);
    string_assign_cstr(&a_position, position);
    string_init(&a_guild);
    string_assign_cstr(&a_guild, guild);
    string_init(&a_party);
    string_assign_cstr(&a_party, party);
    string_init(&a_name);
    string_assign_cstr(&a_name, name);
    name_card_holder_set_all(&self->actor_names, aid, a_name, a_party, a_guild, a_position, a_title, a_npc_title, 0);
    string_free(&title);
}

#pragma pack(push, 1)
struct PacketAckReqNameAll2 {
    unsigned short packet_type;
    unsigned aid;
    char name[24];
    char party_name[24];
    char guild_name[24];
    char position_name[24];
    int title_id;
};
#pragma pack(pop)

void game_mode_on_ack_req_name_all2(struct GameMode *self, const struct PacketAckReqNameAll2 *packet)
{
    char name[25] = { 0 };

    memcpy(name, packet->name, 24);
    name[24] = 0;
    game_mode_set_player_names(self, packet->aid, name, packet->party_name, packet->guild_name, packet->position_name,
                            packet->title_id);
}

#pragma pack(push, 1)
struct PACKET_ZC_COMBODELAY {
    unsigned short id;
    unsigned aid;
    int delay;
};
#pragma pack(pop)

void game_mode_zc_combo_delay(struct GameMode *self, const struct PACKET_ZC_COMBODELAY *packet)
{
    session_set_combo_delay(&g_session1, packet->aid, packet->delay);
    if (packet->aid == ((unsigned)g_session1.ownAid)) {
        render_object_set_force_anim_end_tick((struct RenderObject *)self->world->player, packet->delay);
    } else {
        struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

        if (actor)
            render_object_set_force_anim_end_tick((struct RenderObject *)actor, packet->delay);
    }
}

#define ACTOR_MSG_EMOTION       0x15

void game_mode_zc_high_jump(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_HIGHJUMP *packet = (const struct PACKET_ZC_HIGHJUMP *)pkt;
    float x = 0, y = 0, z = 0;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    game_mode_cell_to_world(game_current_mode(), packet->x,
                         packet->y, &x, &z);
    y = c3d_attr_get_height(self->world->attr, x, z);
    if (aid == ((unsigned)g_session1.ownAid))
        actor = (struct GameActor *)self->world->player;
    else if ((actor = world_get_game_actor_by_aid(self->world, aid)) == 0)
        return;
    actor->slide_x = x;
    actor->slide_z = z;
    actor->pos.x = x;
    actor->pos.y = y;
    actor->pos.z = z;
}

void game_mode_zc_emotion(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_EMOTION *packet = (const struct PACKET_ZC_EMOTION *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid))
        actor = (struct GameActor *)self->world->player;
    else if ((actor = world_get_game_actor_by_aid(self->world, aid)) == 0)
        return;
    actor_send(actor, ACTOR_MSG_EMOTION, packet->type, 0);
}

void game_mode_zc_notify_font(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_FONT *packet = (const struct PACKET_ZC_NOTIFY_FONT *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid)) {
        g_session1.font = (unsigned char)packet->font;
        return;
    }
    actor = world_get_game_actor_by_aid(self->world, aid);
    if (actor)
        actor->balloon_font = packet->font;
}

#define ACTOR_MSG_PROGRESS      0x52
#define ACTOR_MSG_PROGRESS_STOP 0x53

void game_mode_zc_progress(struct GameMode *self, const unsigned char *pkt)
{
    const struct PacketActorProgress *packet = (const struct PacketActorProgress *)pkt;
    struct GameActor *player = (struct GameActor *)self->world->player;

    if (!player)
        return;
    actor_send(player, ACTOR_MSG_PROGRESS, (unsigned)(packet->seconds * 1000), 0);
    g_session1.progressBarPending = 1;
}

void game_mode_zc_progress_actor(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PROGRESS_ACTOR *packet = (const struct PACKET_ZC_PROGRESS_ACTOR *)pkt;
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor)
        actor_send(actor, ACTOR_MSG_PROGRESS, (unsigned)(packet->seconds * 1000), 0);
}

void game_mode_zc_progress_cancel(struct GameMode *self, const unsigned char *pkt)
{
    struct GameActor *player = (struct GameActor *)self->world->player;

    (void)pkt;
    if (!player)
        return;
    actor_send(player, ACTOR_MSG_PROGRESS_STOP, 0, 0);
    if (player->state == 8)
        actor_set_state(player, 0, 1);
}

#define ACTOR_MSG_MVP           0x24

void game_mode_zc_mvp(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_MVP *packet = (const struct PACKET_ZC_MVP *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid))
        actor = (struct GameActor *)self->world->player;
    else if ((actor = world_get_game_actor_by_aid(self->world, aid)) == 0)
        return;
    actor_send(actor, ACTOR_MSG_MVP, 0, 0);
}

#define ACTOR_MSG_POSITION      0x3d
#define ACTOR_MSG_ATTACK_TARGET        0x3c

void game_mode_zc_attack_failure_for_distance(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE *packet = (const struct PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE *)pkt;
    struct GameActor *target;

    g_session1.attackRange = packet->range;
    target = world_get_game_actor_by_aid(self->world, packet->target_aid);
    if (target)
        actor_send(target, ACTOR_MSG_POSITION, packet->target_x, packet->target_y);
    actor_send((struct GameActor *)self->world->player, ACTOR_MSG_POSITION, packet->own_x,
               packet->own_y);
    actor_send((struct GameActor *)self->world->player, ACTOR_MSG_ATTACK_TARGET, packet->target_aid, 0);
}

void game_mode_zc_attack_range(const unsigned char *pkt)
{
    const struct PACKET_ZC_ATTACK_RANGE *packet = (const struct PACKET_ZC_ATTACK_RANGE *)pkt;
    g_session1.attackRange = packet->range;
}

void game_mode_zc_equip_arrow(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_EQUIP_ARROW *packet = (const struct PACKET_ZC_EQUIP_ARROW *)pkt;
    session_set_arrow(&g_session1, packet->index);
    self->wear_ack_time = timeGetTime() - 2000;
}
