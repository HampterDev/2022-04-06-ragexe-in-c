#include "ui/controls/buttons.h"
#include "ui/shops/shops.h"
#include "ui/items/items.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/mail/rodex_ui.h"
#include "ui/ui_std.h"
#include "game/rodex_mail.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

#define ROWS 6
#define ROW_STEP 0x2c

#define ID_DELETE_CHECKED 0xcb
#define ID_TITLE 0x13b
#define ID_SENDER 0x13a
#define ID_WRITE 0x129
#define ID_RETURN 0x12e
#define ID_PREV 0x134
#define ID_NEXT 0x135
#define ID_SEARCH 0x139
#define ID_REFRESH 0x197
#define ID_DELETE_ALL 0x21a
#define ID_TAKE_CHECKED 0x21b
#define ID_TAKE_ALL 0x21c

#define MODE_RODEX_OPEN 0xc2
#define MODE_RODEX_DELETE 0xc4
#define MODE_RODEX_WRITE 0x10c

#define PACKET_CZ_REQ_OPEN_MAIL 0xac0
#define PACKET_CZ_REQ_REFRESH_MAIL 0xac1
#define PACKET_CZ_REQ_ZENY_FROM_RODEX 0x9f1
#define PACKET_CZ_REQ_ITEM_FROM_RODEX 0x9f3
#define PACKET_CZ_RODEX_RETURN 0xb98

#define MSG_DELETE_ALL 0xe05
#define MSG_DELETE_CHECKED 0xe04
#define MSG_DELETE_ALL_QUESTION 0xe06
#define MSG_TAKE_ALL 0xe08
#define MSG_TAKE_CHECKED 0xe07
#define MSG_TAKE_CHECKED_QUESTION 0xe09
#define MSG_TAKE_ALL_QUESTION 0xe0a
#define MSG_DELETE_QUESTION 0xb26
#define MSG_TAKE_FIRST 0xa34
#define MSG_UNKNOWN_SENDER 0xf1b
#define MSG_TAB_RETURNED 0xddc
#define MSG_TAB_ACCOUNT 0xdda
#define MSG_TAB_CHARACTER 0xddb
#define MSG_TAB_SEARCH 0xddd
#define MSG_LOADING 0xc76
#define MSG_LOADING_TITLE 0xc77
#define MSG_SEARCH_TITLE 0xc78
#define MSG_SEARCH_SENDER 0xc79
#define MSG_MAIL_COUNT 0xbda
#define MSG_DAYS_LEFT 0xb24
#define MSG_HOURS_LEFT 0xb23
#define MSG_MINUTES_LEFT 0xb22
#define MSG_RETURNED 0xb25
#define MSG_DELETED_TITLE 0xb5b
#define MSG_TIP_RETURN 0xb20
#define MSG_TIP_REFRESH 0xb21
#define MSG_TIP_WRITE 0x402
#define MSG_TIP_PREV 0x511
#define MSG_TIP_NEXT 0x512
#define MSG_TIP_SENDER 0xa8d
#define MSG_TIP_TITLE 0xa8e

#define COLOUR_TAB 0xccb5ad
#define COLOUR_TAB_CHOSEN 0x9c3e1f
#define COLOUR_HOVER 0x51417e
#define COLOUR_SENDER 0x844a42
#define COLOUR_NOTICE 0xc89482
#define COLOUR_GREY 0xc8c8c8
#define COLOUR_RETURNED 0x666673

#define SECONDS_PER_DAY 86400
#define SECONDS_PER_HOUR 3600

struct UIRodexInboxWnd {
    struct UIItemWnd base;
    struct UIWindow *tabs[3];
    struct UIBitmapButton *btn_refresh;
    struct UIBitmapButton *btn_write;
    struct UITextButton *senders[ROWS];
    struct UITextButton *titles[ROWS];
    struct UICheckBox *checks[ROWS];
    struct UIBasicButton *btn_delete;
    struct UIBasicButton *btn_take;
    struct UIBitmapButton *btn_prev;
    struct UIBitmapButton *btn_next;
    int page;
    int page_count;
    BYTE expired;
    struct UIBitmapButton *returns[ROWS];
    struct UIRadioBtn *search_by[2];
    struct UIWindow *search_edit;
    struct UIBitmapButton *btn_search;
    struct UIWindow *tab_search;
    BYTE searching;
    struct Vec hits;
    struct Vec lists[3];
};
void ui_rodex_inbox_wnd_destroy(struct UIWindow *window);
void ui_rodex_inbox_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_rodex_inbox_wnd_on_process(struct UIWindow *self);
void ui_rodex_inbox_wnd_on_draw(struct UIWindow *self);
void ui_rodex_inbox_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_rodex_inbox_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_rodex_inbox_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_rodex_inbox_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_rodex_inbox_wnd_ops = {
    .destroy = ui_rodex_inbox_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_wnd_drag_and_drop,
    .store_info = ui_item_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_rodex_inbox_wnd_on_create,
    .on_destroy = ui_slot_no_op,
    .on_process = ui_rodex_inbox_wnd_on_process,
    .on_draw = ui_rodex_inbox_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_rodex_inbox_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_rodex_inbox_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_rodex_inbox_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_rodex_inbox_wnd_send_msg,
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

void ui_rodex_inbox_wnd_take_all(struct UIRodexInboxWnd *self);

#pragma pack(push, 1)
struct PacketRodexOpen {
    WORD id;
    long long last_id[3];
};
#pragma pack(pop)
SIZE_ASSERT(PacketRodexOpen, 26);

#define WND(self) (&(self)->base.base.frame.base)
#define BTN(b) (&(b)->button.base)

static void rodex_send(WORD id, void *packet)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), id);

    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

static struct Tree *tab_map(int type)
{
    switch (type) {
    case 0:
    case 1:
    case 2:
        return &g_rodex->tabs[type];
    default:
        return NULL;
    }
}

static struct MailPair *find_mail(struct Tree *map, long long id)
{
    return tree_find(map, &id, mail_id_greater);
}

static unsigned ref_count(const struct Vec *v)
{
    return (unsigned)VEC_COUNT(v, struct MailRef);
}

static void uncheck_all(struct UIRodexInboxWnd *self)
{
    int i;

    for (i = 0; i < ROWS; i++) {
        ui_check_box_set_check(&self->checks[i]->base, 0);
        self->checks[i]->notice_value = 0;
    }
}

static void request_list(WORD id)
{
    struct PacketRodexOpen packet;

    packet.id = id;
    packet.last_id[0] = 0;
    if (g_rodex->tabs[0].size)
        packet.last_id[0] = TREE_VALUE(tree_prev(g_rodex->tabs[0].head), struct MailPair)->id;
    packet.last_id[1] = 0;
    if (g_rodex->tabs[2].size)
        packet.last_id[1] = TREE_VALUE(tree_prev(g_rodex->tabs[2].head), struct MailPair)->id;
    packet.last_id[2] = 0;
    if (g_rodex->tabs[1].size)
        packet.last_id[2] = TREE_VALUE(tree_prev(g_rodex->tabs[1].head), struct MailPair)->id;
    rodex_send(id, &packet);
}

struct UIRodexInboxWnd *ui_rodex_inbox_wnd_construct(struct UIRodexInboxWnd *self)
{
    int i;

    ui_item_wnd_construct(&self->base);
    WND(self)->ops = &ui_rodex_inbox_wnd_ops;
    self->tabs[0] = self->tabs[1] = self->tabs[2] = NULL;
    self->btn_refresh = NULL;
    self->btn_write = NULL;
    self->btn_delete = NULL;
    self->btn_take = NULL;
    self->btn_prev = NULL;
    self->btn_next = NULL;
    self->page = 0;
    self->page_count = 0;
    self->expired = 0;
    self->search_edit = NULL;
    self->btn_search = NULL;
    self->tab_search = NULL;
    self->searching = 0;
    memset(&self->hits, 0, sizeof self->hits);
    memset(self->lists, 0, sizeof self->lists);
    for (i = 0; i < ROWS; i++) {
        self->senders[i] = NULL;
        self->titles[i] = NULL;
        self->checks[i] = NULL;
        self->returns[i] = NULL;
    }
    g_rodex->list_complete = 0;
    request_list(PACKET_CZ_REQ_OPEN_MAIL);
    return self;
}

struct UIWindow *ui_rodex_inbox_wnd_new(void)
{
    return (struct UIWindow *)ui_rodex_inbox_wnd_construct(mem_alloc(sizeof(struct UIRodexInboxWnd)));
}

void ui_rodex_inbox_wnd_destruct(struct UIRodexInboxWnd *self)
{
    WND(self)->ops = &ui_rodex_inbox_wnd_ops;
    vec_free(&self->lists[2]);
    vec_free(&self->lists[1]);
    vec_free(&self->lists[0]);
    vec_free(&self->hits);
    ui_item_wnd_destruct(&self->base);
}

void ui_rodex_inbox_wnd_destroy(struct UIWindow *window)
{
    struct UIRodexInboxWnd *self = (struct UIRodexInboxWnd *)window;
    ui_rodex_inbox_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *make_tab(struct UIRodexInboxWnd *self, int msg, int x, int y)
{
    struct UIWindow *tab = ui_bmp_radio_btn_create(0);
    const char *caption;
    int tx, ty, state;

    ui_bmp_radio_btn_set_bitmap(tab, RODEX_DIR "btn_listtab_empty_out.bmp", UI_BTN_UP);
    ui_bmp_radio_btn_set_bitmap(tab, RODEX_DIR "btn_listtab_empty_over.bmp", UI_BTN_HOVER);
    ui_bmp_radio_btn_set_bitmap(tab, RODEX_DIR "btn_listtab_empty_press.bmp", UI_BTN_DOWN);
    caption = msg_str(msg);
    ui_bmp_radio_btn_set_caption(tab, text_view_cstr(caption));
    tx = (int)((float)(tab->width - ui_window_text_width(WND(self), msg_str(msg), 0, 0, 0xc, 0, 0)) * 0.5f);
    ty = (int)((float)(tab->height - ui_window_text_height(WND(self), msg_str(msg), 0, 0, 0xc, 0, 0) - 3)
               * 0.5f);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
        ui_bmp_radio_btn_set_text_style(tab, state, 0xc, state == UI_BTN_UP ? COLOUR_TAB : COLOUR_TAB_CHOSEN, 1, 0,
                                   tx, ty);
    ui_window_move(tab, x, y);
    ui_window_add_child(WND(self), tab);
    ui_window_send_msg(tab, WND(self), UI_MSG_SET_STATE, 0, 0, 0, 0);
    return tab;
}

static struct UIBitmapButton *bitmap_button(const char *out, const char *over, const char *press)
{
    struct UIBitmapButton *mem = mem_alloc(sizeof *mem);
    struct UIBitmapButton *btn = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_bitmap_button_set_bitmap(btn, out, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(btn, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(btn, press, UI_BTN_DOWN, 0);
    return btn;
}

static struct UIBitmapButton *tool_button(struct UIRodexInboxWnd *self, const char *out, const char *over,
                                          const char *press, int tip, int x, int y, int id)
{
    struct UIBitmapButton *btn = bitmap_button(out, over, press);

    ui_bitmap_button_set_tool_tip(btn, msg_str(tip));
    ui_window_create(BTN(btn), btn->bitmap_width, btn->bitmap_height);
    ui_window_move_base(BTN(btn), x, y);
    BTN(btn)->id = id;
    ui_window_add_child(WND(self), BTN(btn));
    return btn;
}

static struct UIBasicButton *basic_button(struct UIRodexInboxWnd *self, int msg, int x, int y, int id)
{
    struct UIBasicButton *mem = mem_alloc(sizeof *mem);
    struct UIBasicButton *btn = mem ? ui_basic_button_construct(mem) : NULL;

    ui_basic_button_set_label(BTN(btn), 0x46, msg_str(msg));
    BTN(btn)->id = id;
    ui_window_move_base(BTN(btn), x, y);
    ui_window_add_child(WND(self), BTN(btn));
    return btn;
}

static void make_row(struct UIRodexInboxWnd *self, int row, int y)
{
    void *mem;
    struct UICheckBox *box;
    struct UITextButton *text;
    struct UIBitmapButton *ret;

    mem = mem_alloc(sizeof(struct UICheckBox));
    box = mem ? ui_check_box_construct(mem) : NULL;
    self->checks[row] = box;
    ui_window_create(&box->base, 0xe, 0xe);
    ui_check_box_set_bitmaps_fit(box, RODEX_DIR "checkbox_on.bmp", RODEX_DIR "checkbox_off.bmp");
    ui_window_move_base(&box->base, 0x11, y + 6);
    ui_check_box_set_check(&box->base, 0);
    box->notice_value = 0;
    ui_window_set_show_base(&box->base, 0);
    ui_window_add_child(WND(self), &box->base);

    text = ui_text_button_create();
    self->titles[row] = text;
    ui_text_button_set_text(BTN(text), "", 0, 0, 0xc, 0, 0);
    text->hover_colour = COLOUR_HOVER;
    ui_window_move_base(BTN(text), 0x45, y);
    BTN(text)->id = ID_TITLE;
    ui_button_set_tooltip(text, msg_str(MSG_TIP_TITLE));
    ui_button_set_show((struct UIButton *)(BTN(text)), 0);
    ui_window_add_child(WND(self), BTN(text));

    text = ui_text_button_create();
    self->senders[row] = text;
    ui_text_button_set_text(BTN(text), "", COLOUR_SENDER, 0, 0xb, 1, 0);
    text->hover_colour = COLOUR_HOVER;
    ui_window_move_base(BTN(text), 0x47, y + 0x10);
    BTN(text)->id = ID_SENDER;
    ui_button_set_tooltip(text, msg_str(MSG_TIP_SENDER));
    ui_button_set_show((struct UIButton *)(BTN(text)), 0);
    ui_window_add_child(WND(self), BTN(text));

    ret = bitmap_button(RODEX_DIR "btn_return_out.bmp", RODEX_DIR "btn_return_over.bmp",
                        RODEX_DIR "btn_return_press.bmp");
    self->returns[row] = ret;
    ui_window_create(BTN(ret), ret->bitmap_width, ret->bitmap_height);
    ui_window_move_base(BTN(ret), 0xba, y + 0x1d);
    ui_bitmap_button_set_tool_tip(ret, msg_str(MSG_TIP_RETURN));
    BTN(ret)->id = ID_RETURN;
    ui_button_show((struct UIButton *)(BTN(ret)), 0);
    ui_window_add_child(WND(self), BTN(ret));
}

static void make_search_radios(struct UIRodexInboxWnd *self)
{
    static const int labels[2] = { MSG_SEARCH_TITLE, MSG_SEARCH_SENDER };
    int i;

    for (i = 0; i < 2; i++) {
        void *mem = mem_alloc(sizeof(struct UIRadioBtn));
        struct UIRadioBtn *radio = mem ? ui_radio_btn_construct(mem) : NULL;

        self->search_by[i] = radio;
        ui_window_create(&radio->base, 0x33, 0xc);
        string_assign_cstr(&radio->on_bitmap, RODEX_DIR "checkbox_search_on.bmp");
        string_assign_cstr(&radio->off_bitmap, RODEX_DIR "checkbox_search_off.bmp");
        ui_window_invalidate_base(&radio->base);
        ui_radio_btn_set_label(&radio->base, msg_str(labels[i]));
        radio->label_colour = 0x654939;
        ui_window_move_base(&radio->base, 0x16 + i * 0x33, 0x44);
        radio->value = ID_SEARCH;
        ui_radio_btn_send_msg(&radio->base, WND(self), UI_MSG_SET_STATE, i == 0, 0, 0, 0);
        ui_window_add_child(WND(self), &radio->base);
    }
}

void ui_rodex_inbox_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIRodexInboxWnd *self = (struct UIRodexInboxWnd *)window;
    struct UIWindow *wnd = WND(self);
    void *mem;
    int row, y;

    rodex_purge_deleted(g_rodex);
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);
    self->tabs[2] = make_tab(self, MSG_TAB_RETURNED, 0xcf, 0x46);
    self->tabs[1] = make_tab(self, MSG_TAB_ACCOUNT, 0x13, 0x46);
    self->tabs[0] = make_tab(self, MSG_TAB_CHARACTER, 0x71, 0x46);
    if (g_rodex->open_type <= 2)
        ui_window_send_msg(self->tabs[g_rodex->open_type], wnd, UI_MSG_SET_STATE, 1, 0, 0, 0);

    self->btn_refresh = tool_button(self, RODEX_DIR "btn_refresh_out.bmp", RODEX_DIR "btn_refresh_over.bmp",
                                    RODEX_DIR "btn_refresh_press.bmp", MSG_TIP_REFRESH, 0xef, 0x14, ID_REFRESH);
    self->btn_write = tool_button(self, RODEX_DIR "btn_write_out.bmp", RODEX_DIR "btn_write_over.bmp",
                                  RODEX_DIR "btn_write_press.bmp", MSG_TIP_WRITE, 0x10c, 0x12, ID_WRITE);
    for (row = 0, y = 0x66; row < ROWS; row++, y += ROW_STEP)
        make_row(self, row, y);
    self->btn_delete = basic_button(self, MSG_DELETE_ALL, 4, 0x178, ID_DELETE_ALL);
    self->btn_take = basic_button(self, MSG_TAKE_ALL, 0x54, 0x188, ID_TAKE_ALL);
    self->btn_prev = tool_button(self, RODEX_DIR "btn_prev_out.bmp", RODEX_DIR "btn_prev_over.bmp",
                                 RODEX_DIR "btn_prev_press.bmp", MSG_TIP_PREV, 0xf6, 0x176, ID_PREV);
    self->btn_next = tool_button(self, RODEX_DIR "btn_next_out.bmp", RODEX_DIR "btn_next_over.bmp",
                                 RODEX_DIR "btn_next_press.bmp", MSG_TIP_NEXT, 0x10e, 0x176, ID_NEXT);
    make_search_radios(self);

    mem = mem_alloc(sizeof(struct UIEditCtrl));
    self->search_edit = mem ? ui_edit_ctrl_construct(mem) : NULL;
    ui_window_create(self->search_edit, 0x84, 0x12);
    ui_window_move(self->search_edit, 0x85, 0x41);
    UI_EDIT_MAX_CHARS(self->search_edit) = 0x10;
    UI_EDIT_NO_BORDER(self->search_edit) = 0;
    ui_edit_ctrl_set_border_colour(self->search_edit, 0xad, 0xad, 0xad);
    ((struct UIEditCtrl *)self->search_edit)->keep_ime_on_enter = 1;
    ui_window_add_child(wnd, self->search_edit);

    self->btn_search = bitmap_button(RODEX_DIR "btn_search_out.bmp", RODEX_DIR "btn_search_over.bmp",
                                     RODEX_DIR "btn_search_press.bmp");
    ui_window_create(BTN(self->btn_search), self->btn_search->bitmap_width, self->btn_search->bitmap_height);
    ui_window_move_base(BTN(self->btn_search), 0x108, 0x41);
    BTN(self->btn_search)->id = ID_SEARCH;
    ui_window_add_child(wnd, BTN(self->btn_search));
    self->tab_search = make_tab(self, MSG_TAB_SEARCH, 0xdf, 0x56);

    ui_window_move(self->tabs[1], 0x13, 0x56);
    ui_window_move(self->tabs[0], 0x57, 0x56);
    ui_window_move(self->tabs[2], 0x9b, 0x56);
    ui_window_move_base(BTN(self->btn_refresh), 0xf8, 0x14);
    ui_window_move_base(BTN(self->btn_write), 0x115, 0x12);
    for (row = 0, y = 0x79; row < ROWS; row++, y += ROW_STEP) {
        ui_window_move_base(&self->checks[row]->base, 0x11, y + 7);
        ui_window_move_base(BTN(self->titles[row]), 0x45, y);
        ui_window_move_base(BTN(self->senders[row]), 0x47, y + 0x14);
    }
    ui_window_move_base(BTN(self->btn_delete), 4, 0x188);
    ui_window_move_base(BTN(self->btn_prev), 0xff, 0x186);
    ui_window_move_base(BTN(self->btn_next), 0x117, 0x186);
    ui_rodex_inbox_wnd_rebuild_lists(self);
    ui_window_invalidate_base(wnd);
}

static void draw_row_icons(struct UIRodexInboxWnd *self, int row, BYTE type, BYTE is_read, BYTE returned)
{
    struct UIWindow *wnd = WND(self);
    int y = row * ROW_STEP;
    const char *status, *attached;

    if (returned == 1)
        status = RODEX_DIR "icon_status_mail_returned.bmp";
    else if (is_read)
        status = RODEX_DIR "icon_status_mail_read.bmp";
    else
        status = RODEX_DIR "icon_status_mail_received.bmp";
    ui_window_draw_bitmap(wnd, 0x24, y + 0x7c, res_mgr_get(g_resMgr(), status), 1);
    if ((type & (MAIL_HAS_ZENY | MAIL_HAS_ITEMS)) == (MAIL_HAS_ZENY | MAIL_HAS_ITEMS))
        attached = RODEX_DIR "icon_zeny_n_item.bmp";
    else if (type & MAIL_HAS_ZENY)
        attached = RODEX_DIR "icon_zeny.bmp";
    else if (type & MAIL_HAS_ITEMS)
        attached = RODEX_DIR "icon_item.bmp";
    else
        return;
    ui_window_draw_bitmap(wnd, 0xd8, y + 0x8b, res_mgr_get(g_resMgr(), RODEX_DIR "shadow_item_on.bmp"), 1);
    ui_window_draw_bitmap(wnd, 0xdb, y + 0x7f, res_mgr_get(g_resMgr(), attached), 1);
}

static void draw_time_left(struct UIRodexInboxWnd *self, int row, int32_t since, int32_t expire, BYTE returned)
{
    struct UIWindow *wnd = WND(self);
    struct String text;
    int left, n;

    (void)platform_time_diff(platform_time_now(), since);
    left = (int)platform_time_diff(expire, platform_time_now());
    if (left <= 0) {
        self->expired = 1;
        return;
    }
    string_init(&text);
    if ((n = left / SECONDS_PER_DAY) > 0)
        string_format(&text, msg_str(MSG_DAYS_LEFT), n);
    else if ((n = left / SECONDS_PER_HOUR) > 0)
        string_format(&text, msg_str(MSG_HOURS_LEFT), n);
    else
        string_format(&text, msg_str(MSG_MINUTES_LEFT), left / 60);
    if (returned == 1) {
        ui_window_text_out_centred(wnd, 0x104, row * ROW_STEP + 0x7d, msg_str(MSG_RETURNED), 0, 0, 0xb,
                                COLOUR_RETURNED, 0);
        ui_window_text_out_centred(wnd, 0x104, row * ROW_STEP + 0x8a, string_cstr(&text), 0, 0, 0xb,
                                COLOUR_RETURNED, 0);
    } else {
        ui_window_text_out_centred(wnd, 0x104, row * ROW_STEP + 0x82, string_cstr(&text), 0, 0, 0xb, 0xa0ae9,
                                1);
    }
    string_free(&text);
}

static void set_text(struct UITextButton *btn, const char *text)
{
    string_assign_cstr(&btn->button.text, text);
    ui_text_button_fit_width(btn, 0x8c, BTN(btn)->height);
}

static void show_row(struct UIRodexInboxWnd *self, int row, long long id, struct String sender,
                     struct String title, BYTE is_read, BYTE deleted, int trusted)
{
    struct String from, subject;
    struct String nr = {0};
    struct UITextButton *name = self->senders[row], *subj = self->titles[row];
    COLORREF colour;

    string_copy(&from, &sender);
    string_copy(&subject, &title);
    if (navi_links_strip_nr(&nr, string_view(&subject)))
        ui_button_set_tooltip(name, "");
    string_assign(&subject, string_cstr(&nr), nr.size);
    rodex_expand_msg_tags(&from);
    rodex_expand_msg_tags(&subject);

    ui_window_set_show_base(&self->checks[row]->base, 1);
    self->checks[row]->notice_value = (int)id;
    set_text(name, string_cstr(&from));
    name->value = (int)id;
    ui_button_set_show((struct UIButton *)(BTN(name)), 1);
    if (trusted == 0) {
        ui_button_show((struct UIButton *)(BTN(self->returns[row])), 1);
        self->returns[row]->button.cmd_value = (int)id;
        colour = is_read == 1 ? 0xc8c8e6 : 0xff;
    } else {
        colour = is_read == 1 ? COLOUR_GREY : 0;
    }
    subj->text_colour = colour;
    set_text(subj, string_cstr(&subject));
    subj->value = (int)id;
    ui_button_set_show((struct UIButton *)(BTN(subj)), 1);
    if (deleted == 1) {
        self->checks[row]->notice_value = -1;
        ui_button_set_show((struct UIButton *)(BTN(name)), 0);
        subj->text_colour = COLOUR_GREY;
        set_text(subj, msg_str(MSG_DELETED_TITLE));
        subj->value = -1;
        ui_button_set_show((struct UIButton *)(BTN(subj)), 1);
    }
    string_free(&nr);
    string_free(&subject);
    string_free(&from);
    string_free(&sender);
    string_free(&title);
}

static void draw_mail(struct UIRodexInboxWnd *self, int row, struct Tree *map, struct MailPair *node,
                      BYTE returned, int show_tab, int trusted_known)
{
    struct Mail *mail = &node->mail;
    struct String sender, title;

    draw_row_icons(self, row, mail->type, mail->is_read, returned);
    draw_time_left(self, row, mail->sent_time, mail->expire_time, returned);
    if (self->expired == 1) {
        self->expired = 0;
        tree_erase(map, TREE_NODE_OF(node), mail_pair_destroy);
        return;
    }
    if (show_tab) {
        if (show_tab == 1) {
            ui_check_box_set_check(&self->checks[row]->base, 0);
            self->checks[row]->notice_value = 0;
        }
        ui_window_set_show_base(&self->checks[row]->base, 1);
        ui_button_set_show((struct UIButton *)(BTN(self->senders[row])), 1);
        ui_button_set_show((struct UIButton *)(BTN(self->titles[row])), 1);
    }
    string_copy(&title, &mail->title);
    string_copy(&sender, &mail->sender);
    show_row(self, row, node->id, sender, title, mail->is_read, mail->deleted,
             trusted_known ? mail->sender_trusted : -1);
}

void ui_rodex_inbox_wnd_on_draw(struct UIWindow *window)
{
    struct UIRodexInboxWnd *self = (struct UIRodexInboxWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct String text;
    int row;

    ui_window_clear_canvas(wnd, 0xff00ff);
    ui_window_draw_bitmap(wnd, 0, 0, res_mgr_get(g_resMgr(), RODEX_DIR "bg_rodex_list.bmp"), 0);
    for (row = 0; row < ROWS; row++)
        ui_button_show((struct UIButton *)(BTN(self->returns[row])), 0);

    if (!g_rodex->list_complete) {
        ui_window_draw_bitmap(wnd, 0x1f, 0x9a, res_mgr_get(g_resMgr(), RODEX_DIR "img_notice.bmp"), 1);
        string_init(&text);
        string_assign_cstr(&text, msg_str(MSG_LOADING));
        ui_window_text_out_wrapped(wnd, 0x1f, 0xe5, &text, 0xf8, 3, 6, 0, 0xc, 0, 0);
        string_free(&text);
        ui_window_text_out_centred(wnd, 0x1f, 0x10e, msg_str(MSG_LOADING_TITLE), 0xf8, 0, 0x12, COLOUR_NOTICE, 1);
        return;
    }

    if (self->searching == 1) {
        for (row = 0; row < ROWS; row++) {
            unsigned idx = self->page * ROWS + row;
            struct MailRef *ref;
            struct Tree *map;
            struct MailPair *node;

            if (idx >= ref_count(&self->hits)) {
                ui_window_set_show_base(&self->checks[row]->base, 0);
                ui_check_box_set_check(&self->checks[row]->base, 0);
                self->checks[row]->notice_value = 0;
                ui_button_set_show((struct UIButton *)(BTN(self->senders[row])), 0);
                ui_button_set_show((struct UIButton *)(BTN(self->titles[row])), 0);
                continue;
            }
            ref = (struct MailRef *)self->hits.first + idx;
            if (!(map = tab_map(ref->type)) || !(node = find_mail(map, ref->id)))
                continue;
            draw_mail(self, row, map, node, ref->type == 2, 0, 1);
        }
    } else {
        for (row = 0; row < ROWS; row++) {
            unsigned idx = self->page * ROWS + row;
            int type = g_rodex->open_type;
            struct Vec *list;
            struct Tree *map;
            struct MailPair *node;

            ui_window_set_show_base(&self->checks[row]->base, 0);
            ui_button_set_show((struct UIButton *)(BTN(self->senders[row])), 0);
            ui_button_set_show((struct UIButton *)(BTN(self->titles[row])), 0);
            if (type != 1 && type != 2) {
                type = 0;
                ui_button_set_show((struct UIButton *)(BTN(self->senders[row])), 0);
                ui_button_set_show((struct UIButton *)(BTN(self->titles[row])), 0);
            }
            list = &self->lists[type];
            map = &g_rodex->tabs[type];
            if (idx >= ref_count(list))
                continue;
            if (!(node = find_mail(map, ((struct MailRef *)list->first)[idx].id)))
                continue;
            draw_mail(self, row, map, node, 0, type == 0 ? 1 : 2, type == 0);
        }
    }

    string_init(&text);
    string_format(&text, "%d", g_rodex->mail_count);
    ui_window_text_out_right(wnd, 0x111, 0x2e, string_cstr(&text), 0, 0, 0xb, COLOUR_NOTICE, 1);
    string_free(&text);
    ui_window_text_out(wnd, 0x111, 0x2e, msg_str(MSG_MAIL_COUNT), 0, 0, 0xb, COLOUR_NOTICE, 1, 0);
}

void ui_rodex_inbox_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRodexInboxWnd *self = (struct UIRodexInboxWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down(WND(self), x, y);
}

void ui_rodex_inbox_wnd_on_mouse_hover(struct UIWindow *self, int x, int y)
{
}

void ui_rodex_inbox_wnd_on_process(struct UIWindow *self)
{
}

void ui_rodex_inbox_wnd_on_r_btn_down(struct UIWindow *self, int x, int y)
{
}

static void fill_list(struct Vec *list, int type)
{
    struct Tree *map = &g_rodex->tabs[type];
    struct TreeNode *node;
    struct MailRef ref;

    for (node = tree_first(map); node != map->head; node = tree_next(node)) {
        ref.type = type;
        ref.id_align = 0;
        ref.id = TREE_VALUE(node, struct MailPair)->id;
        vec_push(list, &ref, sizeof ref);
    }
    std_sort_mailrefs(list->first, list->last, (int)ref_count(list));
}

void ui_rodex_inbox_wnd_rebuild_lists(struct UIRodexInboxWnd *self)
{
    self->lists[0].last = self->lists[0].first;
    self->lists[2].last = self->lists[2].first;
    self->lists[1].last = self->lists[1].first;
    fill_list(&self->lists[0], 0);
    fill_list(&self->lists[2], 2);
    fill_list(&self->lists[1], 1);
}

void rodex_inbox_wnd_remove(struct UIRodexInboxWnd *self, int mail_id)
{
    struct MailRef *ref = self->lists[0].first, *end = self->lists[0].last;

    for (; ref != end; ref++) {
        if (ref->id == (long long)mail_id) {
            for (; ref + 1 != end; ref++)
                *ref = ref[1];
            self->lists[0].last = end - 1;
            return;
        }
    }
}

static void search(struct UIRodexInboxWnd *self, struct String text, int by_title)
{
    int type;

    self->hits.last = self->hits.first;
    for (type = 0; type < 3; type++) {
        struct Tree *map = &g_rodex->tabs[type];
        struct TreeNode *node;

        for (node = tree_first(map); node != map->head; node = tree_next(node)) {
            const struct MailPair *it = TREE_VALUE(node, const struct MailPair);
            const struct String *field = by_title ? &it->mail.title : &it->mail.sender;

            if (text_view_find(string_view(field), string_view(&text), 0) != STRING_NPOS) {
                struct MailRef ref;

                ref.type = type;
                ref.id_align = 0;
                ref.id = it->id;
                vec_push(&self->hits, &ref, sizeof ref);
            }
        }
    }
    std_sort_mailrefs(self->hits.first, self->hits.last, (int)ref_count(&self->hits));
    string_free(&text);
}

void ui_rodex_inbox_wnd_search_sender(struct UIRodexInboxWnd *self, struct String text)
{
    search(self, text, 0);
}

void ui_rodex_inbox_wnd_search_title(struct UIRodexInboxWnd *self, struct String text)
{
    search(self, text, 1);
}

static void set_caption(struct UIBasicButton *btn, struct String text)
{
    string_assign(&btn->label, string_cstr(&text), text.size);
    ui_window_invalidate_base(BTN(btn));
    string_free(&text);
}

static void set_caption_msg(struct UIBasicButton *btn, int msg, int id)
{
    struct String text;

    string_init(&text);
    string_assign_cstr(&text, msg_str(msg));
    set_caption(btn, text);
    BTN(btn)->id = id;
}

void ui_rodex_inbox_wnd_reset_to_first_page(struct UIRodexInboxWnd *self)
{
    self->page = 0;
    uncheck_all(self);
    set_caption_msg(self->btn_delete, MSG_DELETE_ALL, ID_DELETE_ALL);
    set_caption_msg(self->btn_take, MSG_TAKE_ALL, ID_TAKE_ALL);
    ui_window_invalidate_base(WND(self));
}

static void set_disabled(struct UIButton *btn, BYTE disabled)
{
    btn->disabled = disabled;
    ui_window_invalidate_base(&btn->base);
    ui_window_invalidate_base(&btn->base);
}

void ui_rodex_inbox_wnd_set_buttons_disabled(struct UIRodexInboxWnd *self, BYTE disabled)
{
    set_disabled(&self->btn_prev->button, disabled);
    set_disabled(&self->btn_next->button, disabled);
    set_disabled(&self->btn_refresh->button, disabled);
    set_disabled(&self->btn_delete->button, disabled);
    set_disabled(&self->btn_write->button, disabled);
    set_disabled(&self->btn_take->button, disabled);
    ui_window_invalidate_base(WND(self));
}

void ui_rodex_inbox_wnd_next_page(struct UIRodexInboxWnd *self)
{
    self->page++;
    uncheck_all(self);
    ui_window_invalidate_base(WND(self));
}

static void take_mail(struct UIRodexInboxWnd *self, int type, long long id)
{
    struct PacketRodexTake packet;
    struct Tree *map = tab_map(type);
    struct MailPair *node;
    BYTE flags = 0;

    if (map && (node = find_mail(map, id)))
        flags = node->mail.type;
    packet.mail_id = id;
    packet.open_type = (BYTE)type;
    if (flags & MAIL_HAS_ITEMS) {
        packet.id = PACKET_CZ_REQ_ITEM_FROM_RODEX;
        rodex_send(PACKET_CZ_REQ_ITEM_FROM_RODEX, &packet);
    }
    if (flags & MAIL_HAS_ZENY) {
        packet.id = PACKET_CZ_REQ_ZENY_FROM_RODEX;
        rodex_send(PACKET_CZ_REQ_ZENY_FROM_RODEX, &packet);
    }
}

static void take_known(struct UIRodexInboxWnd *self, const struct MailRef *ref)
{
    if (ref->type == 0) {
        struct MailPair *node = find_mail(&g_rodex->tabs[0], ref->id);

        if (node && node->mail.sender_trusted <= 0)
            return;
    }
    take_mail(self, ref->type, ref->id);
}

void ui_rodex_inbox_wnd_take_all(struct UIRodexInboxWnd *self)
{
    if (self->searching == 1) {
        struct MailRef *ref;

        for (ref = self->hits.first; ref != self->hits.last; ref++)
            take_known(self, ref);
    } else {
        int type = g_rodex->open_type;
        struct Tree *map = tab_map(type);
        struct TreeNode *node;

        if (!map)
            return;
        for (node = tree_first(map); node != map->head; node = tree_next(node)) {
            const struct MailPair *it = TREE_VALUE(node, const struct MailPair);

            if (it->mail.sender_trusted > 0)
                take_mail(self, type, it->id);
        }
    }
}

static BYTE delete_mail(struct UIRodexInboxWnd *self, int type, long long id)
{
    struct Tree *map = tab_map(type);
    struct MailPair *node;

    if (!map || !(node = find_mail(map, id)) || (node->mail.type & (MAIL_HAS_ZENY | MAIL_HAS_ITEMS)))
        return 0;
    game_send_message(MODE_RODEX_DELETE, (int)id, type, 0, 0);
    if (g_windowMgr.rodex_contents_wnd)
        ui_close_window(WID_RODEX_CONTENTS);
    return 1;
}

static void delete_all(struct UIRodexInboxWnd *self)
{
    int deleted = 0, kept = 0;

    if (self->searching == 1) {
        struct MailRef *ref;

        for (ref = self->hits.first; ref != self->hits.last; ref++) {
            struct Tree *map = tab_map(ref->type);
            struct MailPair *node;

            if (!map || !(node = find_mail(map, ref->id)))
                continue;
            if (node->mail.type & (MAIL_HAS_ZENY | MAIL_HAS_ITEMS)) {
                kept++;
                continue;
            }
            game_send_message(MODE_RODEX_DELETE, (int)ref->id,
                          ref->type, 0, 0);
            deleted++;
        }
    } else {
        int type = g_rodex->open_type;
        struct Tree *map = tab_map(type);
        struct TreeNode *node;

        if (!map)
            return;
        for (node = tree_first(map); node != map->head; node = tree_next(node)) {
            const struct MailPair *it = TREE_VALUE(node, const struct MailPair);

            if (it->mail.type & (MAIL_HAS_ZENY | MAIL_HAS_ITEMS)) {
                kept++;
                continue;
            }
            game_send_message(MODE_RODEX_DELETE, (int)it->id, type, 0,
                          0);
            deleted++;
        }
    }
    if (deleted > 0 && g_windowMgr.rodex_contents_wnd)
        ui_close_window(WID_RODEX_CONTENTS);
    if (kept > 0)
        rodex_chat(MSG_TAKE_FIRST);
}

static int row_checked(const struct UIRodexInboxWnd *self, int row)
{
    const struct UICheckBox *box = self->checks[row];

    return box->base.visible == 1 && box->base.state != 0 && box->notice_value != -1;
}

static int ask(const char *text)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_QUESTION, 0, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78,
                                  0);
}

static int checked_refs(struct UIRodexInboxWnd *self, struct MailRef refs[ROWS])
{
    int row, n = 0;

    for (row = 0; row < ROWS; row++) {
        if (!row_checked(self, row))
            continue;
        if (self->searching == 1) {
            refs[n++] = ((struct MailRef *)self->hits.first)[self->page * ROWS + row];
        } else {
            refs[n].type = g_rodex->open_type;
            refs[n].id_align = 0;
            refs[n++].id = self->checks[row]->notice_value;
        }
    }
    return n;
}

static void delete_checked(struct UIRodexInboxWnd *self)
{
    struct MailRef refs[ROWS];
    int n = checked_refs(self, refs), i;
    BYTE all = 1;

    if (n == 0 || ask(msg_str(MSG_DELETE_QUESTION)) != UI_MSGBOX_ANSWER_YES)
        return;
    uncheck_all(self);
    for (i = 0; i < n; i++)
        all &= delete_mail(self, refs[i].type, refs[i].id);
    if (!all)
        rodex_chat(MSG_TAKE_FIRST);
}

static void take_checked(struct UIRodexInboxWnd *self)
{
    struct MailRef refs[ROWS];
    int n = checked_refs(self, refs), i;

    if (n == 0 || ask(msg_str(MSG_TAKE_CHECKED_QUESTION)) != UI_MSGBOX_ANSWER_YES)
        return;
    for (i = 0; i < n; i++)
        take_mail(self, refs[i].type, refs[i].id);
    uncheck_all(self);
}

static void list_name(const struct UIRodexInboxWnd *self, struct String *out)
{
    string_init(out);
    if (self->searching == 1)
        string_assign_cstr(out, msg_str(MSG_TAB_SEARCH));
    else if (g_rodex->open_type == 0)
        string_assign_cstr(out, msg_str(MSG_TAB_CHARACTER));
    else if (g_rodex->open_type == 1)
        string_assign_cstr(out, msg_str(MSG_TAB_ACCOUNT));
    else if (g_rodex->open_type == 2)
        string_assign_cstr(out, msg_str(MSG_TAB_RETURNED));
}

static void all_command(struct UIRodexInboxWnd *self, int question, int take)
{
    struct String name, text;

    list_name(self, &name);
    string_init(&text);
    string_format(&text, msg_str(question), string_cstr(&name));
    if (ask(string_cstr(&text)) == UI_MSGBOX_ANSWER_YES) {
        if (take)
            ui_rodex_inbox_wnd_take_all(self);
        else
            delete_all(self);
    }
    string_free(&text);
    string_free(&name);
}

static void checks_changed(struct UIRodexInboxWnd *self)
{
    int row, checked = 0;

    for (row = 0; row < ROWS; row++)
        if (self->checks[row]->base.state > 0)
            checked++;
    if (checked) {
        set_caption_msg(self->btn_delete, MSG_DELETE_CHECKED, ID_DELETE_CHECKED);
        set_caption_msg(self->btn_take, MSG_TAKE_CHECKED, ID_TAKE_CHECKED);
    } else {
        set_caption_msg(self->btn_delete, MSG_DELETE_ALL, ID_DELETE_ALL);
        set_caption_msg(self->btn_take, MSG_TAKE_ALL, ID_TAKE_ALL);
    }
}

static void radio_chosen(struct UIRodexInboxWnd *self, struct UIWindow *sender, int value)
{
    struct UIWindow *wnd = WND(self);

    if (value == ID_SEARCH) {
        int i;

        for (i = 0; i < 2; i++)
            ui_radio_btn_send_msg(&self->search_by[i]->base, wnd, UI_MSG_SET_STATE,
                     &self->search_by[i]->base == sender, 0, 0, 0);
        ui_window_invalidate_base(wnd);
        return;
    }
    ui_window_send_msg(self->tabs[0], wnd, UI_MSG_SET_STATE, 0, 0, 0, 0);
    ui_window_send_msg(self->tabs[1], wnd, UI_MSG_SET_STATE, 0, 0, 0, 0);
    ui_window_send_msg(self->tabs[2], wnd, UI_MSG_SET_STATE, 0, 0, 0, 0);
    ui_window_send_msg(self->tab_search, wnd, UI_MSG_SET_STATE, 0, 0, 0, 0);
    ui_window_send_msg(sender, wnd, UI_MSG_SET_STATE, 1, 0, 0, 0);
    uncheck_all(self);
    self->page = 0;
    if (self->tabs[0] == sender)
        g_rodex->open_type = 0;
    else if (self->tabs[1] == sender)
        g_rodex->open_type = 1;
    else if (self->tabs[2] == sender)
        g_rodex->open_type = 2;
    self->searching = self->tab_search == sender;
    ui_window_invalidate_base(wnd);
}

static void search_command(struct UIRodexInboxWnd *self)
{
    struct String text;

    if (self->search_by[0]->base.state == 1) {
        string_init(&text);
        string_assign_cstr(&text, ui_edit_ctrl_get_text(self->search_edit));
        ui_rodex_inbox_wnd_search_title(self, text);
    } else if (self->search_by[1]->base.state == 1) {
        string_init(&text);
        string_assign_cstr(&text, ui_edit_ctrl_get_text(self->search_edit));
        ui_rodex_inbox_wnd_search_sender(self, text);
    }
    ui_rodex_inbox_wnd_send_msg(WND(self), self->tab_search, UI_MSG_COMMAND, UI_ID_RADIO, 0, 0, 0);
    ui_window_invalidate_base(WND(self));
}

static void next_page(struct UIRodexInboxWnd *self)
{
    unsigned shown = (self->page + 1) * ROWS;

    if (self->searching == 1) {
        if (ref_count(&self->hits) > shown)
            ui_rodex_inbox_wnd_next_page(self);
    } else {
        struct Tree *map = tab_map(g_rodex->open_type);

        if (map && map->size > shown)
            ui_rodex_inbox_wnd_next_page(self);
    }
}

static void prev_page(struct UIRodexInboxWnd *self)
{
    if (--self->page < 0) {
        self->page = 0;
        ui_window_invalidate_base(WND(self));
        return;
    }
    uncheck_all(self);
    ui_window_invalidate_base(WND(self));
}

static void refresh(struct UIRodexInboxWnd *self)
{
    rodex_purge_deleted(g_rodex);
    uncheck_all(self);
    request_list(PACKET_CZ_REQ_REFRESH_MAIL);
    ui_rodex_inbox_wnd_set_buttons_disabled(self, 1);
}

static void return_mail(int mail_id)
{
    struct PacketRodexReturn packet;
    struct MailPair *node;

    if (g_rodex->open_type != 0 || !(node = find_mail(&g_rodex->tabs[0], mail_id)))
        return;
    packet.id = PACKET_CZ_RODEX_RETURN;
    packet.mail_id = (int)node->id;
    rodex_send(PACKET_CZ_RODEX_RETURN, &packet);
}

static void command(struct UIRodexInboxWnd *self, struct Mode *mode, struct UIWindow *sender, int id, int value)
{
    switch (id) {
    case UI_ID_CLOSE:
        ui_close_window(WID_RODEX_INBOX);
        break;
    case ID_DELETE_CHECKED:
        delete_checked(self);
        break;
    case UI_ID_CHECKBOX:
        checks_changed(self);
        break;
    case UI_ID_RADIO:
        radio_chosen(self, sender, value);
        break;
    case ID_WRITE:
        if (ui_window_mgr_is_barter_open(&g_windowMgr) != 1)
            mode_send_message(mode, MODE_RODEX_WRITE, 0, 0, 0, 0);
        break;
    case ID_RETURN:
        return_mail(value);
        break;
    case ID_PREV:
        prev_page(self);
        break;
    case ID_NEXT:
        next_page(self);
        break;
    case ID_SEARCH:
        search_command(self);
        break;
    case ID_REFRESH:
        refresh(self);
        break;
    case ID_DELETE_ALL:
        all_command(self, MSG_DELETE_ALL_QUESTION, 0);
        break;
    case ID_TAKE_CHECKED:
        take_checked(self);
        break;
    case ID_TAKE_ALL:
        all_command(self, MSG_TAKE_ALL_QUESTION, 1);
        break;
    }
}

static void text_button(struct Mode *mode, int id, int mail_id)
{
    struct MailPair *node = find_mail(&g_rodex->tabs[0], mail_id);

    if (id == ID_TITLE) {
        if (node && node->mail.sender_trusted == 0) {
            ui_chat_line(msg_str(MSG_UNKNOWN_SENDER), 0xff);
            if (ask(msg_str(MSG_UNKNOWN_SENDER)) == UI_MSGBOX_ANSWER_NO)
                return;
        }
        mode_send_message(mode, MODE_RODEX_OPEN, mail_id, g_rodex->open_type, 0, 0);
    } else if (id == ID_SENDER) {
        struct String nr = {0};

        if (g_rodex->open_type != 0 || !node || (node->mail.type & MAIL_FROM_SERVER) || !node->mail.sender.size)
            return;
        if (!navi_links_strip_nr(&nr, string_view(&node->mail.title)))
            mode_send_message(mode, MODE_RODEX_WRITE, (intptr_t)string_cstr(&node->mail.sender), 0, 0, 0);
        string_free(&nr);
    }
}

intptr_t ui_rodex_inbox_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                       intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIRodexInboxWnd *self = (struct UIRodexInboxWnd *)window;
    struct Mode *mode;

    if (g_windowMgr.rodex_compose_wnd || g_windowMgr.rodex_contents_wnd)
        return 0;
    mode = game_current_base_mode();
    if (!g_rodex->list_complete && (msg != UI_MSG_COMMAND || val1 != UI_ID_CLOSE))
        return 0;
    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        if (sender == self->search_edit)
            ui_rodex_inbox_wnd_send_msg(WND(self), BTN(self->btn_search), UI_MSG_COMMAND, ID_SEARCH, 0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
        command(self, mode, sender, val1, val2);
        return 0;
    case UIM_RODEX_REFRESH:
        ui_rodex_inbox_wnd_rebuild_lists(self);
        self->page = 0;
        uncheck_all(self);
        set_caption_msg(self->btn_delete, MSG_DELETE_ALL, ID_DELETE_ALL);
        set_caption_msg(self->btn_take, MSG_TAKE_ALL, ID_TAKE_ALL);
        ui_window_invalidate_base(WND(self));
        ui_rodex_inbox_wnd_set_buttons_disabled(self, 0);
        ui_window_invalidate_base(WND(self));
        return 0;
    case UIM_RODEX_INBOX_REFRESH:
        ui_window_invalidate_base(WND(self));
        return 0;
    case UI_MSG_TEXT_BUTTON:
        if (val1 == ID_TITLE || val1 == ID_SENDER)
            text_button(mode, val1, val2);
        return 0;
    default:
        return ui_frame_wnd_send_msg(WND(self), sender, msg, val1, val2, val3, val4);
    }
}
