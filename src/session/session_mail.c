#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"

#define MAIL_SLOTS 5
#define MSG_MAIL_ITEMS 0x17

void session_add_mail_item(struct Session *self, struct ItemInfo item)
{
    int i;

    for (i = 0; i < MAIL_SLOTS; i++)
        if (self->mailItems[i].count != 0 && self->mailItems[i].index == item.index)
            break;
    if (i < MAIL_SLOTS) {
        self->mailItems[i].count += item.count;
    } else {
        for (i = 0; i < MAIL_SLOTS; i++)
            if (self->mailItems[i].count == 0) {
                item_info_assign(&self->mailItems[i], &item);
                break;
            }
    }
    if (g_windowMgr.rodex_compose_wnd)
        ui_window_send(g_windowMgr.rodex_compose_wnd, NULL, MSG_MAIL_ITEMS, 0, 0, 0, 0);
    string_free(&item.name);
    string_free(&item.id_text);
}

void session_put_mail_item(struct Session *self, struct ItemInfo item)
{
    int i;

    for (i = 0; i < MAIL_SLOTS; i++)
        if (self->mailItems[i].count == 0) {
            item_info_assign(&self->mailItems[i], &item);
            break;
        }
    string_free(&item.name);
    string_free(&item.id_text);
}

void session_reset_mail_items(struct Session *self)
{
    BYTE any = 0;
    int i;

    for (i = 0; i < MAIL_SLOTS; i++)
        if (self->mailItems[i].count != 0) {
            any = 1;
            session_add_inventory_item(&g_session1, &self->mailItems[i], 0);
        }
    if (any)
        ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void session_clear_mail_item_counts(struct Session *self)
{
    int i;

    for (i = 0; i < MAIL_SLOTS; i++)
        self->mailItems[i].count = 0;
    self->mailWeight = 0;
}

int session_count_mail_items(struct Session *self)
{
    int n = 0, i;

    for (i = 0; i < MAIL_SLOTS; i++)
        if (self->mailItems[i].count != 0)
            n++;
    return n;
}

struct ItemInfo *session_get_mail_item(struct Session *self, struct ItemInfo *out,
                                                 int index)
{
    return item_info_copy(out, &self->mailItems[index]);
}
