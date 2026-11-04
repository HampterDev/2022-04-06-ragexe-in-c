#include "game/packet_handlers.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "session/state.h"
#include <string.h>
#include "platform/win_types.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "game/client.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_jobs.h"
#include "ui/window.h"
#include "platform/clock.h"

enum { PLAYER_MSG_SYNC_ATTACKED_POS = 0x95 };

void game_mode_on_sync_attacked_pos(struct GameMode *self, const void *packet)
{
    (void)packet;
    render_object_send(self->world->player, NULL, PLAYER_MSG_SYNC_ATTACKED_POS, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct BonusDetail {
    unsigned char type;
    int exp;
    int death;
    int drop;
};
struct PacketPersonalInfo {
    unsigned short id;
    short length;
    int total_exp;
    int total_death;
    int total_drop;
    struct BonusDetail details[1];
};
#pragma pack(pop)
SIZE_ASSERT(BonusDetail, 13);
OFFSET_ASSERT(PacketPersonalInfo, details, 0x10);

enum { BONUS_BASIC = 0, BONUS_PREMIUM = 1, BONUS_SERVER = 2, BONUS_CAFE = 3 };

#define BONUS_LINE_COLOR 0x0062b0ff

static double per_mille(int value)
{
    return (float)value * 0.001f;
}

static void append_rates(struct String *line, int total, int basic, int cafe, int server)
{
    struct String text;

    string_init(&text);
    string_format(&text, msg_str(0xbd8), per_mille(total), per_mille(basic), per_mille(cafe),
                      string_cstr(&g_session1.serverName), per_mille(server));
    string_append(line, string_cstr(&text), text.size);
    string_free(&text);
}

static void line_from_msg(struct String *line, int msg)
{
    const char *text = msg_str(msg);

    string_init(line);
    string_assign_cstr(line, text);
}

static void chat_line(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)text, BONUS_LINE_COLOR, 0, 0);
}

void game_mode_on_personal_info(struct GameMode *self, const struct PacketPersonalInfo *packet)
{
    struct BonusDetail basic = {0}, cafe = {0}, server = {0};
    struct String exp_line, drop_line, death_line;
    const struct BonusDetail *d;
    int count, x;
    struct UIWindow *wnd;

    g_session1.skills.extra_skill_ids.last = g_session1.skills.extra_skill_ids.first;
    count = (int)((unsigned)(packet->length - 16) / 13);
    if (count <= 0)
        return;
    for (d = packet->details; count; d++, count--) {
        if (d->type == BONUS_BASIC)
            basic = *d;
        else if (d->type == BONUS_SERVER)
            server = *d;
        else if (d->type == BONUS_CAFE)
            cafe = *d;
    }

    self->notice_shown_at = platform_ticks_ms();
    if (!self->notice_wnd) {
        void *block = mem_alloc(sizeof(struct UITransBalloonText));
        self->notice_wnd = block ? ui_trans_balloon_text_construct(block) : 0;
        ui_window_create(self->notice_wnd, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->notice_wnd);
    }

    line_from_msg(&exp_line, 0x78d);
    line_from_msg(&drop_line, 0x78e);
    line_from_msg(&death_line, 0x78f);
    append_rates(&exp_line, packet->total_exp, basic.exp, cafe.exp, server.exp);
    append_rates(&drop_line, packet->total_drop, basic.drop, cafe.drop, server.drop);
    append_rates(&death_line, packet->total_death, basic.death, cafe.death, server.death);

    ui_trans_balloon_text_set_first_line((struct UITransBalloonText *)self->notice_wnd, string_cstr(&exp_line), 0xffff);
    ui_trans_balloon_text_add_colored_line((struct UITransBalloonText *)self->notice_wnd, string_cstr(&drop_line), 0xffff);
    ui_trans_balloon_text_add_colored_line((struct UITransBalloonText *)self->notice_wnd, string_cstr(&death_line), 0xffff);
    wnd = self->notice_wnd;
    x = (g_renderer->width - 640) / 2 - wnd->width / 2 + 320;
    ui_window_move_safe(wnd, x, 0x33);

    chat_line("==================================================================");
    chat_line(string_cstr(&exp_line));
    chat_line(string_cstr(&drop_line));
    chat_line(string_cstr(&death_line));
    chat_line("==================================================================");
    string_free(&death_line);
    string_free(&drop_line);
    string_free(&exp_line);
}

#pragma pack(push, 1)
struct PacketRecoverPenaltyOverweight {
    unsigned short packet_type;
    int threshold;
};
#pragma pack(pop)

void game_mode_on_recover_penalty_overweight(struct GameMode *self,
                                                    const struct PacketRecoverPenaltyOverweight *packet)
{
    (void)self;
    if (packet->threshold > 0)
        g_session1.weightPenaltyPercent = packet->threshold;
}

#pragma pack(push, 1)
struct PacketActionFailure {
    unsigned short packet_type;
    short type;
};
#pragma pack(pop)

enum {
    ACTION_FAILURE_NO_AMMO = 0,
    ACTION_FAILURE_OVERWEIGHT_ATTACK = 1,
    ACTION_FAILURE_OVERWEIGHT_SKILL = 2,
    ACTION_FAILURE_AMMO_KIND = 3,
    MSG_ACTION_FAILURE_0 = 0xf2,
    MSG_AMMO_KIND = 0xf5,
    MSG_AMMO_KIND_ASSASSIN = 0x410,
    MSG_AMMO_KIND_GUNSLINGER = 0x497,
    ACTION_FAILURE_COLOR = 0xffff64,
    ACTION_FAILURE_RED = 0xff,
};

enum {
    JOB_ASSASSIN = 12,
    JOB_GUNSLINGER = 24,
    JOB_NINJA = 25,
    JOB_ASSASSIN_CROSS = 4013,
    JOB_BABY_ASSASSIN = 4035,
    JOB_BABY_NINJA = 4222,
};

static void action_failure_line(int msg, unsigned color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(msg), color, 0, 0);
}

static int ammo_kind_message(int job)
{
    switch (job) {
    case JOB_NINJA:
    case JOB_BABY_NINJA:
        return 0;
    case JOB_GUNSLINGER:
        return MSG_AMMO_KIND_GUNSLINGER;
    case JOB_ASSASSIN:
    case JOB_ASSASSIN_CROSS:
    case JOB_BABY_ASSASSIN:
        return MSG_AMMO_KIND_ASSASSIN;
    default:
        return MSG_AMMO_KIND;
    }
}

void game_mode_on_action_failure(struct GameMode *self, const struct PacketActionFailure *packet)
{
    int msg;

    (void)self;
    switch (packet->type) {
    case ACTION_FAILURE_NO_AMMO:
    case ACTION_FAILURE_OVERWEIGHT_ATTACK:
    case ACTION_FAILURE_OVERWEIGHT_SKILL:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CLOSE_DIALOG, 0, 0, 0, 0);
        action_failure_line(MSG_ACTION_FAILURE_0 + packet->type,
                            packet->type == ACTION_FAILURE_NO_AMMO ? ACTION_FAILURE_RED : ACTION_FAILURE_COLOR);
        break;
    case ACTION_FAILURE_AMMO_KIND:
        msg = ammo_kind_message(session_get_drawn_job(&g_session1));
        if (msg != 0)
            action_failure_line(msg, ACTION_FAILURE_COLOR);
        break;
    }
}
