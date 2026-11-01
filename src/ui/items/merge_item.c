#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "contents/connection_api.h"
#include "ui/items/items.h"
#include <math.h>
#include <stdlib.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "std/vec.h"

#define UI_WND_MERGE_ITEM 0xe3

#define UI_MSG_MERGE_CLICK 2
#define UI_MSG_RESIZER 0xe
#define UI_MSG_MERGE_COUNT 0x3c
#define PACKET_MERGE_ITEMS 0x96e
#define PACKET_MERGE_CANCEL 0x974
#define MERGE_MIN_ITEMS 2

#define MSG_MERGE_TITLE 0x879
#define MSG_MERGE_TOO_FEW 0x87a
#define MSG_MERGE_TOOLTIP 0xb7
#define MERGE_REFUSAL_COLOUR 0xff

#define SLOT 35
#define GRID_LEFT 10
#define GRID_TOP 20
#define TITLE_HEIGHT 0x11

struct MergeItemWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *ok;
    struct UIBitmapButton *cancel;
    struct UIWindow *close;
    struct Vec checks;
    struct UIResizer *resizer;
    struct UIScrollBar *scroll;
    int count;
    int columns;
    int rows;
    int cells;
    int top_row;
    int max_row;
};
OFFSET_ASSERT(MergeItemWnd, checks, 0xa0);
OFFSET_ASSERT(MergeItemWnd, count, 0xb4);
OFFSET_ASSERT(MergeItemWnd, max_row, 0xc8);
SIZE_ASSERT(MergeItemWnd, 0xcc);

void merge_item_wnd_destroy(struct UIWindow *window);
void merge_item_wnd_resize(struct UIWindow *self, int width, int height);
void merge_item_wnd_on_create(struct UIWindow *self, int width, int height);
void merge_item_wnd_on_process(struct UIWindow *self);
void merge_item_wnd_on_draw(struct UIWindow *self);
void merge_item_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void merge_item_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void merge_item_wnd_on_wheel(struct UIWindow *self, int delta);
int merge_item_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps merge_item_wnd_ops = {
    .destroy = merge_item_wnd_destroy,
    .resize = merge_item_wnd_resize,
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
    .on_create = merge_item_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = merge_item_wnd_on_process,
    .on_draw = merge_item_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = merge_item_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = merge_item_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = merge_item_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = merge_item_wnd_send_msg,
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

#define CHECK_AT(self, i) (((struct UICheckBox **)(self)->checks.first)[i])
#define CHECK_COUNT(self) VEC_COUNT(&(self)->checks, struct UICheckBox *)

struct MergeItemWnd *merge_item_wnd_construct(struct MergeItemWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &merge_item_wnd_ops;
    self->ok = NULL;
    self->cancel = NULL;
    self->close = NULL;
    self->checks.first = NULL;
    self->checks.last = NULL;
    self->checks.end = NULL;
    self->resizer = NULL;
    self->scroll = NULL;
    self->count = 0;
    self->columns = 0;
    self->rows = 0;
    self->cells = 0;
    self->top_row = 0;
    self->max_row = 0;
    return self;
}

struct UIWindow *merge_item_wnd_new(void)
{
    return (struct UIWindow *)merge_item_wnd_construct(mem_alloc(sizeof(struct MergeItemWnd)));
}

void merge_item_wnd_destruct(struct MergeItemWnd *self)
{
    self->frame.base.ops = &merge_item_wnd_ops;
    vec_free(&self->checks);
    ui_frame_wnd_destruct(&self->frame);
}

void merge_item_wnd_destroy(struct UIWindow *window)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    merge_item_wnd_destruct(self);
    mem_free(self);
}

void merge_item_wnd_update_scroll(struct MergeItemWnd *self)
{
    int total, max;

    if (!self->scroll)
        return;
    total = self->count / ((self->frame.base.width - 20) / SLOT);
    total = total > 0 ? total + 1 : total;
    max = total - self->rows;
    if (max > 0) {
        if (self->top_row < 0)
            self->top_row = 0;
        if (self->top_row > max)
            self->top_row = max;
    } else {
        self->top_row = 0;
        max = 0;
    }
    self->max_row = max;
    ui_scroll_bar_set_range(self->scroll, max, self->rows, total);
    ui_scroll_bar_set_pos(self->scroll, self->top_row);
    ui_window_invalidate_base(&self->scroll->base);
}

void merge_item_wnd_snap_size(struct MergeItemWnd *self, int width, int height)
{
    int w, h;

    if (height <= 0)
        return;
    w = (width - 20) / SLOT * SLOT + 10;
    if (w < 0xb9)
        w = 0xb9;
    else if (w > 0x168)
        w = 0x168;
    h = (height - 50) / SLOT * SLOT + 20;
    if (h < 0x7d)
        h = 0x7d;
    else if (h > 0xe6)
        h = 0xe6;
    if (w != self->frame.base.width || h != self->frame.base.height) {
        merge_item_wnd_resize(&self->frame.base, w + 10, h + 0x1e);
        self->columns = (self->frame.base.width - 20) / SLOT;
        self->rows = (self->frame.base.height - 50) / SLOT;
        self->cells = self->rows * self->columns;
    }
    ui_window_mgr_update_snap_rect(&g_windowMgr, &self->frame.base);
}

void merge_item_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    ui_window_resize_base(&self->frame.base, width, height);
    ui_window_move_base(&self->ok->button.base, width - 0x6e, height - 0x19);
    ui_window_move_base(&self->cancel->button.base, width - 0x3c, height - 0x19);
    ui_window_move_base(self->close, width - 0xe, 3);
    ui_window_move_base(&self->resizer->base, width - 0xf, height - 0xf);
    ui_window_move_base(&self->scroll->base, width - 0xf, TITLE_HEIGHT);
    ui_scroll_bar_resize(&self->scroll->base, self->scroll->base.width, self->frame.base.height - 0x26);
    merge_item_wnd_update_scroll(self);
}

static struct UIBitmapButton *skin_button(struct UIWindow *parent, const char *const bitmaps[3], int id)
{
    struct UIBitmapButton *btn = mem_alloc(sizeof(struct UIBitmapButton));
    int state;

    if (btn)
        btn = ui_bitmap_button_construct(btn);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
        ui_bitmap_button_set_bitmap(btn, bitmaps[state], state, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    btn->button.base.id = id;
    ui_window_add_child(parent, &btn->button.base);
    return btn;
}

void merge_item_wnd_make_controls(struct MergeItemWnd *self)
{
    static const char *const ok[3] = { UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp" };
    static const char *const cancel[3] = { UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp",
                                           UI_SKIN "btn_cancel_b.bmp" };
    struct UIResizer *resizer;
    struct UIScrollBar *scroll;

    self->ok = skin_button(&self->frame.base, ok, UI_ID_OK);
    self->cancel = skin_button(&self->frame.base, cancel, UI_ID_CANCEL);
    self->close = ui_window_add_close_button(&self->frame.base, "", 0xf0);

    resizer = mem_alloc(sizeof(struct UIResizer));
    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->resizer = resizer;
    ui_window_create((struct UIWindow *)resizer, 0x10, 0x10);
    ui_resizer_set_edge(self->resizer, UI_EDGE_BOTTOM_RIGHT);
    ui_resizer_set_bitmap(self->resizer, UI_SKIN "btn_resize.bmp");
    ui_resizer_set_colours(self->resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(&self->frame.base, (struct UIWindow *)self->resizer);

    scroll = mem_alloc(sizeof(struct UIScrollBar));
    if (scroll)
        scroll = ui_scroll_bar_construct(scroll);
    self->scroll = scroll;
    ui_scroll_bar_create(scroll, 1, 1, 1, 1);
    ui_window_add_child(&self->frame.base, (struct UIWindow *)self->scroll);
}

void merge_item_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    merge_item_wnd_make_controls(self);
    merge_item_wnd_snap_size(self, self->frame.base.width, self->frame.base.height);
}

void merge_item_wnd_on_process(struct UIWindow *self)
{
}

static void place_items(struct MergeItemWnd *self)
{
    int first = self->top_row * self->columns;
    size_t k;
    int i;

    for (k = 0; k < CHECK_COUNT(self); k++)
        ui_check_box_set_hidden(&CHECK_AT(self, k)->base, 1);
    for (i = 0; i < self->cells && first + i < self->count; i++) {
        int cols = (self->frame.base.width - 20) / SLOT;
        int row_y = i / cols * SLOT;
        int x = i % cols * SLOT + GRID_LEFT;
        struct UICheckBox *check = CHECK_AT(self, first + i);
        struct ItemInfo item;

        if (!check)
            continue;
        session_get_inventory_item(&g_session1, &item, check->notice_value);
        if (item.index != 0) {
            ui_check_box_set_hidden(&check->base, 0);
            ui_window_move_base(&check->base, x, row_y + 0x28);
            ui_window_draw_item_icon_count(&self->frame.base, x + 5, row_y + 0x16, &item, 0, 0);
        }
        item_info_destruct(&item);
    }
}

void merge_item_wnd_draw_cells(struct MergeItemWnd *self)
{
    int i;

    for (i = 0; i < self->cells; i++) {
        int cols = (self->frame.base.width - 20) / SLOT;

        ui_window_draw_item_win_mid(&self->frame.base, i % cols * SLOT + GRID_LEFT, i / cols * SLOT + GRID_TOP);
    }
}

void merge_item_wnd_on_draw(struct UIWindow *window)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    COLORREF back = ui_skin_colour(2, 2);

    ui_window_draw_title_bar(&self->frame.base, 1, msg_str(MSG_MERGE_TITLE), 0);
    ui_window_fill_rect(&self->frame.base, 0, TITLE_HEIGHT, self->frame.base.width, self->frame.base.height,
                      back);
    merge_item_wnd_draw_cells(self);
    place_items(self);
    ui_window_draw_button_bar(&self->frame.base, 2, 0, 1);
}

static int item_at(struct MergeItemWnd *self, int x, int y)
{
    int col = (x - 10) / SLOT;
    int row = (y - 20) / SLOT;
    int idx = (self->top_row + row) * self->columns + col;
    int dx, dy;

    if (idx >= self->count)
        return -1;
    dx = x - col * SLOT - 0x19;
    dy = y - row * SLOT - 0x20;
    if (!(sqrtf((float)(dy * dy + dx * dx)) < 10.0f))
        return -1;
    if (!CHECK_AT(self, idx))
        return -1;
    return CHECK_AT(self, idx)->notice_value;
}

static int item_id_at(int index)
{
    struct ItemInfo item;
    int id;

    session_get_inventory_item(&g_session1, &item, index);
    id = atoi(string_cstr(&item.id_text));
    item_info_destruct(&item);
    return id;
}

static int check_item_id(const struct UICheckBox *check)
{
    return item_id_at(check->notice_value);
}

static void click_follow_kind(struct MergeItemWnd *self, int index)
{
    int kind, mark = 0;
    size_t k;

    if (!((GetAsyncKeyState(VK_CONTROL) >> 8) & 0xff))
        return;
    kind = item_id_at(index);
    for (k = 0; k < CHECK_COUNT(self); k++) {
        struct UICheckBox *check = CHECK_AT(self, k);

        if (check_item_id(check) != kind)
            continue;
        if (check->notice_value == index ? check->base.state != 0 : check->base.state == 0) {
            mark = 1;
            break;
        }
    }
    for (k = 0; k < CHECK_COUNT(self); k++) {
        struct UICheckBox *check = CHECK_AT(self, k);

        ui_check_box_set_check(&check->base, check_item_id(check) == kind ? mark : 0);
    }
}

void merge_item_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    int index;
    size_t k;

    if (y < TITLE_HEIGHT) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    index = item_at(self, x, y);
    for (k = 0; k < CHECK_COUNT(self); k++) {
        struct UICheckBox *check = CHECK_AT(self, k);

        if (index == check->notice_value) {
            ui_check_box_set_check(&check->base, check->base.state == 0);
            break;
        }
    }
    click_follow_kind(self, index);
}

void merge_item_wnd_show_tooltip(struct MergeItemWnd *self, int x, int y, int index)
{
    struct ItemInfo item;

    session_get_inventory_item(&g_session1, &item, index);
    if (item.index != 0) {
        struct String text;
        struct String name = {0};

        ui_window_to_screen(&self->frame.base, &x, &y);
        item_info_get_display_name(&item, &name);
        string_init(&text);
        string_format(&text, msg_str(MSG_MERGE_TOOLTIP), string_cstr(&name), item.count);
        string_free(&name);
        mode_show_tooltip(g_modeMgr.curMode, string_cstr(&text), x, y,
                          session_get_item_dis_color(&item), 0);
        string_free(&text);
    }
    item_info_destruct(&item);
}

void merge_item_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    merge_item_wnd_show_tooltip(self, x, y, item_at(self, x, y));
}

void merge_item_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    int step;

    if (!self->max_row)
        return;
    step = self->rows > 3 ? 3 : self->rows - 1;
    if (delta >= 0)
        step = -step;
    merge_item_wnd_send_msg(&self->frame.base, (struct UIWindow *)self->scroll, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

void merge_item_wnd_set_items(struct MergeItemWnd *self, const short *indices, int count)
{
    int i;

    self->count = count;
    for (i = 0; i < self->count; i++) {
        struct UICheckBox *check = mem_alloc(sizeof(struct UICheckBox));

        if (check)
            check = ui_check_box_construct(&check->base);
        ui_window_create(&check->base, 0x78, 0xc);
        ui_check_box_set_bitmaps_fit(check, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp");
        check->notice_value = indices[i];
        ui_check_box_set_hidden(&check->base, 1);
        check->send_notice = 1;
        vec_push(&self->checks, &check, sizeof(struct UICheckBox *));
        ui_window_add_child(&self->frame.base, &check->base);
    }
    merge_item_wnd_snap_size(self, self->frame.base.width, self->frame.base.height);
}

BYTE merge_item_wnd_collect_ticked(struct MergeItemWnd *self, struct Vec *picked)
{
    size_t k;

    for (k = 0; k < CHECK_COUNT(self); k++) {
        struct UICheckBox *check = CHECK_AT(self, k);
        unsigned short index;

        if (!check || check->base.state == 0)
            continue;
        index = (unsigned short)check->notice_value;
        vec_push(picked, &index, sizeof(unsigned short));
    }
    if (VEC_COUNT(picked, unsigned short) < MERGE_MIN_ITEMS) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_MERGE_TOO_FEW), MERGE_REFUSAL_COLOUR, 0, 0);
        return 0;
    }
    return 1;
}

struct MergePacketHeader {
    unsigned short id;
    unsigned short len;
};

void merge_item_wnd_build_packet(const struct Vec *picked,
                                          struct Vec *packet)
{
    int header = PACKET_MERGE_ITEMS;

    vec_insert_range(packet, packet->last, &header, &header + 1, 1);
    vec_insert_range(packet, packet->last, picked->first,
                     (const char *)picked->first + VEC_COUNT(picked, unsigned short) * 2, 1);
}

static BYTE send_merge(struct MergeItemWnd *self)
{
    struct Vec picked = { NULL, NULL, NULL };
    BYTE sent = 0;

    if (merge_item_wnd_collect_ticked(self, &picked)) {
        struct Vec packet = { NULL, NULL, NULL };

        merge_item_wnd_build_packet(&picked, &packet);
        ((struct MergePacketHeader *)packet.first)->len = (unsigned short)((char *)packet.last - (char *)packet.first);
        rag_connection_send_packet(rag_connection_instance(), (int)((char *)packet.last - (char *)packet.first),
                                  packet.first);
        vec_free(&packet);
        sent = 1;
    }
    if (picked.first)
        mem_free(picked.first);
    return sent;
}

void merge_item_wnd_cancel(void)
{
    unsigned short id = PACKET_MERGE_CANCEL;

    rag_connection_send_packet(rag_connection_instance(),
                              rag_connection_get_packet_length(rag_connection_instance(), PACKET_MERGE_CANCEL), &id);
    ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_MERGE_ITEM);
}

void merge_item_wnd_set_count(int index, int count)
{
    struct ItemInfo item;

    session_get_inventory_item(&g_session1, &item, index);
    session_remove_inventory_item(&g_session1, item.index, item.count);
    item.count = count;
    session_add_inventory_item(&g_session1, &item, 1);
    item_info_destruct(&item);
}

int merge_item_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct MergeItemWnd *self = (struct MergeItemWnd *)window;
    switch (msg) {
    case UI_MSG_MERGE_CLICK:
        click_follow_kind(self, val1);
        break;

    case UI_MSG_COMMAND:
        if (val1 == UI_ID_OK) {
            send_merge(self);
        } else if (val1 == UI_ID_CANCEL || val1 == UI_ID_CLOSE) {
            merge_item_wnd_cancel();
        }
        break;

    case UI_MSG_SCROLL_V:
        if (val1 != 0) {
            self->top_row += val1;
            merge_item_wnd_update_scroll(self);
            ui_window_invalidate_base(&self->frame.base);
        }
        break;

    case UI_MSG_RESIZER:
        if (val1 == UI_EDGE_BOTTOM_RIGHT)
            merge_item_wnd_snap_size(self, val2, val3);
        break;

    case UI_MSG_SET_INFO:
        merge_item_wnd_set_items(self, (const short *)val1, val2);
        break;

    case UI_MSG_MERGE_COUNT:
        merge_item_wnd_set_count(val1, val2);
        break;
    }
    return 0;
}
