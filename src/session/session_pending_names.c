#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_party.h"

#include <string.h>

void session_on_pending_name(struct Session *self, int aid, const char *text)
{
    if (!text || !strlen(text) || !g_windowMgr.new_chat_wnd)
        return;
    if (!tree_find(&self->pendingNameAids, &aid, tree_less_uint))
        return;
    ui_window_send(g_windowMgr.new_chat_wnd, NULL, 0x9a, (int)text, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, 0x9a, (int)text, 0, 0, 0);
    tree_erase_key(&self->pendingNameAids, &aid, tree_less_uint, NULL);
}

void session_add_pending_name(struct Session *self, int aid)
{
    tree_add(&self->pendingNameAids, &aid, sizeof aid, tree_less_uint, NULL);
}
