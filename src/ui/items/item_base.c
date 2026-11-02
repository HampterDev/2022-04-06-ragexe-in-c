#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/items/items.h"
#include "ui/shops/shops.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"

void ui_item_base_wnd_destroy(struct UIWindow *window);
void ui_item_base_wnd_on_create(struct UIWindow *self, int width, int height);
const struct UIWindowOps ui_item_base_wnd_ops = {
    .destroy = ui_item_base_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_base_wnd_drag_and_drop,
    .store_info = ui_item_base_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_base_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_base_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_base_wnd_send_msg,
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

void ui_item_base_wnd_destroy(struct UIWindow *window)
{
    struct UIItemBaseWnd *self = (struct UIItemBaseWnd *)window;
    self->frame.base.ops = &ui_item_base_wnd_ops;
    list_free(&self->items, item_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

struct UIItemBaseWnd *ui_item_base_wnd_construct(struct UIItemBaseWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_item_base_wnd_ops;
    list_init(&self->items);
    self->resizer = NULL;
    self->scroll_bar = NULL;
    self->scroll = 0;
    self->scrollable = 0;
    self->show_counts = 1;
    self->button_bar = 0;
    self->close = NULL;
    memset(self->extra_buttons, 0, sizeof self->extra_buttons);
    self->scroll = 0;
    return self;
}

void ui_item_base_wnd_destruct(struct UIItemBaseWnd *self)
{
    self->frame.base.ops = &ui_item_base_wnd_ops;
    list_free(&self->items, item_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

#define GRID_LEFT 0x28
#define GRID_TOP 0x11
#define GRID_CELL 32
#define GRID_MARGIN 0x26
#define STRIP_HEIGHT 8

#define BASE_MIN_WIDTH 0xe8
#define BASE_MAX_WIDTH 0x140
#define BASE_MIN_HEIGHT 0x66
#define BASE_MAX_HEIGHT 0xf0
#define BASE_WIDTH_ORIGIN 0x118

#define ITEM_BASE_MSG_PAGE_BACK 3
#define ITEM_BASE_MSG_PAGE_FWD 4
#define ITEM_BASE_MSG_REFRESH 0x17

void ui_item_base_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

void ui_item_base_wnd_update_scroll(struct UIItemBaseWnd *self)
{
    unsigned total;
    int over;

    if (!self->scroll_bar)
        return;
    total = self->items.size / (unsigned)self->columns + (self->items.size % (unsigned)self->columns != 0);
    over = (int)total - self->rows;
    if (over < 0)
        over = 0;
    self->scrollable = over != 0;
    if (over) {
        ui_scroll_bar_set_range(self->scroll_bar, over, self->rows, total);
        if (self->scroll < 0)
            self->scroll = 0;
        if (self->scroll > (int)total - self->rows)
            self->scroll = total - self->rows;
        ui_scroll_bar_set_pos(self->scroll_bar, self->scroll);
    } else {
        self->scroll = 0;
        ui_scroll_bar_set_range(self->scroll_bar, 0, self->rows, total);
    }
    ui_window_invalidate_base(&self->scroll_bar->base);
}

void ui_item_base_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemBaseWnd *self = (struct UIItemBaseWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ListNode *node;
    COLORREF colour;
    int body, strips, i, y, x, row, column, rows, columns, index, cell;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == GRID_TOP)
        return;
    body = wnd->height - GRID_MARGIN;
    strips = body / STRIP_HEIGHT + (body % STRIP_HEIGHT != 0);
    for (i = 0, y = GRID_TOP; i < strips; i++, y += STRIP_HEIGHT)
        ui_window_draw_bitmap(wnd, 0, y,
                            res_mgr_get(g_resMgr(), ui_skin_path(UI_SKIN "basic_interface\\itemwin_left.bmp")),
                            0);
    body = wnd->height - GRID_MARGIN;
    strips = body / STRIP_HEIGHT + (body % STRIP_HEIGHT != 0);
    for (i = 0, y = GRID_TOP; i < strips; i++, y += STRIP_HEIGHT) {
        struct BitmapRes *right = res_mgr_get(g_resMgr(),
                                                ui_skin_path(UI_SKIN "basic_interface\\itemwin_right.bmp"));

        ui_window_draw_bitmap(wnd, wnd->width - right->width, y, right, 0);
    }
    colour = ui_skin_colour(2, 2);
    ui_window_fill_rect(wnd, 0x14, GRID_TOP, (wnd->width - 0x3c) % GRID_CELL, wnd->height - GRID_MARGIN, colour);
    body = wnd->height - GRID_MARGIN;
    ui_window_fill_rect(wnd, 0x14, body / GRID_CELL * GRID_CELL + GRID_TOP, wnd->width - 0x18, body % GRID_CELL,
                      colour);
    ui_window_draw_button_bar(wnd, self->button_bar ? 2 : 0, 0, 1);

    columns = (wnd->width - GRID_LEFT) / GRID_CELL;
    rows = (wnd->height - GRID_MARGIN) / GRID_CELL;
    for (row = 0, y = GRID_TOP; row < rows; row++, y += GRID_CELL)
        for (column = 0, x = GRID_LEFT; column < columns; column++, x += GRID_CELL)
            ui_window_draw_item_win_mid(wnd, x, y);

    cell = 0;
    for (node = self->items.head->next, index = 0; node != self->items.head; node = node->next, index++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);

        if (index < self->scroll * self->columns)
            continue;
        if (session_is_pet_egg_id(&g_session1, atoi(string_cstr(&info->id_text))) && info->damaged)
            continue;
        x = cell % self->columns * GRID_CELL;
        y = cell / self->columns * GRID_CELL;
        if (self->show_counts)
            ui_window_draw_item_icon_count(wnd, x + 0x2c, y + 0x15, info, 0, 0);
        else
            ui_window_draw_item_icon(wnd, x + 0x2c, y + 0x15, info, 0);
        if (++cell == self->rows * self->columns)
            break;
    }
}

int ui_item_base_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIItemBaseWnd *self = (struct UIItemBaseWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int old;

    switch (msg) {
    case ITEM_BASE_MSG_PAGE_BACK:
        if (self->scrollable == 1)
            ui_window_send_msg(wnd, &self->scroll_bar->base, UI_MSG_SCROLL_V, -self->rows, 0, 0, 0);
        return 0;
    case ITEM_BASE_MSG_PAGE_FWD:
        if (self->scrollable == 1)
            ui_window_send_msg(wnd, &self->scroll_bar->base, UI_MSG_SCROLL_V, self->rows, 0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
    case ITEM_BASE_MSG_REFRESH:
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old = self->scroll;
        if (msg == UI_MSG_PAGE_UP)
            self->scroll = old - self->rows + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->scroll = self->rows - 1 + old;
        else
            self->scroll = val1 + old;
        if (old != self->scroll) {
            ui_item_base_wnd_update_scroll(self);
            ui_window_invalidate_base(wnd);
        }
        return self->scroll - old;
    case UI_MSG_RESIZE: {
        int width, height, body;

        if (val1 != UI_EDGE_BOTTOM_RIGHT)
            return 0;
        width = val2 > BASE_MAX_WIDTH ? BASE_MAX_WIDTH : val2;
        height = val3 > BASE_MAX_HEIGHT ? BASE_MAX_HEIGHT : val3;
        if (width < BASE_MIN_WIDTH)
            width = BASE_MIN_WIDTH;
        if (height < BASE_MIN_HEIGHT)
            height = BASE_MIN_HEIGHT;
        body = (height - GRID_MARGIN) / GRID_CELL * GRID_CELL;
        height = body + GRID_MARGIN;
        width = (width - BASE_WIDTH_ORIGIN) / GRID_CELL * GRID_CELL + BASE_WIDTH_ORIGIN;
        if (width != wnd->width || height != wnd->height) {
            ui_window_resize_base(wnd, width, height);
            self->columns = (width - GRID_LEFT) / GRID_CELL;
            self->rows = body / GRID_CELL;
            ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
            ui_window_move_base(&self->scroll_bar->base, wnd->width - 0xf, GRID_TOP);
            ui_scroll_bar_resize(&self->scroll_bar->base, self->scroll_bar->base.width, wnd->height - GRID_MARGIN);
            ui_item_base_wnd_update_scroll(self);
        }
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_item_base_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    mode_send_message(client_current_mode(), MODE_MSG_DRAG_END, 0, 0, 0, 0);
}

void ui_item_base_wnd_store_info(struct UIWindow *self)
{
}

void ui_item_base_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIItemBaseWnd *self = (struct UIItemBaseWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows <= 3 ? self->rows - 1 : 3;
    if (delta >= 0)
        step = -step;
    ui_window_send_msg(&self->frame.base, &self->scroll_bar->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}
