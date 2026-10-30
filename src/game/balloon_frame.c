#include "ui/window_access.h"
#include "skills/skill_info.h"
#include "world/object_lookup.h"
#include "session/state.h"
#include <windows.h>
#include <mmsystem.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "ui/chat/balloon.h"
#include "core/msg_str.h"
#include "game/name_balloon.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "ui/window.h"

enum {
    NOTICE_TIME = 20000,
    ACHIEVEMENT_TIME = 10000,
    MSG_SKILL_HINT = 0xea,
    BALLOON_LINE_CHARS = 0x23,
    BALLOON_GREEN = 0x32ff32,
};

static void follow_balloon(struct GameMode *self, struct UIWindow **slot, int *refilled)
{
    struct UIWindow *balloon = *slot;
    void *target;

    if (balloon == 0)
        return;
    if (*refilled != 0) {
        ui_window_mgr_post_quit2(&g_windowMgr, balloon);
        *slot = 0;
        return;
    }
    *refilled = 1;
    if (self->picked_aid == g_session1.ownAid) {
        game_mode_move_balloon_text_window(self, balloon, (struct GameActor *)self->world->player);
        return;
    }
    target = world_get_game_actor_by_aid(self->world, self->picked_aid);
    if (target != 0) {
        game_mode_move_balloon_text_window(self, *slot, (struct GameActor *)target);
        return;
    }
    target = world_get_item_by_aid(self->world, self->picked_aid);
    if (target != 0)
        game_mode_move_balloon_text_window(self, *slot, (struct GameActor *)target);
}

static void close_after(struct UIWindow **slot, unsigned since, unsigned after)
{
    if (timeGetTime() > since + after) {
        ui_window_mgr_post_quit2(&g_windowMgr, *slot);
        *slot = 0;
    }
}

static struct UIWindow *make_trans_balloon(struct UIWindow **slot)
{
    void *mem = mem_alloc(sizeof(struct UITransBalloonText));
    struct UIWindow *wnd = mem != 0 ? ui_trans_balloon_text_construct(mem) : 0;

    *slot = wnd;
    ui_window_create(wnd, 8, 8);
    ui_window_mgr_add_window(&g_windowMgr, *slot);
    return wnd;
}

static void centre_at(struct UIWindow *wnd, int y)
{
    ui_window_move_safe(wnd, (g_renderer->width - 640) / 2 - wnd->width / 2 + 320, y);
}

void game_mode_update_balloons(struct GameMode *self)
{
    struct GameActor *player;
    int made, x, y, offset;

    follow_balloon(self, &self->name_balloon, &self->name_balloon_stale);
    follow_balloon(self, &self->name_balloon2, &self->name_balloon2_stale);
    if (self->notice_wnd != 0)
        close_after(&self->notice_wnd, self->notice_shown_at, NOTICE_TIME);
    if (self->admin_whisper_balloon != 0)
        close_after(&self->admin_whisper_balloon, self->admin_whisper_time, NOTICE_TIME);

    if (self->skill_use.type == 0) {
        if (self->skill_name_wnd != 0) {
            ui_window_mgr_post_quit2(&g_windowMgr, self->skill_name_wnd);
            self->skill_name_wnd = 0;
        }
        if (self->skill_hint_wnd != 0) {
            ui_window_mgr_post_quit2(&g_windowMgr, self->skill_hint_wnd);
            self->skill_hint_wnd = 0;
        }
    } else {
        made = 0;
        if (self->skill_name_wnd == 0) {
            make_trans_balloon(&self->skill_name_wnd);
            made = 1;
        }
        if (self->skill_name_id != self->skill_use.skill_id || made == 1) {
            struct String name;

            self->skill_name_id = self->skill_use.skill_id;
            string_init(&name);
            if (self->skill_name_id == 10000)
                string_assign_cstr(&name, "Capture Monster");
            else if (self->skill_name_id == 20000)
                string_assign_cstr(&name, "Please Marry Me");
            else if (self->skill_name_id == 20001)
                string_assign_cstr(&name, "Be my Baby");
            else if (self->skill_name_id == 40000)
                string_assign_cstr(&name, "Capture Monster");
            else if (session_is_abracadabra_skill(&g_session1, self->skill_name_id) != 0)
                string_assign_cstr(&name, "????");
            else
                string_assign_cstr(&name, skill_name(self->skill_name_id));
            ui_balloon_text_set_text((struct UIBalloonText *)self->skill_name_wnd, string_cstr(&name), BALLOON_LINE_CHARS);
            centre_at(self->skill_name_wnd, g_renderer->height * 0x2e / 480);
            ((struct UIBalloonText *)self->skill_name_wnd)->colour = BALLOON_GREEN;
            ((struct UIBalloonText *)self->skill_name_wnd)->outline = 0;
            string_free(&name);
        }
        if (self->skill_hint_wnd == 0) {
            make_trans_balloon(&self->skill_hint_wnd);
            ui_balloon_text_set_text((struct UIBalloonText *)self->skill_hint_wnd, msg_str(MSG_SKILL_HINT), BALLOON_LINE_CHARS);
            centre_at(self->skill_hint_wnd, g_renderer->height * 400 / 480);
            ((struct UIBalloonText *)self->skill_hint_wnd)->colour = BALLOON_GREEN;
            ((struct UIBalloonText *)self->skill_hint_wnd)->outline = 0;
        }
    }

    if (self->status_bars != 0) {
        player = (struct GameActor *)self->world->player;
        x = player->screen_x - 30;
        y = player->screen_y;
        offset = (int)((float)(g_renderer->height * 12 / 480) * player->scale);
        if (session_is_screen_flipped(&g_session1))
            ui_window_move_safe(self->status_bars, x, y - offset);
        else
            ui_window_move_safe(self->status_bars, x, offset + y);
    }
    if (self->achievement_notice != 0)
        close_after(&self->achievement_notice, self->achievement_notice_time, ACHIEVEMENT_TIME);
}
