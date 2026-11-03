#include "actor/weather.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <windows.h>
#include <stdlib.h>
#include "game/talk_type.h"
#include "game/client.h"
#include "platform/language.h"
#include "core/msg_str.h"
#include "platform/clientinfo.h"
#include "session/session_skills.h"
#include "ui/chat/chat_window.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/render/graphic_setting.h"

enum {
    MODE_MSG_EMOTION = 0x1f,
    MODE_MSG_REPEAT_TALK = 0x2a,
    MODE_MSG_STAT_UP = 0x5b,
    MODE_MSG_BROADCAST = 7,
    MODE_MSG_REQ_RANKING = 0x11f,
};

#define ARG(n) string_cstr(&args[n])

static void mode_msg(struct GameMode *self, int msg, int a)
{
    mode_send_msg(self, msg, a, 0, 0, 0);
}

static void emote(struct GameMode *self, int emotion)
{
    mode_send_msg(self, MODE_MSG_EMOTION, emotion, 0, 0, 0);
}

static void repeat_talk(const unsigned char *types, int count)
{
    int i;

    for (i = 0; i < count; i++)
        mode_send_message(client_current_mode(), MODE_MSG_REPEAT_TALK, types[i], 0, 0, 0);
}

static void toggle_window(int id)
{
    if (!ui_window_mgr_post_quit1(&g_windowMgr, id))
        ui_window_open(id);
}

#pragma pack(push, 1)
struct PacketId {
    unsigned short id;
};
struct PacketByte {
    unsigned short id;
    unsigned char value;
};
struct PacketConfig {
    unsigned short id;
    int type;
    int value;
};
#pragma pack(pop)

#define CZ_REQ_WHISPER_LIST 0xd3
#define CZ_PARTY_CONFIG 0x2c8
#define CZ_CONFIG 0x2d8
#define CZ_CMD_RESETCOOLTIME 0xa88
#define CZ_NOCOSPLAY 0xae8

static void send_id(int id)
{
    struct PacketId packet;

    packet.id = (unsigned short)id;
    send_packet(id, &packet);
}

static void send_byte(int id, int value)
{
    struct PacketByte packet;

    packet.value = (unsigned char)value;
    packet.id = (unsigned short)id;
    send_packet(id, &packet);
}

static const short g_helpLines[] = {
    0x25d, 0x224, 0x22b, 0x22c, 0x22d, 0x22e, 0x22f, 0x230, 0x231, 0x232, 0x233, 0x234, 0x235,
    0x236, 0x237, 0x238, 0x239, 0x23a, 0x23b, 0x23c, 0x23d, 0x304, 0x305, 0x306, 0x307, 0x308,
    0x309, 0x30a, 0x30b, 0x30c, 0x30d, 0x30e, 0x30f,
};

static const unsigned char g_set1_talk_types[] = { 0x6f, 0x6a, 0x6b, 0x8b };
static const unsigned char g_q3_talk_types[] = { 0x76, 0x77 };
static const unsigned char g_set2_talk_types[] = { 0x6f, 0x6a, 0x6b, 0x76, 0x77, 0x8b };
static const unsigned char g_set3_talk_types[] = { 0x6f, 0x6a, 0x6b, 0x8b, 0xa3, 0x85, 0xa5 };

void game_mode_process_talk_type(struct GameMode *self, int type, struct String *args)
{
    struct String none[3] = {0};
    struct String name;
    struct UIWindow *w;
    int i, value;

    if (!args)
        args = none;

    switch (type) {
    case TALK_CHAT:
    case TALK_CHAT_TO_GUILD:
        talk_cmd_send_chat(self, type);
        break;
    case 0x01: mode_msg(self, MODE_MSG_BROADCAST, 0); break;
    case 0x57: mode_msg(self, MODE_MSG_BROADCAST, 1); break;
    case 0x5a: mode_msg(self, MODE_MSG_BROADCAST, 2); break;
    case 0x5b: mode_msg(self, MODE_MSG_BROADCAST, 3); break;
    case 0x64: mode_msg(self, MODE_MSG_BROADCAST, 4); break;
    case 0x65: mode_msg(self, MODE_MSG_BROADCAST, 5); break;
    case TALK_UNKNOWN:
        talk_print_msg(0x5f, TALK_COLOR_YELLOW);
        break;
    case 0x04: mode_msg(self, 0xe, 0); break;
    case 0x05: mode_msg(self, 0xf, 0); break;
    case 0x06: mode_msg(self, 0x20, 0); break;
    case 0x07:
    case 0x0b:
        talk_cmd_sound(self, type);
        break;
    case 0x08: talk_cmd_snow(self); break;
    case 0x09: talk_cmd_where(self); break;
    case 0x0a:
        talk_option_toggle(type);
        break;
    case 0x0c: talk_cmd_volume(self, args, 0); break;
    case 0x0d: talk_cmd_volume(self, args, 1); break;
    case 0x0e: talk_cmd_help(self); break;
    case 0x0f: send_id(CZ_REQ_WHISPER_LIST); break;
    case 0x10: talk_cmd_whisper_block_all(self, 0); break;
    case 0x11: talk_cmd_whisper_block_all(self, 1); break;
    case TALK_WHISPER_BLOCK: talk_cmd_whisper_block(self, args, 0); break;
    case TALK_WHISPER_UNBLOCK: talk_cmd_whisper_block(self, args, 1); break;
    case 0x15:
    case 0x16:
        if (self->world->player)
            render_object_send((struct GameActor *)self->world->player, NULL, type == 0x15 ? 0x1e : 0x1f, 0, 0, 0, 0);
        break;
    case 0x17:
        if (g_windowMgr.merchant_shop_make_wnd == 0 && g_windowMgr.merchant_item_my_shop_wnd == 0)
            ui_window_open(0x1b);
        else if (g_windowMgr.merchant_shop_make_wnd != 0 && g_windowMgr.merchant_item_my_shop_wnd == 0)
            mode_msg(self, 0x52, 0);
        else
            ui_window_mgr_message_box(&g_windowMgr, msg_str(0x24b), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        break;
    case 0x18: mode_msg(self, 0x30, 0); break;
    case 0x19:
        string_init(&name);
        string_assign_cstr(&name, ARG(0));
        value = name_card_holder_find_id(&self->actor_names, name);
        if (value)
            mode_msg(self, 0x31, value);
        break;
    case 0x1a: mode_msg(self, 0x3a, (int)ARG(0)); break;
    case 0x1b: mode_msg(self, 0x3d, 0); break;
    case 0x1c: mode_msg(self, 0x3e, (int)ARG(0)); break;

    case 0x1d: case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24:
    case 0x25: case 0x26: case 0x27: case 0x28: case 0x29:
        emote(self, type - 0x1d);
        break;
    case 0x4f:
        emote(self, 0xe);
        break;
    case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: case 0x48: case 0x49:
    case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e:
        emote(self, type - 0x33);
        break;
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
        emote(self, type - 0x34);
        break;
    case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: case 0x80:
    case 0x81: case 0x82: case 0x83: case 0x84:
        emote(self, type - 0x55);
        break;
    case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d:
        emote(self, type - 0x64);
        break;
    case 0x9e:
        value = rand() % 6;
        emote(self, (value < 0 ? 0 : value) + 0x3a);
        break;
    case 0xa9:
        emote(self, 0x41);
        break;
    case 0xad: case 0xae: case 0xaf: case 0xb0: case 0xb1: case 0xb2:
        emote(self, type - 0x69);
        break;
    case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
        emote(self, type - 0x6f);
        break;

    case 0x2b: mode_msg(self, 0x4b, 0); break;
    case 0x2c: talk_cmd_on_off(self, args); break;
    case 0x30: mode_msg(self, 0x5b, (int)ARG(0)); break;
    case 0x31: {
        int b, c;

        c = atoi(ARG(2));
        b = atoi(ARG(1));
        mode_send_msg(self, 0x5c, (int)ARG(0), b, c, 0);
        break;
    }
    case 0x33: ui_window_mgr_send_msg_(&g_windowMgr, 6, 0, 0, 0, 0); break;
    case 0x34: ui_window_mgr_send_msg_(&g_windowMgr, 7, 0, 0, 0, 0); break;
    case 0x37: mode_msg(self, 0x70, (int)ARG(0)); break;
    case 0x38: mode_msg(self, 0x6a, (int)ARG(0)); break;
    case 0x39:
        ui_window_mgr_set_chat_line(&g_windowMgr, ARG(0));
        mode_msg(self, 0x81, 0);
        break;
    case 0x3c: talk_cmd_fog(self); break;
    case 0x3d: mode_msg(self, 0x8e, 0); break;
    case 0x3e: mode_msg(self, 0x146, atoi(ARG(0))); break;
    case 0x3f: {
        int a, b, c;

        c = atoi(ARG(2));
        b = atoi(ARG(1));
        a = atoi(ARG(0));
        mode_send_msg(self, 0x8f, a, b, c, 0);
        break;
    }
    case 0x40:
        if (g_session1.showGuildEmblems) {
            talk_print("Guild Emblem Off", TALK_COLOR_YELLOW);
            g_session1.showGuildEmblems = 0;
        } else {
            talk_print("Guild Emblem On", TALK_COLOR_YELLOW);
            g_session1.showGuildEmblems = 1;
        }
        break;
    case 0x56:
        for (i = 0; i < (int)(sizeof g_helpLines / sizeof g_helpLines[0]); i++)
            talk_print_msg(g_helpLines[i], TALK_COLOR_HELP);
        break;
    case 0x58:
        value = g_session1.effectState;
        if (is_effect_state_star2(value))
            effect_state_clear_bit6(&value);
        else
            effect_state_set_bit6(&value);
        mode_msg(self, 0x90, 0x40);
        break;
    case 0x59:
        if (++self->name_balloon_pos == 4)
            self->name_balloon_pos = 0;
        break;
    case 0x5c:
        self->name_balloon_pos = self->name_balloon_pos == 0;
        break;
    case 0x5d: case 0x5e: case 0x5f: case 0x60:
        mode_msg(self, type + 0x3e, (int)ARG(0));
        break;
    case 0x63: mode_msg(self, 0xa1, (int)ARG(0)); break;
    case 0x66: talk_cmd_chat_auto_save(self); break;
    case 0x68:
    case 0x69:
        talk_cmd_show_effects(self, type);
        break;
    case 0x6a:
        g_session1.showName = g_session1.showName == 0;
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        talk_print_msg(g_session1.showName ? 0x2d2 : 0x2d3, TALK_COLOR_NOTICE);
        break;
    case 0x6c: talk_cmd_sit_or_stand(self); break;
    case 0x6d: talk_cmd_turn_by45(self, 0); break;
    case 0x6e: talk_cmd_turn_by45(self, 1); break;
    case 0x71: repeat_talk(g_set1_talk_types, sizeof g_set1_talk_types); break;
    case 0x75:
        g_lightmap = g_lightmap == 0;
        g_weather.ground_lighting = g_lightmap != 1;
        talk_refresh_option_window();
        break;
    case 0x78: repeat_talk(g_q3_talk_types, sizeof g_q3_talk_types); break;
    case 0x85:
        g_windowMgr.chat_bar_toggleable = g_windowMgr.chat_bar_toggleable == 0;
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        if (g_windowMgr.chat_bar_toggleable) {
            talk_print_msg(0x311, TALK_COLOR_NOTICE);
        } else {
            if (!g_windowMgr.chat_bar_shown)
                ui_new_chat_wnd_on_enter((struct UINewChatWnd *)g_windowMgr.new_chat_wnd);
            talk_print_msg(0x312, TALK_COLOR_NOTICE);
        }
        break;
    case 0x88: repeat_talk(g_set2_talk_types, sizeof g_set2_talk_types); break;
    case 0x8d: mode_msg(self, 0xb1, (int)ARG(0)); break;
    case 0x8e: mode_msg(self, 0xb2, (int)ARG(0)); break;
    case 0x8f:
        if (g_session1.loadingScreenRandom) {
            g_session1.loadingScreenRandom = 0;
            talk_print("RANDOM LOADING SCREEN ON", 0x6e96ff);
        } else {
            g_session1.loadingScreenRandom = 1;
            talk_print("RANDOM LOADING SCREEN OFF", 0x6e96ff);
        }
        break;
    case 0x90: mode_msg(self, MODE_MSG_REQ_RANKING, 0); break;
    case 0x91: mode_msg(self, MODE_MSG_REQ_RANKING, 1); break;
    case 0x96: mode_msg(self, MODE_MSG_REQ_RANKING, 2); break;
    case 0x9f: mode_msg(self, MODE_MSG_REQ_RANKING, 3); break;
    case 0x93:
        for (i = 0; i < 32; i++)
            mode_msg(self, MODE_MSG_STAT_UP, (int)"LEVEL");
        break;
    case 0x94:
        for (i = 0; i < 5; i++) {
            mode_msg(self, MODE_MSG_STAT_UP, (int)"SPPOINT");
            mode_msg(self, MODE_MSG_STAT_UP, (int)"JOBLEVEL");
        }
        break;
    case 0x95: talk_cmd_hi_friends(self, args); break;
    case 0x97:
        g_session1.autoMessageOn = g_session1.autoMessageOn == 0;
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        if (g_session1.autoMessageOn)
            ui_window_open(0x74);
        else
            talk_print_msg(0x3af, TALK_COLOR_NOTICE);
        break;
    case 0xa0: talk_cmd_trace_ai(self); break;
    case 0xa1: mode_msg(self, 0xbd, 0); break;
    case 0xa4: repeat_talk(g_set3_talk_types, sizeof g_set3_talk_types); break;
    case 0xb3:
        if (clientinfo_is_admin_aid(g_session1.ownAid)) {
            char was = self->copy_name;

            self->copy_name = was == 0;
            talk_print(was ? "Copy_Name_Off!!" : "Copy_Name_On!!", TALK_COLOR_NOTICE);
        }
        break;
    case 0xb4:
    case 0xb5:
        send_byte(CZ_PARTY_CONFIG, type != 0xb4);
        break;
    case 0xb6: mode_msg(self, 0x3b, (int)ARG(0)); break;
    case 0xb7: mode_msg(self, 0xfd, g_session1.equipOpenToOthers == 0); break;
    case 0xc7:
        g_session1.showStateInfo = g_session1.showStateInfo == 0;
        if (g_session1.showStateInfo) {
            talk_print_msg(0x6a6, TALK_COLOR_NOTICE);
        } else {
            w = (struct UIWindow *)self->state_icon_tip;
            if (w)
                ui_window_show(w, 0);
            talk_print_msg(0x6a7, TALK_COLOR_NOTICE);
        }
        break;
    case 0xcd: talk_cmd_build_info(self); break;
    case 0xcf: talk_cmd_navigate(self, args, 0); break;
    case 0xd0: talk_cmd_navigate(self, args, 1); break;
    case 0xd1: mode_msg(self, 0x11e, (int)ARG(0)); break;
    case 0xd7: ui_window_open(0xcb); break;
    case 0xdd:
        w = g_windowMgr.minimap_zoom_wnd;
        g_session1.minimapButtons = g_session1.minimapButtons == 0;
        if (w)
            ui_window_send(w, NULL, 6, 0x1ee, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        talk_print_msg(g_session1.minimapButtons ? 0xb2e : 0xb2f, TALK_COLOR_NOTICE);
        break;
    case 0xe1: talk_cmd_send_camera(self, args, 1); break;
    case 0xe2: talk_cmd_send_camera(self, args, 0); break;
    case TALK_SHOW_NAME:
        value = 1 - g_session1.map_rule_flags[0];
        g_session1.map_rule_flags[0] = value;
        g_session1.map_rule_flags[1] = value;
        talk_print_msg(value ? 0x2af : 0x2b0, TALK_COLOR_NOTICE);
        break;
    case 0xe4: send_id(CZ_CMD_RESETCOOLTIME); break;
    case 0xe5: {
        struct PacketConfig packet;

        packet.type = 1;
        packet.id = CZ_CONFIG;
        packet.value = g_session1.call_allowed == 0;
        send_packet(CZ_CONFIG, &packet);
        break;
    }
    case 0xeb: toggle_window(0x144); break;
    case 0xed: send_id(CZ_NOCOSPLAY); break;
    case 0xef: talk_cmd_zoom_range(self); break;
    case 0xf8: ui_window_mgr_set_shop_titles_shown(&g_windowMgr, g_windowMgr.show_shop_titles == 0); break;
    case 0xf9: mode_msg(self, 0x13d, (int)ARG(0)); break;

    case 0x2d: case 0x2e: case 0x2f: case 0x35: case 0x36: case 0x67: case 0x6b: case 0x6f:
    case 0x70: case 0x76: case 0x77: case 0x86: case 0x8b: case 0x92: case 0xa2: case 0xa3:
    case 0xa5: case 0xac: case 0xb8: case 0xd3: case 0xdb: case 0xee: case 0xf1:
        talk_option_toggle(type);
        break;
    }
    for (i = 3; i-- > 0;)
        string_free(&none[i]);
}
