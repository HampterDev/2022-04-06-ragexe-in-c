#include "game/gamemode.h"
#include "ui/window_types.h"
#include "effects/rag/rag_effect.h"
#include "render/sprite_atlas.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <windows.h>
#include "render/renderer.h"
#include <errno.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <string.h>
#include "game/mode_util.h"
#include "game/client.h"
#include "game/screen_counters.h"
#include "core/msg_str.h"
#include "ui/ui_std.h"
#include "world/world.h"
#include "game/mode_callbacks.h"

unsigned g_rendererTickNext;

void renderer_run_every15s(void)
{
    unsigned now = timeGetTime();
    if (now > g_rendererTickNext) {
        g_rendererTickNext = now + 15000;
        sprite_atlas_purge(g_renderer);
    }
}

void follow_set_times(unsigned moved_at, unsigned lost_at)
{
    g_followLastMoveTick = moved_at != (unsigned)-1 ? moved_at : g_followLastMoveTick;
    g_followLostSinceTick = lost_at != (unsigned)-1 ? lost_at : g_followLostSinceTick;
}

void game_mode_remove_listed(struct GameMode *self, void *obj)
{
    struct ListNode *head = self->listed.head;
    struct ListNode *n;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, void *) == obj)
            break;
    if (n == head)
        return;
    list_erase(&self->listed, n, NULL);
}

void game_mode_notify_listed(struct GameMode *self, int arg)
{
    struct ListNode *n;
    for (n = self->listed.head->next; n != self->listed.head; n = n->next) {
        struct UIWindow *window = *LIST_VALUE(n, struct UIWindow *);
        ui_window_send_msg(window, NULL, 0x3c, arg, 0, 0, 0);
    }
}

char item_id_is994_to997(int id)
{
    return (unsigned)(id - 994) <= 3;
}

char item_id_is990_to993(int id)
{
    return (unsigned)(id - 990) <= 3;
}

void game_mode_confirm_feed_homun(void)
{
    if (g_windowMgr.exchange_wnd)
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)msg_str(0xddf), CHAT_COLOR_RED, 0, 0);
    else
        mode_send_message(client_current_mode(), 0xb8, 1, 0, 0, 0);
}

void game_mode_confirm_feed_pet(void)
{
    if (g_windowMgr.exchange_wnd)
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)msg_str(0xdde), CHAT_COLOR_RED, 0, 0);
    else
        mode_send_message(client_current_mode(), 0x96, 1, 0, 0, 0);
}

void game_mode_draw_shown_timer(struct GameMode *self)
{
    if (g_session1.timerShown)
        game_mode_draw_time_attack(self, g_session1.countdownTicks, g_session1.timerKind);
}

void game_mode_on_own_aid(struct GameMode *self, int aid)
{
    if (g_windowMgr.hide_ui_state == 2 || aid != g_session1.ownAid)
        return;
    rag_effect_send_msg(self->world->player->status_icons, NULL, 0x2c, 1, 0, 0);
}
