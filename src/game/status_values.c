#include "game/packet_handlers.h"
#include "std/integer_map.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include "game/status_values.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "session/session_windows.h"
#include "ui/character/gauge.h"
#include "ui/character/status_wnd.h"
#include <stdint.h>

enum {
    SP_BASEEXP = 1,
    SP_JOBEXP = 2,
    SP_ZENY = 20,
    SP_NEXTBASEEXP = 22,
    SP_NEXTJOBEXP = 23,
};

enum { OPTION_SHOW_EXP_GAIN = 2 };

#define CHAT_COLOR_EXP 0x00f51ef5

#pragma pack(push, 1)
struct PacketLongLongParChange {
    unsigned short packet_type;
    unsigned short var_id;
    int64_t value;
};
#pragma pack(pop)

void game_mode_on_long_long_par_change(struct GameMode *self, const struct PacketLongLongParChange *packet)
{
    switch (packet->var_id) {
    case SP_BASEEXP:
        if (session_is_admin_option(&g_session1, OPTION_SHOW_EXP_GAIN) != 0) {
            int64_t value = packet->value;
            int gained = (int)value - (int)g_session1.baseExp;

            if (value != 0 && gained != 0) {
                char line[0x100];
                int kills_left = (int)((g_session1.nextBaseExp - g_session1.baseExp) / (int64_t)gained);

                snprintf(line, 0x100, "%lld : %lld/%lld More : %d",
                            value - g_session1.baseExp, value, g_session1.nextBaseExp, kills_left);
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)line, CHAT_COLOR_EXP, 0, 0);
            }
        }
        g_session1.baseExp = packet->value;
        session_invalidate_windows(&g_session1);
        break;
    case SP_JOBEXP:
        g_session1.jobExp = packet->value;
        session_invalidate_windows(&g_session1);
        break;
    case SP_ZENY:
        if (self->exchange_open != 0) {
            self->exchange_open = 0;
            session_end_exchange2(&g_session1);
        }
        g_session1.zeny = (int)packet->value;
        session_refresh_basic_info(&g_session1);
        session_invalidate_windows(&g_session1);
        break;
    case SP_NEXTBASEEXP:
        g_session1.nextBaseExp = packet->value;
        session_invalidate_windows(&g_session1);
        break;
    case SP_NEXTJOBEXP:
        g_session1.nextJobExp = packet->value;
        session_invalidate_windows(&g_session1);
        break;
    }
    ui_basic_info_wnd_set_base_exp((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd,g_session1.baseExp, g_session1.nextBaseExp);
    ui_basic_info_wnd_set_job_exp((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd,g_session1.jobExp, g_session1.nextJobExp);
}

#pragma pack(push, 1)
struct PacketLongParChange {
    unsigned short packet_type;
    unsigned short var_id;
    int value;
};
#pragma pack(pop)

void game_mode_on_long_par_change(struct GameMode *self, const struct PacketLongParChange *packet)
{
    switch (packet->var_id) {
    case SP_BASEEXP:
        if (session_is_admin_option(&g_session1, OPTION_SHOW_EXP_GAIN) != 0) {
            char line[0x100];
            int64_t value = packet->value;
            int64_t gained = value - g_session1.baseExp;

            memset(line, 0, sizeof line);
            if (packet->value != 0 && gained != 0) {
                snprintf(line, 0x100, "%lld : %lld/%lld More : %lld",
                            gained, value, g_session1.nextBaseExp, (g_session1.nextBaseExp - g_session1.baseExp) / gained);
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)line, CHAT_COLOR_EXP, 0, 0);
            }
        }
        g_session1.baseExp = packet->value;
        session_invalidate_windows(&g_session1);
        ui_basic_info_wnd_set_base_exp((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd,g_session1.baseExp, g_session1.nextBaseExp);
        break;
    case SP_JOBEXP:
        g_session1.jobExp = packet->value;
        session_invalidate_windows(&g_session1);
        ui_basic_info_wnd_set_job_exp((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd,g_session1.jobExp, g_session1.nextJobExp);
        break;
    case SP_ZENY:
        if (self->exchange_open != 0) {
            self->exchange_open = 0;
            session_end_exchange2(&g_session1);
        }
        g_session1.zeny = packet->value;
        session_refresh_basic_info(&g_session1);
        session_invalidate_windows(&g_session1);
        break;
    case SP_NEXTBASEEXP:
        g_session1.nextBaseExp = packet->value;
        session_invalidate_windows(&g_session1);
        ui_basic_info_wnd_set_base_exp((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd,g_session1.baseExp, g_session1.nextBaseExp);
        break;
    case SP_NEXTJOBEXP:
        g_session1.nextJobExp = packet->value;
        session_invalidate_windows(&g_session1);
        ui_basic_info_wnd_set_job_exp((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd,g_session1.jobExp, g_session1.nextJobExp);
        break;
    }
}

#pragma pack(push, 1)
struct PacketNotifyExp2 {
    unsigned short packet_type;
    int aid;
    int64_t amount;
    unsigned short var_id;
    unsigned short exp_type;
};
#pragma pack(pop)

enum {
    MSG_BASE_EXP_GAINED = 0xc94,
    MSG_JOB_EXP_GAINED = 0xc9b,
    MSG_EXP_GAINED_SUFFIX = 0x64f,
    MSG_EXP_LOST_SUFFIX = 0x650,
    CHAT_CHANNEL_EXP = 0x11,
};
#define CHAT_COLOR_QUEST_EXP 0x00be58be
#define CHAT_COLOR_KILL_EXP 0x0000ffff

void game_mode_on_notify_exp2(struct GameMode *self, const struct PacketNotifyExp2 *packet)
{
    struct String line;

    (void)self;
    string_init(&line);
    if (g_session1.ownAid == packet->aid) {
        int color = packet->exp_type != 1 ? CHAT_COLOR_KILL_EXP : CHAT_COLOR_QUEST_EXP;

        if (packet->amount != 0) {
            const char *suffix;
            char amount[0x100];

            if (packet->var_id == SP_BASEEXP) {
                sprintf(amount, msg_str(MSG_BASE_EXP_GAINED), packet->amount);
                string_append_cstr(&line, amount);
            } else if (packet->var_id == SP_JOBEXP) {
                sprintf(amount, msg_str(MSG_JOB_EXP_GAINED), packet->amount);
                string_append_cstr(&line, amount);
            }
            suffix = msg_str(packet->amount >= 0 ? MSG_EXP_GAINED_SUFFIX : MSG_EXP_LOST_SUFFIX);
            string_append_cstr(&line, suffix);
            ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)string_cstr(&line), color,
                                CHAT_CHANNEL_EXP, 0);
        }
    }
    string_free(&line);
}

#pragma pack(push, 1)
struct PacketNotifyExp {
    unsigned short packet_type;
    int aid;
    int amount;
    unsigned short var_id;
    unsigned short exp_type;
};
#pragma pack(pop)

enum {
    MSG_BASE_EXP_GAINED_INT = 0x64d,
    MSG_JOB_EXP_GAINED_INT = 0x64e,
};

void game_mode_on_notify_exp(struct GameMode *self, const struct PacketNotifyExp *packet)
{
    struct String line;

    (void)self;
    string_init(&line);
    if (g_session1.ownAid == packet->aid) {
        int color = packet->exp_type != 1 ? CHAT_COLOR_KILL_EXP : CHAT_COLOR_QUEST_EXP;

        if (packet->amount != 0) {
            const char *suffix;
            char amount[0x100];

            if (packet->var_id == SP_BASEEXP) {
                sprintf(amount, msg_str(MSG_BASE_EXP_GAINED_INT), packet->amount);
                string_append_cstr(&line, amount);
            } else if (packet->var_id == SP_JOBEXP) {
                sprintf(amount, msg_str(MSG_JOB_EXP_GAINED_INT), packet->amount);
                string_append_cstr(&line, amount);
            }
            suffix = msg_str(packet->amount >= 0 ? MSG_EXP_GAINED_SUFFIX : MSG_EXP_LOST_SUFFIX);
            string_append_cstr(&line, suffix);
            ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)string_cstr(&line), color,
                                CHAT_CHANNEL_EXP, 0);
        }
    }
    string_free(&line);
}

void session_set_advanced_status(struct Session *session, int status, int value)
{
    struct IntIntPair *pair = tree_get(&session->statusValues, &status, sizeof status, sizeof *pair, tree_less_int);

    pair->value = value;
}

void game_mode_on_advanced_status_change_ack(struct GameMode *self, unsigned char result,
                                                   unsigned short status, unsigned char value)
{
    self->status_request_pending = 0;
    if (result != 1)
        return;
    session_set_advanced_status(&g_session1, status, value);
    if (g_windowMgr.status_wnd != 0)
        ui_status_wnd_restore_frame((struct UIStatusWnd *)g_windowMgr.status_wnd);
}
