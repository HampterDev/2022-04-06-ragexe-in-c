#include "game/packet_handlers.h"
#include "actor/actor_ops.h"
#include "game/actor_packets.h"
#include "contents/sound.h"
#include "session/state.h"
#include "platform/win_types.h"
#include <string.h>
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "game/own_states.h"

#include "game/skill_cooldowns.h"
#include "game/rodex_mail.h"
#include "game/state_change_helpers.h"
#include "session/session_items.h"
#include "session/session_party.h"

#pragma pack(push, 1)
struct PacketMsgStateChange {
    unsigned short packet_type;
    short index;
    int aid;
    unsigned char state;
};
#pragma pack(pop)

static void own_states_add(struct GameMode *mode, int index)
{
    struct OwnStateEntry entry = {0};

    entry.index = index;
    entry.end_tick = 999999;
    if (!game_mode_is_exception_state(index) && own_states_count() != 0)
        own_states_remove(index, index);
    own_states_push(&entry);
}

static struct Player *own_player(struct GameMode *mode)
{
    return mode->world->player;
}

static int is_own(const struct PacketMsgStateChange *packet)
{
    return packet->aid == g_session1.ownAid;
}

static struct GameActor *state_actor(struct GameMode *mode, const struct PacketMsgStateChange *packet)
{
    if (is_own(packet))
        return (struct GameActor *)own_player(mode);
    return world_get_game_actor_by_aid(mode->world, packet->aid);
}

static int end_effect(const struct PacketMsgStateChange *packet, int effect_id)
{
    struct Effect *effect = effect_mgr_find_effect(g_effectMgr, packet->aid, effect_id);
    if (effect == 0)
        return 0;
    effect_mgr_remove(g_effectMgr, effect, 1);
    return 1;
}

static int begins_without_line(int index)
{
    switch (index) {
    case 0x00: case 0x01: case 0x03: case 0x04: case 0x05: case 0x0b: case 0x1b: case 0x1c:
    case 0x22: case 0x36: case 0x37: case 0x38: case 0x39: case 0x3d: case 0x56: case 0x57:
    case 0x5f: case 0x61: case 0x62: case 0x67: case 0x68: case 0x69: case 0x6b: case 0x6e:
    case 0x82: case 0x84: case 0x85: case 0x86: case 0x87: case 0x89: case 0x8b: case 0x8d:
    case 0x8f: case 0x91: case 0x95: case 0x98: case 0x99: case 0x9c: case 0x9d: case 0x9e:
    case 0x9f: case 0xa0: case 0xa5: case 0xa6: case 0xa7: case 0xa9: case 0xaa: case 0xab:
    case 0xb5: case 0xb8: case 0xba: case 0xbf: case 0xee: case 0xef: case 0xf0: case 0xfa:
    case 0x11a: case 0x11e: case 0x12a: case 0x139: case 0x13a: case 0x13c: case 0x148: case 0x14a:
    case 0x14d: case 0x14e: case 0x150: case 0x151: case 0x153: case 0x165: case 0x175: case 0x176:
    case 0x177: case 0x179: case 0x17b: case 0x189: case 0x18a: case 0x18c: case 0x18d: case 0x18e:
    case 0x18f: case 0x1a5: case 0x1a6: case 0x1a7: case 0x1a9: case 0x1aa: case 0x1ab: case 0x1ad:
    case 0x1ae: case 0x1cd: case 0x1cf: case 0x1d0: case 0x1d8: case 0x1d9: case 0x1dc: case 0x23b:
    case 0x241: case 0x244: case 0x245: case 0x248: case 0x25d: case 0x25e: case 0x25f: case 0x260:
    case 0x263: case 0x288: case 0x289: case 0x29e: case 0x365: case 0x420: case 0x421: case 0x422:
    case 0x423: case 0x461:
        return 1;
    }
    return (index >= 0x70 && index <= 0x76) || (index >= 0x78 && index <= 0x7d)
        || (index >= 0xc5 && index <= 0xc9) || (index >= 0xcb && index <= 0xec)
        || (index >= 0xfe && index <= 0x103) || (index >= 0x105 && index <= 0x10e)
        || (index >= 0x121 && index <= 0x128) || (index >= 0x12c && index <= 0x12e)
        || (index >= 0x13e && index <= 0x143) || (index >= 0x155 && index <= 0x163)
        || (index >= 0x168 && index <= 0x16f) || (index >= 0x17d && index <= 0x186)
        || (index >= 0x191 && index <= 0x195) || (index >= 0x197 && index <= 0x1a3)
        || (index >= 0x1b1 && index <= 0x1c0) || (index >= 0x1c2 && index <= 0x1c6)
        || (index >= 0x1eb && index <= 0x1f0);
}

static int state_began(struct GameMode *mode, const struct PacketMsgStateChange *packet, char *text, int *color)
{
    *color = STATE_COLOR_BEGIN;
    switch (packet->index) {
    case 0x02: case 0x25: case 0x26: case 0x27: case 0x28: case 0x44: case 0xa1:
        say(text, 0x1cf); break;
    case 0x06: say(text, 0x1d5); break;
    case 0x07: say(text, 0x26c); break;
    case 0x08: say(text, 0x26e); break;
    case 0x09: say(text, 0xd6b); break;
    case 0x0a: say(text, 0xd6a); break;
    case 0x0c: case 0x29: case 0x2a:
        say(text, 0xd1); break;
    case 0x0d: say(text, 0xd2); break;
    case 0x0e: say(text, 0x276); break;
    case 0x0f: say(text, 0xd87); break;
    case 0x10: say(text, 0x1d3); break;
    case 0x11: case 0x40: say(text, 0x1d6); break;
    case 0x12: say(text, 0x1d8); break;
    case 0x13: say(text, 0x1da); break;
    case 0x14: say(text, 0x278); break;
    case 0x15: say(text, 0x27a); break;
    case 0x16: say(text, 0x27c); break;
    case 0x17: case 0x93: say(text, 0x27e); break;
    case 0x18: say(text, 0x1dc); break;
    case 0x19: case 0xbc: say(text, 0x1d1); break;
    case 0x1a: say(text, 0x1e0); break;
    case 0x1d:
        no_line(text);
        if (is_own(packet)) {
            render_object_set_action((struct RenderObject *)own_player(mode), 0x40, 1);
            own_player(mode)->motion_held = 1;
        }
        break;
    case 0x1e: say(text, 0xd69); break;
    case 0x1f: say(text, 0x288); break;
    case 0x20: say(text, 0x28a); break;
    case 0x21: say(text, 0x28c); break;
    case 0xb6:
        if (is_own(packet))
            sound_play_wave("lava_golem_move.wav", 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
        no_line(text);
        break;
    case 0xf1: case 0x10f: say(text, 0x4cf); break;
    case 0xf2: case 0x110: say(text, 0x4d1); break;
    case 0xf3: case 0x111: say(text, 0x4d3); break;
    case 0xf4: case 0x112: say(text, 0x4d7); break;
    case 0xf5: case 0x113: say(text, 0x4d5); break;
    case 0xf6: case 0x114: say(text, 0x4d9); break;
    case 0xf7: say(text, 0x4db); break;
    case 0xf8: say(text, 0x4dd); break;
    case 0xf9: say(text, 0x4df); break;
    case 0xfb: say(text, 0x4e2); break;
    case 0xfc: say(text, 0x4e3); break;
    case 0xfd:
        if (is_own(packet))
            mode->boss_shown = 1;
        say(text, 0x4e4);
        break;
    case 0x104:
        if (is_own(packet))
            g_session1.ownIllusion = 1;
        no_line(text);
        break;
    case 0x138: say(text, 0x567); break;
    case 0x37d:
        if (is_own(packet)) {
            world_get_game_actor_by_aid(mode->world, packet->aid);
            actor_set_state(own_player(mode), 0x2f, 1);
        }
        break;
    case 0x380: case 0x395: case 0x397: case 0x3a9: case 0x3d6:
        break;
    case 0x3a5:
        if (is_own(packet)) {
            actor_set_state(own_player(mode), 0x30, 1);
            own_player(mode)->motion_held = 1;
        }
        break;
    case 0x3f5:
        *color = STATE_COLOR_END;
        say(text, 0xcaf);
        break;
    default:
        if (!begins_without_line(packet->index))
            return 0;
        no_line(text);
        break;
    }
    return 1;
}

static int ends_without_line(int index)
{
    switch (index) {
    case 0x00: case 0x01: case 0x03: case 0x04: case 0x05: case 0x0b: case 0x1b: case 0x1c:
    case 0x20: case 0x21: case 0x22: case 0x36: case 0x37: case 0x38: case 0x39: case 0x56:
    case 0x57: case 0x5f: case 0x61: case 0x62: case 0x67: case 0x68: case 0x69: case 0x6b:
    case 0x6e: case 0x82: case 0x84: case 0x85: case 0x86: case 0x87: case 0x89: case 0x8b:
    case 0x8d: case 0x8f: case 0x91: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99:
    case 0x9c: case 0x9d: case 0x9e: case 0x9f: case 0xa0: case 0xa5: case 0xa6: case 0xa7:
    case 0xa9: case 0xaa: case 0xab: case 0xb5: case 0xb6: case 0xb8: case 0xba: case 0xbf:
    case 0xee: case 0xef: case 0xf0: case 0xfa: case 0xfb: case 0xfc: case 0x11a: case 0x11e:
    case 0x138: case 0x139: case 0x13a: case 0x13c: case 0x148: case 0x14a: case 0x14d: case 0x14e:
    case 0x150: case 0x151: case 0x165: case 0x175: case 0x176: case 0x177: case 0x179: case 0x17b:
    case 0x189: case 0x18a: case 0x18c: case 0x18d: case 0x18e: case 0x18f: case 0x1a5: case 0x1a6:
    case 0x1a7: case 0x1a9: case 0x1aa: case 0x1ab: case 0x1ad: case 0x1ae: case 0x1cd: case 0x1cf:
    case 0x1d0: case 0x1d8: case 0x1d9: case 0x1dc: case 0x23b: case 0x241: case 0x244: case 0x245:
    case 0x248: case 0x25d: case 0x25e: case 0x25f: case 0x260: case 0x263: case 0x265: case 0x26a:
    case 0x27c: case 0x288: case 0x289: case 0x29e: case 0x365: case 0x420: case 0x421: case 0x422:
    case 0x423: case 0x461:
        return 1;
    }
    return (index >= 0x70 && index <= 0x76) || (index >= 0x78 && index <= 0x7d)
        || (index >= 0xc5 && index <= 0xc9) || (index >= 0xcb && index <= 0xec)
        || (index >= 0xfe && index <= 0x103) || (index >= 0x105 && index <= 0x10e)
        || (index >= 0x121 && index <= 0x12e) || (index >= 0x13e && index <= 0x143)
        || (index >= 0x155 && index <= 0x163) || (index >= 0x168 && index <= 0x16f)
        || (index >= 0x17d && index <= 0x186) || (index >= 0x191 && index <= 0x195)
        || (index >= 0x197 && index <= 0x1a3) || (index >= 0x1b4 && index <= 0x1c0)
        || (index >= 0x1c2 && index <= 0x1c6) || (index >= 0x1eb && index <= 0x1f0);
}

static int ends_quietly(int index)
{
    switch (index) {
    case 0x26d: case 0x285: case 0x286: case 0x28b: case 0x28f: case 0x290: case 0x291: case 0x293:
    case 0x294: case 0x299: case 0x2a2: case 0x2d8: case 0x2f8: case 0x30b: case 0x336: case 0x337:
    case 0x339: case 0x33a: case 0x351: case 0x358: case 0x359: case 0x35b: case 0x35f: case 0x360:
    case 0x368: case 0x37f: case 0x380: case 0x395: case 0x397: case 0x3a9: case 0x3d6: case 0x3f6:
    case 0x413: case 0x41a: case 0x41f: case 0x473: case 0x475: case 0x476: case 0x47c: case 0x47e:
    case 0x539: case 0x53a: case 0x53b:
        return 1;
    }
    return (index >= 0x46 && index <= 0x54) || (index >= 0x2cc && index <= 0x2d2)
        || (index >= 0x2de && index <= 0x2e1) || (index >= 0x3f8 && index <= 0x401)
        || (index >= 0x40e && index <= 0x411);
}

static int state_ended(struct GameMode *mode, const struct PacketMsgStateChange *packet, char *text, int *color)
{
    struct GameActor *actor;
    int i;

    *color = STATE_COLOR_END;
    if (is_own(packet) && own_states_count() != 0)
        own_states_remove(packet->index, packet->index);

    switch (packet->index) {
    case 0x02: case 0x25: case 0x26: case 0x27: case 0x44: case 0xa1:
        say(text, 0x1d0); break;
    case 0x06: case 0x11: say(text, 0x1d7); break;
    case 0x07: say(text, 0x26d); break;
    case 0x08: say(text, 0x26f); break;
    case 0x09: say(text, 0x271); break;
    case 0x0a: say(text, 0x273); break;
    case 0x0c: case 0x28: case 0x2a:
        if (is_own(packet) && own_states_count() != 0)
            own_states_remove(packet->index, packet->index);
        say(text, 0xd2);
        break;
    case 0x0d: say(text, 0xd1); break;
    case 0x0e: say(text, 0x277); break;
    case 0x0f: say(text, 0xd88); break;
    case 0x10: say(text, 0x1d4); break;
    case 0x12: say(text, 0x1d9); break;
    case 0x13: say(text, 0x1db); break;
    case 0x14: say(text, 0x279); break;
    case 0x15: say(text, 0x27b); break;
    case 0x16: say(text, 0x27d); break;
    case 0x17: case 0x93: say(text, 0x27f); break;
    case 0x18: say(text, 0x1dd); break;
    case 0x19: case 0xbc: say(text, 0x1d2); break;
    case 0x1a: say(text, 0x1e1); break;
    case 0x1d:
        no_line(text);
        if (is_own(packet)) {
            actor_set_state(own_player(mode), 0, 1);
            own_player(mode)->motion_held = 0;
        }
        break;
    case 0x1e: say(text, 0x287); break;
    case 0x1f: say(text, 0x289); break;
    case 0x29: say(text, 0xd2); break;
    case 0x40:
        if (is_own(packet) && own_states_count() != 0)
            own_states_remove(0x381, 0x38a);
        say(text, 0x1d7);
        break;
    case 0xf1: case 0x10f: say(text, 0x4d0); break;
    case 0xf2: case 0x110: say(text, 0x4d2); break;
    case 0xf3: case 0x111: say(text, 0x4d4); break;
    case 0xf4: case 0x112: say(text, 0x4d8); break;
    case 0xf5: case 0x113: say(text, 0x4d6); break;
    case 0xf6: case 0x114: say(text, 0x4da); break;
    case 0xf7: say(text, 0x4dc); break;
    case 0xf8: say(text, 0x4de); break;
    case 0xf9: say(text, 0x4e0); break;
    case 0xfd:
        if (is_own(packet))
            mode->boss_shown = 0;
        break;
    case 0x104:
        if (is_own(packet))
            g_session1.ownIllusion = 0;
        no_line(text);
        break;
    case 0x153:
        if (is_own(packet)) {
            struct Player *player = own_player(mode);
            for (i = 0; i < 10; i++) {
                effect_mgr_remove_type(g_effectMgr, player->aid, 0x514 + i);
                effect_mgr_remove_type(g_effectMgr, player->aid, 0x5bb + i);
            }
        }
        no_line(text);
        break;
    case 0x166:
        if (is_own(packet))
            remove_all_effects((struct GameActor *)own_player(mode), 0x2f0);
        no_line(text);
        break;
    case 0x1b1:
        if (is_own(packet))
            actor_msg(own_player(mode), ACTOR_MSG_REMOVE_EFFECT, 0x1c5);
        no_line(text);
        break;
    case 0x1b2:
        if (is_own(packet))
            actor_msg(own_player(mode), ACTOR_MSG_REMOVE_EFFECT, 0x36f);
        no_line(text);
        break;
    case 0x1b3:
        if (is_own(packet))
            actor_msg(own_player(mode), ACTOR_MSG_SLEEP_MARK_OFF, 0);
        no_line(text);
        break;
    case 0x2a1:
        if (is_own(packet))
            session_set_my_exchange_money(&g_session1, 0);
        break;
    case 0x2c6:
        if (is_own(packet) && own_player(mode) != 0)
            actor_msg(own_player(mode), ACTOR_MSG_TINT_OFF, 0);
        break;
    case 0x2da:
        if (is_own(packet))
            remove_all_effects((struct GameActor *)own_player(mode), 0x410);
        break;
    case 0x2f3:
        if (is_own(packet))
            remove_all_effects((struct GameActor *)own_player(mode), 0x425);
        break;
    case 0x37d:
        if (is_own(packet) && own_player(mode)->pose_locked == 0)
            actor_set_state(own_player(mode), 0, 1);
        break;
    case 0x3a5:
        if (is_own(packet)) {
            actor_set_state(own_player(mode), 0, 1);
            own_player(mode)->motion_held = 0;
        }
        break;
    case 0x3f5:
        *color = STATE_COLOR_BEGIN;
        say(text, 0xcb0);
        break;
    case 0x465: end_effect(packet, 0x552); break;
    case 0x48d:
        actor = state_actor(mode, packet);
        if (actor)
            actor_msg(actor, ACTOR_MSG_REMOVE_EFFECT, 0x64a);
        break;
    case 0x48e: end_effect(packet, 0x63f); break;
    case 0x48f:
        actor = state_actor(mode, packet);
        if (actor)
            actor_msg(actor, ACTOR_MSG_REMOVE_EFFECT, 0x641);
        break;
    default:
        if (ends_without_line(packet->index))
            no_line(text);
        else if (!ends_quietly(packet->index) && skill_action_mgr_has_efst_action(skill_action_mgr_instance(), packet->index) != 1)
            return 0;
        break;
    }
    return 1;
}

void game_mode_on_msg_state_change(struct GameMode *self, const struct PacketMsgStateChange *packet)
{
    struct PartyMember member;
    char prefix[0x100], text[0x100], line[0x200];
    int color, known;

    if (packet->aid != g_session1.ownAid) {
        struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);
        if (actor)
            render_object_send(actor, NULL, ACTOR_MSG_STATE_CHANGE, packet->index, packet->state, 0, 0);
    }

    session_get_party_member(&g_session1, &member, packet->aid);
    if (packet->aid == g_session1.ownAid || (member.found != 0 && member.state == 0)) {
        sprintf(prefix, "");
        if (packet->aid != g_session1.ownAid)
            sprintf(prefix, msg_str(MSG_PARTY_MEMBER_PREFIX), string_cstr(&member.name));
        memset(text, 0, sizeof text);

        if (packet->state) {
            if (packet->aid == g_session1.ownAid)
                own_states_add(self, packet->index);
            known = state_began(self, packet, text, &color);
        } else {
            known = state_ended(self, packet, text, &color);
        }
        game_mode_on_own_aid(self, packet->aid);

        if (known) {
            if (strlen(text) > 1) {
                sprintf(line, "%s%s", prefix, text);
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_CLOSE_DIALOG, 0, 0, 0, 0);
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)line, color,
                                    strlen(prefix) > 1 ? CHAT_CHANNEL_MEMBER_STATE : CHAT_CHANNEL_OWN_STATE, 0);
            }
            if (packet->aid == g_session1.ownAid)
                render_object_send(self->world->player, NULL, ACTOR_MSG_STATE_CHANGE, packet->index, packet->state, 0, 0);
        }
    }
    party_member_info_destroy(&member);
}

#pragma pack(push, 1)
struct PacketMsgStateChange2 {
    unsigned short packet_type;
    short index;
    int aid;
    unsigned char state;
    int remain_ms;
    int val1, val2, val3;
};
#pragma pack(pop)

void game_mode_on_msg_state_change2(struct GameMode *self, const struct PacketMsgStateChange2 *packet)
{
    game_mode_zc_msg_state_change(self, packet->aid, packet->state, packet->index, packet->remain_ms,
                                   packet->val1, packet->val2, packet->val3, 0, 0);
}

#pragma pack(push, 1)
struct PacketMsgStateChange3 {
    unsigned short packet_type;
    short index;
    int aid;
    unsigned char state;
    int total_ms;
    int remain_ms;
    int val1, val2, val3;
};
#pragma pack(pop)

void game_mode_on_msg_state_change3(struct GameMode *self, const struct PacketMsgStateChange3 *packet)
{
    game_mode_zc_msg_state_change(self, packet->aid, packet->state, packet->index, packet->remain_ms,
                                   packet->val1, packet->val2, packet->val3, 0, packet->total_ms);
}

#pragma pack(push, 1)
struct PacketEfstSetEnter {
    unsigned short packet_type;
    int aid;
    short index;
    int remain_ms;
    int val1, val2, val3;
};
#pragma pack(pop)

void game_mode_on_efst_set_enter(struct GameMode *self, const struct PacketEfstSetEnter *packet)
{
    game_mode_zc_msg_state_change(self, packet->aid, 1, packet->index, packet->remain_ms,
                                   packet->val1, packet->val2, packet->val3, 1, 0);
}

void game_mode_on_efst_set_enter2(struct GameMode *self, const struct PacketEfstSetEnter2 *packet)
{
    game_mode_zc_msg_state_change(self, packet->aid, 1, packet->index, packet->remain_ms,
                                   packet->val1, packet->val2, packet->val3, 1, packet->total_ms);
}

#pragma pack(push, 1)
struct PacketEquipmentEffect {
    unsigned short packet_type;
    short length;
    int aid;
    unsigned char state;
    unsigned short effects[1];
};
#pragma pack(pop)

void game_mode_on_equipment_effect(struct GameMode *self, const struct PacketEquipmentEffect *packet)
{
    int count = (int)((unsigned)(packet->length - 9) >> 1);
    int i;

    for (i = 0; i < count; i++)
        game_mode_zc_msg_state_change(self, packet->aid, packet->state, EFST_EQUIPMENT_EFFECT, 0,
                                       packet->effects[i], 0, 0, 0, 0);
}

enum { EFST_CLAIRVOYANCE = 0xb8 };

void game_mode_on_set_clairvoyance(struct GameMode *self, char on)
{
    size_t i;

    (void)self;
    if (on == 1) {
        struct OwnStateEntry entry = {0};

        entry.index = EFST_CLAIRVOYANCE;
        entry.end_tick = 999999;
        entry.value = 0;
        entry.total_ms = 9999;
        entry.remain_ms = 9999;
        own_states_push(&entry);
        return;
    }
    for (i = 0; i < own_states_count(); ++i) {
        if (own_states_at(i)->index == EFST_CLAIRVOYANCE) {
            own_states_erase_at(i);
            return;
        }
    }
}
