#include "ui/window_types.h"
#include "ui/window_access.h"
#include "ui/character/gauge.h"
#include "ui/character/gauge_sources.h"
#include "core/memory.h"
#include "game/mode_msg.h"
#include "session/session_jobs.h"
#include "ui/window.h"
#include "ui/wndmgr/wndmgr.h"

#define GAUGE_WIDTH 0x3c
#define GAUGE_HEIGHT_AP 0xd
#define GAUGE_HEIGHT 9

static int gauge_set_bars(struct UIWindow *gauge)
{
    ui_player_gauge_set(gauge, PLAYER_GAUGE_HP,
        gauge_value_from_int(&g_session1.hp), gauge_value_from_int(&g_session1.maxHp));
    ui_player_gauge_set(gauge, PLAYER_GAUGE_SP,
        gauge_value_from_int(&g_session1.sp), gauge_value_from_int(&g_session1.maxSp));
    if (session_is_fourth_job(&g_session1, session_get_drawn_job(&g_session1)) == 1) {
        ui_player_gauge_set(gauge, PLAYER_GAUGE_AP, gauge_ap_source(), gauge_max_ap_source());
        return GAUGE_HEIGHT_AP;
    }
    ui_player_gauge_set(gauge, PLAYER_GAUGE_AP, (struct GaugeValue){0}, (struct GaugeValue){0});
    return GAUGE_HEIGHT;
}

static void gauge_move(struct GameMode *self)
{
    struct UIWindow *gauge = self->status_bars;
    struct Player *player = self->world->player;
    int y = (int)((float)scale_y_from480(12) * player->scale + (float)player->screen_y);

    ui_window_move(gauge, player->screen_x - 30, y);
}

int game_mode_msg_toggle_player_gauge(struct GameMode *self)
{
    int height;

    if (self->status_bars) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->status_bars);
        self->status_bars = 0;
        return 0;
    }
    self->status_bars = ui_player_gauge_create();
    height = gauge_set_bars(self->status_bars);
    ui_window_create(self->status_bars, GAUGE_WIDTH, height);
    gauge_move(self);
    ui_window_mgr_add_to_window_list(&g_windowMgr, self->status_bars);
    return 0;
}

int game_mode_msg_relayout_player_gauge(struct GameMode *self)
{
    struct UIWindow *gauge = self->status_bars;
    int height;

    if (!gauge)
        return 0;
    height = gauge_set_bars(gauge);
    gauge = self->status_bars;
    ui_window_resize(gauge, GAUGE_WIDTH, height);
    gauge_move(self);
    return 0;
}
