#include "ui/window_types.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_windows.h"
#include "ui/wndmgr/wndmgr.h"

static void window_refresh(void *window)
{
    ui_window_send_msg(window, NULL, 0x17, 0, 0, 0, 0);
}

void session_refresh_homun_skill_item_wnd(struct Session *self)
{
    if (g_windowMgr.homun_skill_list_wnd)
        window_refresh(g_windowMgr.homun_skill_list_wnd);
    if (g_windowMgr.short_cut_wnd)
        window_refresh(g_windowMgr.short_cut_wnd);
}

void session_refresh_mer_skill_item_wnd(struct Session *self)
{
    if (g_windowMgr.mer_skill_list_wnd)
        window_refresh(g_windowMgr.mer_skill_list_wnd);
    if (g_windowMgr.short_cut_wnd)
        window_refresh(g_windowMgr.short_cut_wnd);
}

void session_refresh_skill_wnds(struct Session *self)
{
    void *window;

    if (g_windowMgr.new_skill_list_wnd)
        window_refresh(g_windowMgr.new_skill_list_wnd);
    if (g_windowMgr.short_cut_wnd)
        window_refresh(g_windowMgr.short_cut_wnd);
    window = ui_window_find(0x9f);
    if (window)
        window_refresh(window);
}

void session_take_back_mail_item(struct Session *self, int index, int used)
{
    int i;

    for (i = 0; i < 5; i++) {
        struct ItemInfo *item = &self->mailItems[i];
        int count = item->count;

        if (count != 0 && item->index == index) {
            item->count = used;
            session_add_inventory_item(&g_session1, item, 0);
            item->count = count - used;
            ui_window_mgr_refresh_item_windows(&g_windowMgr);
            break;
        }
    }
    if (g_windowMgr.rodex_compose_wnd)
        window_refresh(g_windowMgr.rodex_compose_wnd);
}

void session_set_hide_other_players(struct Session *self, BYTE flag)
{
    if (self->hideOtherPlayers == flag)
        return;
    self->hideOtherPlayers = flag;
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)msg_str(flag == 1 ? 0xf0c : 0xf0d), 0xff, 0, 0);
}

static void quick_slot_changed(int slot)
{
    game_send_message(0x139, slot + 0x24, 0, 0, 0);
}

void session_set_quick_slot(struct Session *self, BYTE kind, int id, int value, int slot)
{
    struct HotkeySlot *q;
    int i;

    for (i = 0; i < 2; i++) {
        q = &self->quickSlots[i];
        if (!(id == q->id && value == (unsigned short)q->level && kind == q->is_skill)) {
            if (skill_info_is_level_used(q->id))
                continue;
            if (id != q->id)
                continue;
        }
        q->is_skill = 0;
        q->id = 0;
        q->level = 0;
        quick_slot_changed(i);
    }
    q = &self->quickSlots[slot];
    if (id == 0) {
        q->level = 0;
        q->is_skill = 0;
        q->id = 0;
    } else {
        q->level = (short)value;
        q->is_skill = kind;
        q->id = id;
    }
    quick_slot_changed(slot);
}
