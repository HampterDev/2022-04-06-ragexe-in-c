#include "contents/agency_internal.h"
#include "ui/social/party_board.h"
#include "ui/controls/buttons.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "session/state.h"
#include "session/session_items.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "core/msg_str.h"

static void chat_line(int id, int color)
{ ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)msg_str(id), color, 0, 0); }
static void board_message(struct UIWindow *w, int msg, int arg)
{ if (w) ui_window_send_msg(w, NULL, msg, arg, 0, 0, 0); }

void agency_present(struct AdventurerAgencyMgr *self, int op, int status,
                    const struct PartyBookingList *data)
{
    struct UIWindow *w = ui_window_find(0x144);
    struct UIAdvenPartyBoardWnd *board = (struct UIAdvenPartyBoardWnd *)w;
    if (status != AGENCY_OK) {
        if (op == AGENCY_LIST && board) {
            board->page = self->page; board->pages = self->pages;
            board_message(w, 0x3c, 0);
        }
        chat_line(op == AGENCY_REGISTER ? 0xd59 : op == AGENCY_DELETE ? 0xdb3 : 0xd73, 0xff);
        return;
    }
    switch (op) {
    case AGENCY_OWN:
        if (board) {
            if (data->count) ui_adven_party_board_wnd_set_own_ad(board, &data->items[0]);
            else ui_adven_party_board_wnd_set_has_ad(board, 0);
        }
        break;
    case AGENCY_LIST:
        if (board) {
            board->page = self->page; board->pages = self->pages;
            board_message(w, 0x3c, 0);
        }
        break;
    case AGENCY_DETAIL: {
        const struct AdvenPartyInfo *ad;
        struct UIApplyForPartyWnd *apply = (struct UIApplyForPartyWnd *)ui_window_find(0x159);
        if (!apply) break;
        if (!data->count || data->items[0].aid != apply->ad.aid) {
            chat_line(0xd73, 0xff);
            ui_window_mgr_post_quit1(&g_windowMgr, 0x159);
            break;
        }
        ad = &data->items[0];
        ui_apply_for_party_wnd_set_ad(apply, ad);
        if (ad->min_level > g_session1.charBaseLevel || ad->max_level < g_session1.charBaseLevel)
            chat_line(0xd58, 0xff);
        else if (session_get_party_member_count(&g_session1)) chat_line(0x4f, 0xff);
        else {
            mode_send_message(client_current_mode(), 0x109, (intptr_t)&ad->aid, (intptr_t)&ad->gid, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, 0x159);
        }
        break;
    }
    case AGENCY_REGISTER:
        chat_line(0xd5a, 0xffff64);
        board_message(w, 6, 0x197);
        break;
    case AGENCY_DELETE:
        chat_line(0xd83, 0xffff64);
        if (board) ui_button_set_show((struct UIButton *)board->booking_off, 0);
        board_message(w, 6, 0x197);
        break;
    }
}
