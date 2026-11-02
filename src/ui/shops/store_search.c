#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/shops/shops.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "iteminfo/iteminfo.h"
#include "session/session_items.h"
#include "ui/shops/shops_std.h"
#include "game/store_search_items.h"
#include "ui/wndmgr/wndmgr.h"

#define SEARCH_WND_ID 0xb5
#define SEARCH_PAGE 10
#define SEARCH_LIST_Y 100
#define SEARCH_MAX_IDS 0x14

#define SEARCH_ID_CLOSE 0xc1
#define SEARCH_ID_FUZZY 0xcd
#define SEARCH_ID_FIND 0x165
#define SEARCH_ID_PREV 0x134
#define SEARCH_ID_NEXT 0x135

#define SEARCH_MSG_RESULT 0x3c
#define SEARCH_RESULT_NOTHING_FOUND 0
#define SEARCH_RESULT_TOO_MANY 1
#define SEARCH_RESULT_NO_SEARCHES_LEFT 2
#define SEARCH_RESULT_TOO_SOON 3
#define SEARCH_RESULT_NO_SALE_INFO 4
#define SEARCH_RESULT_RESTART 0x17b

#define MSG_SEARCH_NO_ITEM_NAME 0x711
#define MSG_SEARCH_NO_ITEM 0x707
#define MSG_SEARCH_NO_CARD 0x714
#define MSG_SEARCH_BAD_PRICE 0x70c
#define MSG_SEARCH_SEARCHING 0x44e
#define MSG_SEARCH_NOTHING_FOUND 0x70b
#define MSG_SEARCH_RESULT_1 0x6f8
#define MSG_SEARCH_RESULT_2 0x706
#define MSG_SEARCH_RESULT_3 0x708
#define MSG_SEARCH_RESULT_4 0x705
#define MSG_SEARCH_CONFIRM_CLICK 0x6f9

#define PKT_CZ_SEARCH_STORE_INFO 0x835
#define PKT_CZ_SEARCH_STORE_INFO_NEXT_PAGE 0x838
#define PKT_CZ_CLOSE_SEARCH_STORE_INFO 0x83b
#define PKT_CZ_SSILIST_ITEM_CLICK 0x83c

struct UISerachStoreInfoWnd;
struct UIWindow;
void ui_serach_store_info_wnd_destroy(struct UIWindow *window);
void ui_serach_store_info_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_serach_store_info_wnd_on_draw(struct UIWindow *self);
void ui_serach_store_info_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_serach_store_info_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_serach_store_info_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_serach_store_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_serach_store_info_wnd_ops = {
    .destroy = ui_serach_store_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_serach_store_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_serach_store_info_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_serach_store_info_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_serach_store_info_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_serach_store_info_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_serach_store_info_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UISerachStoreInfoWnd {
    struct UIFrameWnd frame;
    struct UIWindow *tabs[2];
    struct UIWindow *card_name;
    struct UIWindow *item_name;
    struct UIWindow *price_from;
    struct UIWindow *price_to;
    struct UIBitmapButton *find;
    struct UIButton *prev;
    struct UIButton *next;
    int page;
    int row;
    int tab;
    int row_height;
    BYTE searching;
    BYTE fuzzy;
    struct String notice;
};
OFFSET_ASSERT(UISerachStoreInfoWnd, tabs, 0x94);

#pragma pack(push, 1)
struct PACKET_CZ_SEARCH_STORE_INFO {
    WORD id;
    WORD len;
    BYTE kind;
    int max_price;
    int min_price;
    BYTE item_count;
    BYTE card_count;
    int ids[(0x800 - 0xf) / 4];
};
struct PACKET_CZ_SSILIST_ITEM_CLICK {
    WORD id;
    int aid;
    int store_id;
    int item_id;
};
#pragma pack(pop)

static unsigned result_count(void)
{
    return (unsigned)VEC_COUNT(&g_session1.storeSearchResults, struct StoreSearchItem);
}

static struct StoreSearchItem *page_result(struct UISerachStoreInfoWnd *self, int row)
{
    struct StoreSearchItem *result =
        (struct StoreSearchItem *)g_session1.storeSearchResults.first + (self->page - 1) * SEARCH_PAGE + row;

    return result == g_session1.storeSearchResults.last ? NULL : result;
}

static void send_bare(WORD id)
{
    void *conn;

    conn = rag_connection_instance();
    rag_connection_send_packet(rag_connection_instance(), rag_connection_get_packet_length(conn, id), &id);
}

void ui_serach_store_info_wnd_destruct(struct UISerachStoreInfoWnd *self)
{
    self->frame.base.ops = &ui_serach_store_info_wnd_ops;
    string_free(&self->notice);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_serach_store_info_wnd_destroy(struct UIWindow *window)
{
    struct UISerachStoreInfoWnd *self = (struct UISerachStoreInfoWnd *)window;
    ui_serach_store_info_wnd_destruct(self);
    mem_free(self);
}

struct ItemInfo *ui_serach_store_info_wnd_item_at(struct UISerachStoreInfoWnd *self, struct ItemInfo *out, int x,
                                                        int y)
{
    struct ItemInfo info;

    item_info_construct_with_arg(&info);
    self->row = -1;
    if (g_session1.storeSearchResults.first != g_session1.storeSearchResults.last && x > 5 && x < self->frame.base.width - 10 &&
        y > SEARCH_LIST_Y && y < (self->row_height + 10) * 10) {
        int row = (y - SEARCH_LIST_Y) / self->row_height;
        struct StoreSearchItem *result;

        self->row = row;
        if ((unsigned)(row + (self->page - 1) * SEARCH_PAGE) >= result_count()) {
            item_info_copy(out, &info);
            item_info_destruct(&info);
            return out;
        }
        result = page_result(self, row);
        if (result)
            item_info_assign(&info, &result->item);
    }
    item_info_copy(out, &info);
    item_info_destruct(&info);
    return out;
}

void ui_serach_store_info_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UISerachStoreInfoWnd *self = (struct UISerachStoreInfoWnd *)window;
    struct ItemInfo info;
    struct StoreSearchItem *result;

    ui_serach_store_info_wnd_item_at(self, &info, x, y);
    if (info.id_text.size && (unsigned)(x - 0x33) <= 0x6c && (result = page_result(self, self->row)) != NULL &&
        (!g_session1.storeSearchItemLimit ||
         ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_SEARCH_CONFIRM_CLICK), UI_MSGBOX_QUESTION, 1, 0, 0,
                                "메시지", 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES)) {
        struct PACKET_CZ_SSILIST_ITEM_CLICK packet;
        void *conn;

        g_session1.store_search_pos_x = -1;
        g_session1.store_search_pos_y = -1;
        g_session1.storeSearchClickedAid = -1;
        packet.id = PKT_CZ_SSILIST_ITEM_CLICK;
        packet.aid = result->aid;
        packet.store_id = result->store_id;
        packet.item_id = item_info_get_item_id(&result->item);
        conn = rag_connection_instance();
        rag_connection_send_packet(rag_connection_instance(), rag_connection_get_packet_length(conn, packet.id),
                                  &packet);
        g_session1.storeSearchClickedAid = result->aid;
    }
    item_info_destruct(&info);
}

void ui_serach_store_info_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UISerachStoreInfoWnd *self = (struct UISerachStoreInfoWnd *)window;
    struct ItemInfo info;

    ui_serach_store_info_wnd_item_at(self, &info, x, y);
    if (info.id_text.size) {
        int over_shop = (unsigned)(x - 0x33) <= 0x6c;

        if ((x > 0xa0 && x < 0x14a) || (x > 5 && x < 0x32) || over_shop) {
            struct String text = {0};
            int tip_x = (x - ITEM_GRID_X) / ITEM_GRID_CELL * ITEM_GRID_CELL + ITEM_GRID_X;
            int tip_y = (y - ITEM_GRID_Y) / ITEM_GRID_CELL * ITEM_GRID_CELL + 1;

            ui_window_to_screen(&self->frame.base, &tip_x, &tip_y);
            string_clear(&text);
            if (over_shop) {
                struct StoreSearchItem *result = page_result(self, self->row);

                if (result) {
                    size_t end;

                    string_assign_cstr(&text, result->store_name);
                    end = string_find(&text, "\r", 0);
                    if (end != (size_t)-1) string_truncate(&text, end);
                    mode_set_cursor_action(client_current_mode(), CURSOR_HAND);
                }
            } else {
                string_free(&text);
                item_info_get_display_name(&info, &text);
            }
            if (text.size)
                mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y,
                                  session_get_item_dis_color(&info), 0);
            string_free(&text);
        }
        ui_window_invalidate_base(&self->frame.base);
    }
    item_info_destruct(&info);
}

static void search_finished(struct UISerachStoreInfoWnd *self)
{
    self->searching = 0;
    self->find->button.disabled = 0;
    ui_window_invalidate_base(&self->find->button.base);
}

static int on_result(struct UISerachStoreInfoWnd *self, int answered, int result)
{
    if (!answered) {
        if (g_session1.storeSearchResults.first == g_session1.storeSearchResults.last && self->searching)
            string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_SEARCHING));
    } else {
        string_clear(&self->notice);
        switch (result) {
        case SEARCH_RESULT_NOTHING_FOUND:
            self->page = 1;
            store_search_items_destroy(g_session1.storeSearchResults.first, g_session1.storeSearchResults.last);
            vec_clear(&g_session1.storeSearchResults);
            g_session1.storeSearchHasNextPage = 0;
            string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_NOTHING_FOUND));
            g_session1.storeSearchesLeft--;
            break;
        case SEARCH_RESULT_TOO_MANY:
            string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_RESULT_1));
            break;
        case SEARCH_RESULT_NO_SEARCHES_LEFT:
            string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_RESULT_2));
            break;
        case SEARCH_RESULT_TOO_SOON:
            ui_message_box(msg_str(MSG_SEARCH_RESULT_3));
            return 0;
        case SEARCH_RESULT_NO_SALE_INFO:
            g_session1.store_search_pos_x = -1;
            g_session1.store_search_pos_y = -1;
            g_session1.storeSearchClickedAid = -1;
            ui_message_box(msg_str(MSG_SEARCH_RESULT_4));
            return 0;
        case SEARCH_RESULT_RESTART:
            self->page = 1;
            break;
        }
        search_finished(self);
    }
    ui_button_set_show((struct UIButton *)(self->prev), self->page != 1);
    ui_button_set_show((struct UIButton *)(self->next), g_session1.storeSearchHasNextPage);
    if ((unsigned)(self->page * SEARCH_PAGE) < result_count())
        ui_button_set_show((struct UIButton *)(self->next), 1);
    ui_window_invalidate_base(&self->frame.base);
    return 0;
}

static int on_find(struct UISerachStoreInfoWnd *self)
{
    struct String card, item;

    self->searching = 1;
    self->find->button.disabled = 1;
    ui_window_invalidate_base(&self->find->button.base);
    string_init(&card);
    string_assign_cstr(&card, ui_edit_ctrl_get_text(self->card_name));
    string_init(&item);
    string_assign_cstr(&item, ui_edit_ctrl_get_text(self->item_name));
    if (!item.size) {
        self->searching = 0;
        string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_NO_ITEM_NAME));
    }
    if (self->searching) {
        struct List items, cards;
        struct String query;
        int total;

        cards.head = NULL;
        cards.size = 0;
        string_copy(&query, &item);
        item_search_by_name(&items, query, 0, self->fuzzy);
        if (!items.size) {
            self->searching = 0;
            string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_NO_ITEM));
        } else if (card.size) {
            string_copy(&query, &card);
            item_search_by_name(&cards, query, 1, self->fuzzy);
            if (!cards.size) {
                self->searching = 0;
                string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_NO_CARD));
            }
        }
        total = (int)(items.size + cards.size);
        if (self->searching) {
            int from, to, to_len;

            if (total > SEARCH_MAX_IDS) {
                ui_serach_store_info_wnd_send_msg(&self->frame.base, &self->frame.base, SEARCH_MSG_RESULT, 1, 1, 0, 0);
                list_free(&cards, NULL);
                list_free(&items, NULL);
                string_free(&item);
                string_free(&card);
                return 0;
            }
            from = atoi(ui_edit_ctrl_get_text(self->price_from));
            to = atoi(ui_edit_ctrl_get_text(self->price_to));
            to_len = (int)strlen(ui_edit_ctrl_get_text(self->price_to));
            if (from > to && to_len > 0)
                self->searching = 0;
            if (strlen(ui_edit_ctrl_get_text(self->price_from))) {
                struct String number;

                string_init(&number);
                string_format(&number, "%d", from);
                if (!string_equal_cstr(&number, ui_edit_ctrl_get_text(self->price_from)))
                    self->searching = 0;
                string_free(&number);
            }
            if (to_len > 0) {
                struct String number;

                string_init(&number);
                string_format(&number, "%d", to);
                if (!string_equal_cstr(&number, ui_edit_ctrl_get_text(self->price_to)))
                    self->searching = 0;
                string_free(&number);
            }
            if (!self->searching)
                string_assign_cstr(&self->notice, msg_str(MSG_SEARCH_BAD_PRICE));
            if (self->searching) {
                struct PACKET_CZ_SEARCH_STORE_INFO packet;
                short len = (short)(total * 4 + 0xf);

                packet.id = PKT_CZ_SEARCH_STORE_INFO;
                packet.len = len;
                packet.max_price = atoi(ui_edit_ctrl_get_text(self->price_to));
                packet.min_price = atoi(ui_edit_ctrl_get_text(self->price_from));
                packet.kind = (BYTE)self->tab;
                packet.item_count = (BYTE)items.size;
                if (len > 0x800) {
                    self->searching = 0;
                } else if (self->searching) {
                    struct ListNode *node;
                    int *id = packet.ids;

                    packet.card_count = (BYTE)cards.size;
                    for (node = items.head->next; node != items.head; node = node->next)
                        *id++ = *LIST_VALUE(node, int);
                    if (cards.head)
                        for (node = cards.head->next; node != cards.head; node = node->next)
                            *id++ = *LIST_VALUE(node, int);
                    rag_connection_send_packet(rag_connection_instance(), len, &packet);
                }
            }
        }
        list_free(&cards, NULL);
        list_free(&items, NULL);
    }
    if (!self->searching) {
        self->find->button.disabled = 0;
        ui_window_invalidate_base(&self->find->button.base);
        ui_serach_store_info_wnd_send_msg(&self->frame.base, &self->frame.base, SEARCH_MSG_RESULT, 0, 0, 0, 0);
    }
    string_free(&item);
    string_free(&card);
    return 1;
}

int ui_serach_store_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UISerachStoreInfoWnd *self = (struct UISerachStoreInfoWnd *)window;
    switch (msg) {
    case UI_MSG_TEXT_BUTTON:
        if (val1 == SEARCH_ID_PREV) {
            if (self->page > 1) {
                self->row = -1;
                self->page--;
                ui_serach_store_info_wnd_send_msg(&self->frame.base, &self->frame.base, SEARCH_MSG_RESULT, 0, 0, 0, 0);
            }
        } else if (val1 == SEARCH_ID_NEXT) {
            int page = self->page;

            self->row = -1;
            if ((unsigned)(page * SEARCH_PAGE) < result_count()) {
                self->page = page + 1;
                ui_serach_store_info_wnd_send_msg(&self->frame.base, &self->frame.base, SEARCH_MSG_RESULT, 0, 0, 0, 0);
            } else if (g_session1.storeSearchHasNextPage) {
                self->page = page + 1;
                send_bare(PKT_CZ_SEARCH_STORE_INFO_NEXT_PAGE);
            }
        }
        return 0;

    case SEARCH_MSG_RESULT:
        return on_result(self, val1, val2);

    case UI_MSG_COMMAND:
        switch (val1) {
        case SEARCH_ID_CLOSE:
            send_bare(PKT_CZ_CLOSE_SEARCH_STORE_INFO);
            g_session1.store_search_pos_x = -1;
            g_session1.store_search_pos_y = -1;
            g_session1.storeSearchClickedAid = -1;
            store_search_items_destroy(g_session1.storeSearchResults.first, g_session1.storeSearchResults.last);
            vec_clear(&g_session1.storeSearchResults);
            ui_window_mgr_post_quit1(&g_windowMgr, SEARCH_WND_ID);
            break;
        case SEARCH_ID_FUZZY:
            self->fuzzy = val2 != 0;
            break;
        case UI_ID_RADIO: {
            int i;

            for (i = 0; i < 2; i++) {
                ui_window_invalidate_base(self->tabs[i]);
                if (sender == self->tabs[i]) {
                    self->tab = i;
                    ui_radio_btn_send_msg(self->tabs[i], &self->frame.base, UI_MSG_SET_STATE, 1, 0, 0, 0);
                } else {
                    ui_radio_btn_send_msg(self->tabs[i], &self->frame.base, UI_MSG_SET_STATE, 0, 0, 0, 0);
                }
            }
            break;
        }
        case SEARCH_ID_FIND:
            if (!on_find(self))
                return 0;
            break;
        }
        break;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define SEARCH_TAB_LABEL_FIRST 0x6f2
#define MSG_SEARCH_CARD_TIP 0x709
#define MSG_SEARCH_FUZZY 0x6fc
#define MSG_SEARCH_PREV 0x41d
#define MSG_SEARCH_NEXT 0x41e
#define MSG_SEARCH_TITLE 0x6f4
#define MSG_SEARCH_LEFT 0x70a
#define MSG_SEARCH_COL_SHOP 0x6f5
#define MSG_SEARCH_COL_ITEM 0x424
#define MSG_SEARCH_COL_AMOUNT 0x6f6
#define MSG_SEARCH_COL_PRICE 0x6f7

#define SEARCH_EDIT_Y 0x24
#define SEARCH_EDIT_MAX 0x32
#define SEARCH_EDIT_BACK 0xe8
#define SEARCH_EDIT_BORDER 0xc0
#define SEARCH_GRID 0xc0c0c0
#define SEARCH_HEAD_BACK 0xdffffe
#define SEARCH_HOVER_BACK 0xfbdfca
#define SEARCH_LABEL_TEXT 0x7f3cff
#define SEARCH_PAGE_TEXT 0x8e8e8e
#define SEARCH_PAGE_HOVER 0xefb700
#define SEARCH_NOTICE_TEXT 0xff

static struct UIEditCtrl *new_edit(int x)
{
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));

    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    ui_window_create(edit, 0x4c, 0x10);
    ui_window_move(edit, x, SEARCH_EDIT_Y);
    ((struct UIEditCtrl *)edit)->max_chars = SEARCH_EDIT_MAX;
    ui_edit_ctrl_set_back_colour(edit, SEARCH_EDIT_BACK, SEARCH_EDIT_BACK, SEARCH_EDIT_BACK);
    ui_edit_ctrl_set_border_colour(edit, SEARCH_EDIT_BORDER, SEARCH_EDIT_BORDER, SEARCH_EDIT_BORDER);
    ((struct UIEditCtrl *)edit)->no_border = 0;
    return (struct UIEditCtrl *)edit;
}

static void set_disabled(struct UIWindow *edit, BYTE disabled)
{
    ((struct UIEditCtrl *)edit)->disabled = disabled;
    ui_window_invalidate(edit);
}

static struct UIButton *new_page_button(struct UISerachStoreInfoWnd *self, int msg, int id, int x)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *button = (struct UIWindow *)ui_text_button_create();
    ui_text_button_set_text(button, msg_str(msg), SEARCH_PAGE_TEXT, 0, 0xc, 0, 0);
    ((struct UITextButton *)button)->hover_colour = SEARCH_PAGE_HOVER;
    ui_window_move_base(button, x, wnd->height - 0x15);
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)button, msg_str(msg));
    button->id = id;
    ui_window_add_child(wnd, button);
    return (struct UIButton *)button;
}

void ui_serach_store_info_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UISerachStoreInfoWnd *self = (struct UISerachStoreInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *box;
    struct UIBitmapButton *find;
    struct UIEditCtrl *edit;
    struct String dir;
    static const char *const find_bitmaps[3] = {"btn_find", "btn_find_a", "btn_find_b"};
    int i;

    for (i = 0; i < 2; i++) {
        struct UIWindow *tab = mem_alloc(sizeof(struct UIRadioBtn));

        if (tab)
            tab = (struct UIWindow *)ui_radio_btn_construct((struct UIRadioBtn *)tab);
        self->tabs[i] = tab;
        ui_window_create(tab, wnd->width / 2, 0xc);
        ui_window_move_base(self->tabs[i], 10 + i * 0x28, 0x28);
        ui_window_add_child(wnd, self->tabs[i]);
    }
    ui_radio_btn_set_label(self->tabs[0], msg_str(SEARCH_TAB_LABEL_FIRST));
    ui_radio_btn_set_label(self->tabs[1], msg_str(SEARCH_TAB_LABEL_FIRST + 1));
    ui_radio_btn_send_msg(self->tabs[self->tab], wnd, UI_MSG_SET_STATE, 1, 0, 0, 0);

    edit = new_edit(0x64);
    self->item_name = &edit->base;
    ui_window_add_child(wnd, self->item_name);

    edit = new_edit(0xbb);
    self->card_name = &edit->base;
    {
        const char *tip = msg_str(MSG_SEARCH_CARD_TIP);

        if (tip)
            string_assign_cstr(&edit->tooltip, tip);
    }
    set_disabled(self->card_name, 1);
    edit->multiline = 1;
    ui_window_add_child(wnd, self->card_name);

    edit = new_edit(0x113);
    self->price_from = &edit->base;
    set_disabled(self->price_from, 1);
    ui_window_add_child(wnd, self->price_from);

    edit = new_edit(0x16c);
    self->price_to = &edit->base;
    set_disabled(self->price_to, 1);
    ui_window_add_child(wnd, self->price_to);

    box = mem_alloc(sizeof(struct UICheckBox));
    if (box)
        box = (struct UIWindow *)ui_check_box_construct(box);
    ui_window_create(box, 0xa0, 0xc);
    ui_check_box_set_bitmaps(box, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", msg_str(MSG_SEARCH_FUZZY));
    ui_window_move_base(box, 0x64, 0x3c);
    ui_check_box_set_check(box, self->fuzzy);
    ui_window_add_child(wnd, box);

    string_init(&dir);
    string_assign(&dir, UI_SKIN, 15);
    find = mem_alloc(sizeof(struct UIBitmapButton));
    if (find)
        find = ui_bitmap_button_construct(find);
    self->find = find;
    for (i = 0; i < 3; i++) {
        struct String path;

        string_copy(&path, &dir);
        string_append_cstr(&path, find_bitmaps[i]);
        string_append_cstr(&path, ".bmp");
        ui_bitmap_button_set_bitmap(self->find, string_cstr(&path), i, 0);
        string_free(&path);
    }
    ui_window_create(&self->find->button.base, self->find->bitmap_width, self->find->bitmap_height);
    ui_window_move_base(&self->find->button.base, wnd->width - 0x2d, 0x38);
    self->find->button.base.id = SEARCH_ID_FIND;
    ui_window_add_child(wnd, &self->find->button.base);
    string_free(&dir);

    self->prev = new_page_button(self, MSG_SEARCH_PREV, SEARCH_ID_PREV,
                                 wnd->width / 2 - ui_window_text_width(wnd, msg_str(MSG_SEARCH_PREV), 0, 0, 0xc, 0, 0) -
                                     0x14);
    self->next = new_page_button(self, MSG_SEARCH_NEXT, SEARCH_ID_NEXT, wnd->width / 2 + 0x14);

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);
    ui_serach_store_info_wnd_send_msg(wnd, wnd, SEARCH_MSG_RESULT, 0, 0, 0, 0);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->item_name);
    self->frame.default_id = SEARCH_ID_FIND;
}

static void draw_centred(struct UIWindow *wnd, int x, int width, int y, const char *text)
{
    ui_window_text_out(wnd, (width - ui_window_text_width(wnd, text, 0, 0, 0xc, 0, 0)) / 2 + x, y, text, 0, 0,
                     0xc, 0, 0, 0);
}

static void draw_row(struct UISerachStoreInfoWnd *self, int row, struct StoreSearchItem *result)
{
    struct UIWindow *wnd = &self->frame.base;
    int top = self->row_height * row + SEARCH_LIST_Y;
    int y = top + 7;
    struct String title, z, money, arg;
    struct String name = {0};
    const char *end;
    char buf[100];

    if (self->row == row)
        ui_window_fill_rect(wnd, 5, top, wnd->width - 10, self->row_height, SEARCH_HOVER_BACK);
    ui_window_draw_outline(wnd, 0x32, top, 0x6e, self->row_height + 1, SEARCH_GRID);
    ui_window_draw_outline(wnd, 0x14a, top, 0x1e, self->row_height + 1, SEARCH_GRID);
    if (row + 1 < SEARCH_PAGE)
        ui_window_fill_rect(wnd, 5, self->row_height * (row + 1) + SEARCH_LIST_Y, wnd->width - 10, 1,
                          SEARCH_GRID);

    ui_window_draw_item_icon(wnd, 0xd, top, &result->item, 0);

    string_init(&title);
    string_assign_cstr(&title, result->store_name);
    end = title.size ? memchr(string_cstr(&title), '\r', title.size) : NULL;
    if (end) {
        string_truncate(&title, (unsigned)(end - string_cstr(&title)));
    }
    {
        struct String shortened = {0};
        ui_window_text_out(wnd, 0x37, y, ui_window_shorten_text(wnd, &shortened, string_cstr(&title), 0x64, 0, 0xc, 0),
                         0, 0, 0xc, 0, 0, 0);
        item_info_get_display_name(&result->item, &name);
        ui_window_text_out(wnd, 0xa5, y, ui_window_shorten_text(wnd, &shortened, string_cstr(&name), 0xa0, 0, 0xc, 0),
                         0, 0, 0xc, 0, 0, 0);
        string_free(&name);
        string_free(&shortened);
    }

    sprintf(buf, "%d", result->amount);
    draw_centred(wnd, 0x14a, 0x1e, y, buf);

    string_init(&z);
    string_assign_cstr(&z, " Z");
    string_insert(&z, 0, format_money(result->price, buf), strlen(format_money(result->price, buf)));
    string_move(&money, &z);
    string_copy(&arg, &money);
    ui_window_text_out_price(wnd, result->price, arg,
                          0x1b3 - ui_window_text_width(wnd, string_cstr(&money), 0, 0, 0xc, 0, 0), y);
    string_free(&money);
    string_free(&title);
}

void ui_serach_store_info_wnd_on_draw(struct UIWindow *window)
{
    struct UISerachStoreInfoWnd *self = (struct UISerachStoreInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    BYTE empty = *ui_edit_ctrl_get_text(self->item_name) == '\0';
    struct String left;
    struct StoreSearchItem *result;
    char page[12];
    int row;

    self->find->button.disabled = empty;
    ui_window_invalidate_base(&self->find->button.base);
    set_disabled(self->card_name, empty);
    set_disabled(self->price_from, empty);
    set_disabled(self->price_to, empty);

    ui_window_draw_body(wnd, 0, 0, "");
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_SEARCH_TITLE), 0);
    ui_window_text_out_outlined(wnd, 5, 0x14, "Type", 0, SEARCH_LABEL_TEXT, 0xffffff, 0, 0xb, 1, 0);
    ui_window_text_out_outlined(wnd, 0x62, 0x14, "Item", 0, SEARCH_LABEL_TEXT, 0xffffff, 0, 0xb, 1, 0);
    ui_window_text_out_outlined(wnd, 0xb9, 0x14, "Card", 0, SEARCH_LABEL_TEXT, 0xffffff, 0, 0xb, 1, 0);
    ui_window_text_out_outlined(wnd, 0x111, 0x14, "Zeny", 0, SEARCH_LABEL_TEXT, 0xffffff, 0, 0xb, 1, 0);
    ui_window_text_out(wnd, 0x161, 0x26, "~", 0, 0, 0xc, 0, 0, 0);
    string_init(&left);
    string_format(&left, msg_str(MSG_SEARCH_LEFT), g_session1.storeSearchesLeft);
    ui_window_text_out_right(wnd, wnd->width - 0x3c, 0x3c, string_cstr(&left), 0, 0, 0xc, 0, 0);
    string_free(&left);

    ui_window_fill_rect(wnd, 5, 0x50, wnd->width - 10, 0x14, SEARCH_HEAD_BACK);
    ui_window_draw_round_frame(wnd, 5, 0x50, wnd->width - 10, self->row_height * SEARCH_PAGE + 0x15, SEARCH_GRID);
    ui_window_draw_outline(wnd, 0x32, 0x50, 0x6e, 0x15, SEARCH_GRID);
    ui_window_draw_outline(wnd, 0x14a, 0x50, 0x1e, 0x15, SEARCH_GRID);
    ui_window_fill_rect(wnd, 5, SEARCH_LIST_Y, wnd->width - 10, 1, SEARCH_GRID);
    draw_centred(wnd, 0x32, 0x6e, 0x55, msg_str(MSG_SEARCH_COL_SHOP));
    draw_centred(wnd, 0xa0, 0xaa, 0x55, msg_str(MSG_SEARCH_COL_ITEM));
    draw_centred(wnd, 0x14a, 0x1e, 0x55, msg_str(MSG_SEARCH_COL_AMOUNT));
    draw_centred(wnd, 0x168, 0x4b, 0x55, msg_str(MSG_SEARCH_COL_PRICE));
    if (self->notice.size)
        ui_window_text_out(wnd, 5, wnd->height - 0x14, string_cstr(&self->notice), 0, 0, 0xc,
                         SEARCH_NOTICE_TEXT, 0, 0);

    if ((unsigned)((self->page - 1) * SEARCH_PAGE) >= result_count())
        return;
    result = (struct StoreSearchItem *)g_session1.storeSearchResults.first + (self->page - 1) * SEARCH_PAGE;
    for (row = 0; result != g_session1.storeSearchResults.last && row != SEARCH_PAGE; row++, result++)
        draw_row(self, row, result);
    sprintf(page, " %d ", self->page);
    ui_window_text_out(wnd, (wnd->width - ui_window_text_width(wnd, page, 0, 0, 0xc, 0, 0)) / 2,
                     wnd->height - 0x14, page, 0, 0, 0xc, 0, 0, 0);
}

void ui_serach_store_info_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISerachStoreInfoWnd *self = (struct UISerachStoreInfoWnd *)window;
    struct ItemInfo info;

    ui_serach_store_info_wnd_item_at(self, &info, x, y);
    if (info.id_text.size && ((x > 0xa0 && x < 0x14a) || (unsigned)(x - 6) <= 0x2b)) {
        game_current_base_mode();
        ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
    }
    item_info_destruct(&info);
}

struct UISerachStoreInfoWnd *ui_serach_store_info_wnd_construct(struct UISerachStoreInfoWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_serach_store_info_wnd_ops;
    self->page = 1;
    self->row = -1;
    self->tab = 0;
    self->row_height = 0x19;
    self->searching = 0;
    self->fuzzy = 1;
    string_init(&self->notice);
    return self;
}

struct UIWindow *ui_serach_store_info_wnd_new(void)
{
    return (struct UIWindow *)ui_serach_store_info_wnd_construct(mem_alloc(sizeof(struct UISerachStoreInfoWnd)));
}
