#include "game/packet_handlers.h"
#include "game/replay_actor_events.h"
#include "iteminfo/item_table.h"
#include "iteminfo/item_tables.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "ui/window.h"
#include "ui/wndmgr/wndmgr.h"

enum {
    REPLAY_EV_WINDOW_MSG6 = 0x384,
    REPLAY_EV_DROP = 0x385,
    REPLAY_EV_WINDOW_DATA = 0x386,
    REPLAY_EV_TRADE_ADD_AMOUNT = 0x387,
    REPLAY_EV_CHAT_ROOM = 0x388,
    REPLAY_EV_MODE_MSG = 0x389,
    REPLAY_EV_WHISPER = 0x38a,
    REPLAY_EV_SHOP_TITLE = 0x38b,
    REPLAY_EV_WINDOW_CB_MSG = 0x38c,
    REPLAY_EV_MAKE_WINDOW = 0x38d,
    REPLAY_EV_NAVI_START = 0x38e,
    REPLAY_EV_NAVI_DATA = 0x38f,
    REPLAY_EV_NAVI_SEARCH = 0x390,
    REPLAY_EV_WINDOW_MSG = 0x391,
    REPLAY_EV_CLOSE_WINDOW = 0x392,
};

enum {
    WID_VENDING_SETUP = 0x29,
    WID_BUYING_SETUP = 0xae,
    WID_NAVI_RESULT = 0xcb,
    WID_COMBO_BOX = 0x1a,
    MODE_MSG_CANCEL_SKILL_AIM = 0x47,
};

#pragma pack(push, 1)
struct ReplayWindowMsg {
    struct ReplayEventHeader head;
    int wnd;
    int val[4];
    int msg;
};
struct ReplayWindowData {
    struct ReplayEventHeader head;
    int wnd;
    unsigned char data[1];
};
struct ReplayWindowCbMsg {
    struct ReplayEventHeader head;
    int wnd;
    int val[2];
    char text[1];
};
struct ReplayWindowId {
    struct ReplayEventHeader head;
    int value;
};
struct ReplayShopTitle {
    struct ReplayEventHeader head;
    int wnd;
    int val;
    char title[1];
};
struct ReplayModeMsg {
    struct ReplayEventHeader head;
    int msg;
    int val[3];
};
#pragma pack(pop)
OFFSET_ASSERT(ReplayWindowMsg, msg, 0x1a);
OFFSET_ASSERT(ReplayWindowData, data, 0xa);
OFFSET_ASSERT(ReplayWindowCbMsg, text, 0x12);
OFFSET_ASSERT(ReplayShopTitle, title, 0xe);

static int fix_shop_setup_msg(struct ReplayWindowMsg *rec)
{
    if (rec->val[0] == UI_ID_OK) {
        if (rec->wnd == WID_VENDING_SETUP)
            rec->val[0] = UI_ID_CANCEL;
        else if (rec->wnd == WID_BUYING_SETUP)
            return 0;
    }
    return 1;
}

void game_mode_replay_window_msg6(struct GameMode *self, char *pkt)
{
    struct ReplayWindowMsg *rec = (struct ReplayWindowMsg *)pkt;
    struct UIWindow *wnd;

    (void)self;
    if (fix_shop_setup_msg(rec) && (wnd = ui_window_find(rec->wnd)) != 0)
        ui_window_send(wnd, NULL, 6, rec->val[0], rec->val[1], rec->val[2], rec->val[3]);
}

void game_mode_replay_window_msg(struct GameMode *self, char *pkt)
{
    struct ReplayWindowMsg *rec = (struct ReplayWindowMsg *)pkt;
    struct UIWindow *wnd;

    (void)self;
    if (fix_shop_setup_msg(rec) && (wnd = ui_window_find(rec->wnd)) != 0)
        ui_window_send(wnd, NULL, rec->msg, rec->val[0], rec->val[1], rec->val[2], rec->val[3]);
}

void game_mode_replay_window_data(struct GameMode *self, char *pkt)
{
    const struct ReplayWindowData *rec = (const struct ReplayWindowData *)pkt;
    struct UIWindow *wnd = ui_window_find(rec->wnd);

    (void)self;
    if (wnd)
        ui_window_read_edit_fields(wnd, rec->data);
}

void game_mode_replay_window_cb_msg(struct GameMode *self, char *pkt)
{
    const struct ReplayWindowCbMsg *rec = (const struct ReplayWindowCbMsg *)pkt;

    (void)self;
    if (rec->wnd == WID_NAVI_RESULT && g_windowMgr.navigation_v4_wnd)
        ui_window_send(g_windowMgr.navigation_v4_wnd, NULL, rec->val[0], rec->val[1], (int)rec->text, 0, 0);
}

void game_mode_replay_make_window(struct GameMode *self, char *pkt)
{
    (void)self;
    ui_window_open(((const struct ReplayWindowId *)pkt)->value);
}

void game_mode_replay_close_window(struct GameMode *self, char *pkt)
{
    (void)self;
    ui_window_mgr_replay_close_window(&g_windowMgr, ((const struct ReplayWindowId *)pkt)->value);
}

void game_mode_replay_shop_title(struct GameMode *self, char *pkt)
{
    const struct ReplayShopTitle *rec = (const struct ReplayShopTitle *)pkt;

    (void)self;
    if (rec->wnd == WID_VENDING_SETUP || rec->wnd == WID_BUYING_SETUP)
        string_assign_cstr(&g_session1.shopTitle, rec->title);
}

void game_mode_replay_trade_add_amount(struct GameMode *self, char *pkt)
{
    self->trade_add_amount = ((const struct ReplayWindowId *)pkt)->value;
}

void game_mode_replay_mode_msg(struct GameMode *self, char *pkt)
{
    const struct ReplayModeMsg *rec = (const struct ReplayModeMsg *)pkt;

    mode_send_msg(self, rec->msg, rec->val[0], rec->val[1], rec->val[2], 0);
    mode_send_msg(self, MODE_MSG_CANCEL_SKILL_AIM, 0, 0, 0, 0);
}

void game_mode_replay_whisper(struct GameMode *self, char *pkt)
{
    const struct ReplayEventHeader *header = (const void *)pkt;
    game_replay_whisper(self, pkt, header->size);
}

static void replay_chat_room(struct GameMode *self, char *pkt)
{
    const struct ReplayEventHeader *header = (const void *)pkt;
    game_replay_chat_room(self, pkt, header->size);
}

char game_mode_window_takes_drop(struct GameMode *self, int wnd, int source)
{
    (void)self;
    switch (wnd) {
    case 0x08:
        return source == 6 || source == 9;
    case 0x16:
        if (g_windowMgr.drag_all)
            return 0;
        return source == 4 || source == 5;
    case 0x18:
        if (g_windowMgr.drag_all)
            return 0;
        return source == 3;
    case 0x17:
    case 0xa1:
        return source == 3;
    case 0x1f:
        return source == 1;
    case 0x21:
        return source == 9 || source == 1;
    case 0x28:
        return source == 1 || source == 6;
    case 0x29:
        return source == 0xb;
    case 0x2a:
        return source == 0xa;
    case 0x2b:
        return source == 0xd;
    case 0x2c:
    case 0xb2:
        return source == 0xc || source == 0x16;
    case 0xa0:
        return source == 4 || source == 5;
    case 0xaf:
        return source == 0x15;
    case 0xb8:
    case 0x13e:
        return source == 6 || source == 4;
    }
    return 0;
}

#pragma pack(push, 1)
struct ReplayDrop {
    unsigned short id, kind, length;
    int wnd;
    int x, y;
    int source;
    int index;
    int price;
    int price2;
    int drop_count;
    int count;
    int type;
    int refine;
    char identified;
    char display_name[100];
    char name[100];
    char name2[100];
    int level;
    int slots[4];
    char named;
    int option_count;
    struct ItemOption options[5];
};
#pragma pack(pop)
OFFSET_ASSERT(ReplayDrop, source, 0x12);
OFFSET_ASSERT(ReplayDrop, display_name, 0x33);
OFFSET_ASSERT(ReplayDrop, level, 0x15f);
OFFSET_ASSERT(ReplayDrop, options, 0x178);

enum { WMSG_DROPPED = 0x1d };

void game_mode_replay_drop(struct GameMode *self, struct ReplayDrop *pkt)
{
    struct DragInfo drag;
    struct UIWindow *wnd;
    int i;

    memset(&drag, 0, sizeof drag);
    string_init(&drag.display_name);
    string_init(&drag.name);
    string_init(&drag.name2);
    drag.source = pkt->source;
    drag.index = pkt->index;
    drag.price = pkt->price;
    drag.charged_price = pkt->price2;
    drag.drop_count = pkt->drop_count;
    drag.count = pkt->count;
    drag.type = pkt->type;
    drag.refine = pkt->refine;
    drag.identified = pkt->identified;
    string_assign_cstr(&drag.display_name, pkt->display_name);
    string_assign_cstr(&drag.name, pkt->name);
    string_assign_cstr(&drag.name2, pkt->name2);
    drag.level = pkt->level;
    memcpy(drag.slots, pkt->slots, sizeof drag.slots);
    drag.named = pkt->named;
    drag.option_count = pkt->option_count > 5 ? 0 : pkt->option_count;
    for (i = 0; i < drag.option_count; i++)
        drag.options[i] = pkt->options[i];

    wnd = ui_window_find(pkt->wnd);

    self->drag.source = drag.source;
    self->drag.index = drag.index;
    self->drag.price = drag.price;
    self->drag.charged_price = drag.charged_price;
    self->drag.drop_count = drag.drop_count;
    self->drag.count = drag.count;
    self->drag.type = drag.type;
    self->drag.refine = drag.refine;
    self->drag.identified = drag.identified;
    string_assign(&self->drag.display_name, string_cstr(&drag.display_name), drag.display_name.size);
    string_assign(&self->drag.name, string_cstr(&drag.name), drag.name.size);
    string_assign(&self->drag.name2, string_cstr(&drag.name2), drag.name2.size);
    self->drag.level = drag.level;
    memcpy(self->drag.slots, drag.slots, sizeof drag.slots);
    self->drag.named = drag.named;
    self->drag.drop_window = drag.drop_window;
    self->drag.option_count = drag.option_count;
    memcpy(self->drag.options, drag.options, sizeof drag.options);
    self->drag.shop_index = drag.shop_index;
    self->drag.grade = drag.grade;
    self->drag.by_index = drag.by_index;
    self->drag.equip_location = drag.equip_location;
    self->drag.view = drag.view;

    if (self->drag.drop_count != 1)
        self->replay_drop_taken = game_mode_window_takes_drop(self, pkt->wnd, pkt->source);
    else if (pkt->wnd == 0x17 || pkt->wnd == 0x2c) {
        if (pkt->type <= 3 || pkt->type == 7)
            self->replay_drop_taken = game_mode_window_takes_drop(self, pkt->wnd, pkt->source);
    } else if (pkt->wnd == 0x16) {
        if (pkt->type <= 3 || pkt->type == 7)
            self->replay_drop_taken = game_mode_window_takes_drop(self, 0x16, pkt->source);
    }

    if (wnd)
        wnd->ops->drag_and_drop(wnd, pkt->x, pkt->y, &drag);

    if (pkt->wnd == 0x24 || pkt->wnd == 0x9f) {
        self->replay_drop_taken = 0;
    } else if (self->replay_drop_taken == 1 && g_windowMgr.item_drop_cnt_wnd) {
        if (drag.type == 6) {
            const struct CardAffix *affix = card_affix_get(atoi(string_cstr(&drag.name)));

            ui_window_send(g_windowMgr.item_drop_cnt_wnd, NULL, WMSG_DROPPED, 0, (int)affix->illustName, 0, 0);
        } else {
            const struct ItemTableEntry *entry = item_table_get(atoi(string_cstr(&drag.name)));

            ui_window_send(g_windowMgr.item_drop_cnt_wnd, NULL, WMSG_DROPPED, 0, (int)entry->name, 0, 0);
        }
    }
    mode_send_msg(self, MODE_MSG_DRAG_END, 0, 0, 0, 0);
    string_free(&drag.name2);
    string_free(&drag.name);
    string_free(&drag.display_name);
}

void game_mode_on_replay_packet(struct GameMode *self, char *pkt)
{
    switch (((const struct ReplayEventHeader *)pkt)->kind) {
    case REPLAY_EV_WINDOW_MSG6:
        game_mode_replay_window_msg6(self, pkt);
        break;
    case REPLAY_EV_DROP:
        game_mode_replay_drop(self, (struct ReplayDrop *)pkt);
        break;
    case REPLAY_EV_WINDOW_DATA:
        game_mode_replay_window_data(self, pkt);
        break;
    case REPLAY_EV_TRADE_ADD_AMOUNT:
        game_mode_replay_trade_add_amount(self, pkt);
        break;
    case REPLAY_EV_CHAT_ROOM:
        replay_chat_room(self, pkt);
        break;
    case REPLAY_EV_MODE_MSG:
        game_mode_replay_mode_msg(self, pkt);
        break;
    case REPLAY_EV_WHISPER:
        game_mode_replay_whisper(self, pkt);
        break;
    case REPLAY_EV_SHOP_TITLE:
        game_mode_replay_shop_title(self, pkt);
        break;
    case REPLAY_EV_WINDOW_CB_MSG:
        game_mode_replay_window_cb_msg(self, pkt);
        break;
    case REPLAY_EV_MAKE_WINDOW:
        game_mode_replay_make_window(self, pkt);
        break;
    case REPLAY_EV_WINDOW_MSG:
        game_mode_replay_window_msg(self, pkt);
        break;
    case REPLAY_EV_CLOSE_WINDOW:
        game_mode_replay_close_window(self, pkt);
        break;
    }
}
