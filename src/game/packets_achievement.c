#include "game/packet_handlers.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "contents/game_session.h"
#include <string.h>
#include "game/packets_achievement.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_achievements.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "platform/clock.h"

enum {
    WID_EQUIP = 0xa,
    WID_ACHIEVEMENT = 0x10e,
    WID_ACHIEVEMENT_TRACING = 0x10f,
    WID_MENU_ICON = 0x133,
};
#define UIM_ACH_REFRESH 0x3c
#define EF_ACHIEVEMENT 0x446

void game_mode_zc_all_ach_list(struct GameMode *self, int count, int total_points, unsigned rank,
                                         int current_points, int next_points, const struct AchievementEntry *entries)
{
    struct UIWindow *wnd;
    int i;

    (void)self;
    g_session1.achTotalPoints = total_points;
    g_session1.achRank = (short)rank;
    g_session1.achCurrentPoints = current_points;
    g_session1.achNextPoints = next_points;
    for (i = count; i > 0; i--, entries++) {
        session_put_achievement(&g_session1, entries);
        if (entries->reward == 1)
            session_add_title(&g_session1, entries->id);
    }
    wnd = ui_window_find(WID_ACHIEVEMENT);
    if (wnd != 0)
        ui_window_send(wnd, NULL, UIM_ACH_REFRESH, 0, 0, 0, 0);
}

static void announce(struct GameMode *self, int id)
{
    struct AchievementInfo info;
    struct String text;
    struct UIBalloonText *notice;
    struct UIWindow *wnd;
    struct Vec3 pos;

    self->achievement_notice_time = platform_ticks_ms();
    if (self->achievement_notice == 0) {
        wnd = (struct UIWindow *)ui_ach_balloon_text_new();
        self->achievement_notice = wnd;
        ui_window_create(wnd, g_renderer->width, 0x32);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->achievement_notice);
    }
    achievement_info_init(&info);
    achievement_get_info(g_achievementDb, &info, id);
    string_init(&text);
    string_format(&text, msg_str(0xa79), string_cstr(&info.text[0]));
    notice = (struct UIBalloonText *)self->achievement_notice;
    ui_balloon_text_set_lines(notice, string_cstr(&text), 0x48);
    notice->font_height = 0x14;
    notice->colour = 0xffffff;
    notice->outline = 0;
    ui_window_move_safe(self->achievement_notice, 0, 0x32);
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    effect_mgr_spawn(g_effectMgr, ((unsigned)g_session1.ownAid), EF_ACHIEVEMENT, pos, 0, -1, 0, 0);
    wnd = ui_window_find(WID_MENU_ICON);
    if (wnd != 0)
        ui_window_send(wnd, NULL, 6, 0x116, 0x1d1, 1, 0);
    string_free(&text);
    achievement_info_destruct(&info);
}

void game_mode_zc_ach_update(struct GameMode *self, int total_points, unsigned rank, int current_points,
                                       int next_points, int id, unsigned completed, const int *objectives,
                                       int completed_at, unsigned reward)
{
    struct SessionAchievementState old;
    struct AchievementEntry entry;
    struct UIWindow *wnd;

    if (session_get_achievement(&g_session1, &old, id)->completed == 0 && (unsigned char)completed == 1)
        announce(self, id);
    g_session1.achTotalPoints = total_points;
    g_session1.achRank = (short)rank;
    g_session1.achCurrentPoints = current_points;
    g_session1.achNextPoints = next_points;
    entry.id = id;
    entry.completed = (unsigned char)completed;
    memcpy(entry.objectives, objectives, sizeof entry.objectives);
    entry.completed_at = completed_at;
    entry.reward = (unsigned char)reward;
    session_put_achievement(&g_session1, &entry);
    wnd = ui_window_find(WID_ACHIEVEMENT);
    if (wnd != 0)
        ui_window_send(wnd, NULL, UIM_ACH_REFRESH, id, 0, 0, 0);
    session_refresh_tracked_achievement(&g_session1, id);
    if (session_has_tracked_achievements(&g_session1) == 1) {
        wnd = ui_window_open(WID_ACHIEVEMENT_TRACING);
        if (wnd != 0)
            ui_window_refresh(wnd);
    }
}

void game_mode_zc_req_ach_reward_ack(struct GameMode *self, unsigned result, int id)
{
    struct UIWindow *wnd;
    int index;

    (void)self;
    if ((unsigned char)result != 0)
        return;
    wnd = ui_window_find(WID_ACHIEVEMENT);
    if (wnd != 0)
        ui_window_send(wnd, NULL, UIM_ACH_REFRESH, 0, 0, 0, 0);
    index = session_add_title(&g_session1, id);
    if (index != -1)
        session_add_new_title(&g_session1, index);
}

void game_mode_zc_ack_change_title(struct GameMode *self, unsigned result, int title_id)
{
    struct String name, arg;
    struct ActorNameCard *record, copy;
    struct UIWindow *wnd;

    if ((unsigned char)result != 0)
        return;
    g_session1.ownTitleId = title_id;
    session_get_title_string(&g_session1, &name, title_id);
    record = game_mode_get_name_card(self, ((unsigned)g_session1.ownAid));
    name_card_copy_init(&copy, record);
    string_copy(&arg, &name);
    name_card_holder_set_title(&self->actor_names, ((unsigned)g_session1.ownAid), arg);
    wnd = ui_window_find(WID_EQUIP);
    if (wnd != 0)
        ui_window_send(wnd, NULL, UIM_ACH_REFRESH, 0, 0, 0, 0);
    name_card_destroy(&copy);
    string_free(&name);
}
