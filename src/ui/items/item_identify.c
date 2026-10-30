#include "ui/window_records.h"
#include "ui/window_types.h"
#include "render/rp_pool.h"
#include <stdlib.h>
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/items/item_list.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"

struct UIItemIdentifyWnd;
struct UIWindow;
void ui_item_identify_wnd_destroy(struct UIWindow *window);

const struct UIWindowOps ui_item_identify_wnd_ops = {
    .destroy = ui_item_identify_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_identify_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_identify_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_identify_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_identify_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_identify_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_item_identify_wnd_on_r_btn_up,
    .on_wheel = ui_item_identify_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_identify_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_item_identify_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIItemIdentifyWnd *ui_item_identify_wnd_construct(struct UIItemIdentifyWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_item_identify_wnd_ops;
    list_init(&self->items);
    string_init(&self->title);
    self->info = NULL;
    return self;
}

struct UIWindow *ui_item_identify_wnd_new(void)
{
    return (struct UIWindow *)ui_item_identify_wnd_construct(mem_alloc(sizeof(struct UIItemIdentifyWnd)));
}

void ui_item_identify_wnd_destruct(struct UIItemIdentifyWnd *self)
{
    self->frame.base.ops = &ui_item_identify_wnd_ops;
    string_free(&self->title);
    list_free(&self->items, item_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_item_identify_wnd_destroy(struct UIWindow *window)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    ui_item_identify_wnd_destruct(self);
    mem_free(self);
}

void item_list_update_scroll(struct UIItemIdentifyWnd *self)
{
    int total, excess;

    if (!self->scroll)
        return;
    total = (int)(self->items.size / (unsigned)self->columns) + (self->items.size % (unsigned)self->columns != 0);
    excess = total - self->rows;
    if (excess < 0)
        excess = 0;
    self->scrollable = excess != 0;
    if (excess) {
        ui_scroll_bar_set_range(self->scroll, excess, self->rows, total);
        if (self->first_row < 0)
            self->first_row = 0;
        if (self->first_row > total - self->rows)
            self->first_row = total - self->rows;
        ui_scroll_bar_set_pos(self->scroll, self->first_row);
    } else {
        self->first_row = 0;
        ui_scroll_bar_set_range(self->scroll, 0, self->rows, total);
    }
    ui_window_invalidate_base(&self->scroll->base);
}

void ui_item_identify_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIScrollBar *scroll;
    const char *title = msg_str(MSG_ITEM_IDENTIFY_TITLE);

    string_assign_cstr(&self->title, title);
    self->first_row = 0;
    self->selected = -1;
    self->columns = 1;
    self->rows = 4;
    self->scrollable = 0;
    scroll = mem_alloc(sizeof(struct UIScrollBar));
    self->scroll = scroll ? ui_scroll_bar_construct(scroll) : NULL;
    ui_scroll_bar_create(self->scroll, wnd->width - 0xf, LIST_TOP, 1, wnd->height - 0x37);
    ui_window_add_child(wnd, (struct UIWindow *)self->scroll);
    item_list_update_scroll(self);

    ui_add_skin_button(wnd, "btn_ok", wnd->width - 0x5c, wnd->height - 0x18, UI_ID_OK);
    UI_FRAME_DEFAULT_ID(self) = UI_ID_OK;
    ui_add_skin_button(wnd, "btn_cancel", wnd->width - 0x2e, wnd->height - 0x18, UI_ID_CANCEL);
}

void ui_item_identify_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ListNode *node;
    int title_height = ui_window_draw_title_bar(wnd, 0, string_cstr(&self->title), 0);
    int k = 0, drawn = 0;

    ui_window_draw_body(wnd, title_height, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    for (node = self->items.head->next; node != self->items.head; node = node->next, k++) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        int y;

        if (k < self->first_row * self->columns)
            continue;
        y = drawn / self->columns * LIST_ROW_HEIGHT + LIST_TOP;
        if (self->selected == k)
            ui_window_fill_rect(wnd, 5, y - 3, wnd->width - 0x18, 0x1e, ui_skin_colour(6, 2));
        ui_window_draw_item_icon(wnd, 8, y, item, 0);
        ui_window_draw_item_name(wnd, 0x28, y, item, 0, 0);
        if (item->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", item->grade);
            ui_window_draw_bitmap(wnd, 8, y + 0xc, res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
            string_free(&path);
        }
        if (++drawn == self->rows * self->columns)
            break;
    }
}

void ui_item_identify_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int old = self->selected;
    int hit = -1;

    if (y < 0x11) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (g_replayPlaying)
        return;
    if (x <= wnd->width - 0xe && x >= 4 && y >= LIST_TOP && y < self->rows * LIST_ROW_HEIGHT + LIST_TOP) {
        int row = (y - LIST_TOP) / LIST_ROW_HEIGHT;
        struct ListNode *node;
        int k = 0, drawn = 0;

        for (node = self->items.head->next; node != self->items.head; node = node->next, k++) {
            if (k < self->first_row * self->columns)
                continue;
            if (drawn == self->columns * row) {
                hit = k;
                break;
            }
            drawn++;
        }
    }
    self->selected = hit;
    if (old != hit)
        ui_window_invalidate_base(&self->frame.base);
}

void ui_item_identify_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    ui_item_identify_wnd_on_l_btn_down(&self->frame.base, x, y);
    ui_window_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, UI_ID_OK, 0, 0, 0);
}

void ui_item_identify_wnd_on_r_btn_up(struct UIWindow *self, int x_unused, int y_unused)
{
}

void ui_item_identify_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows > 3 ? 3 : self->rows - 1;
    if (delta >= 0)
        step = -step;
    ui_window_send_msg(&self->frame.base, (struct UIWindow *)self->scroll, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

void ui_item_identify_wnd_store_info(struct UIWindow *window)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

struct ItemInfo *ui_item_identify_wnd_get_item(struct UIItemIdentifyWnd *self, struct ItemInfo *out, int n,
                                                      int *found)
{
    struct ListNode *node;
    struct ItemInfo empty;
    int k = 0;

    *found = 1;
    for (node = self->items.head->next; node != self->items.head; node = node->next, k++) {
        if (k == n)
            return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
    }
    *found = 0;
    item_info_construct_with_arg(&empty);
    item_info_copy(out, &empty);
    item_info_destruct(&empty);
    return out;
}

int item_list_scroll(struct UIItemIdentifyWnd *self, int msg, int rows)
{
    int old = self->first_row;

    if (msg == UI_MSG_PAGE_UP)
        self->first_row = old - self->rows + 1;
    else if (msg == UI_MSG_PAGE_DOWN)
        self->first_row = old + self->rows - 1;
    else
        self->first_row = old + rows;
    if (old != self->first_row) {
        item_list_update_scroll(self);
        ui_window_invalidate_base(&self->frame.base);
    }
    return self->first_row - old;
}

static int item_list_info_usable(const struct WindowInfo *info)
{
    if (info->width != 200 || info->height != 200)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - 200)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - 200)
        return 0;
    return 1;
}

static void item_list_info_reset(struct WindowInfo *info)
{
    info->x = (g_renderer->width - 640) / 2 + 200;
    info->width = 200;
    info->height = 200;
    info->y = g_renderer->height * 200 / 480;
}

void item_list_set_info(struct UIItemIdentifyWnd *self, struct WindowInfo **slot, struct WindowInfo *info)
{
    *slot = info;
    if (!item_list_info_usable(info))
        item_list_info_reset(info);
    ui_frame_wnd_move(&self->frame.base, info->x, info->y);
}

void item_list_add_inventory_item(struct UIItemIdentifyWnd *self, int index)
{
    struct ItemInfo info;

    session_get_inventory_item(&g_session1, &info, index);
    item_info_copy(list_push_back(&self->items, NULL, sizeof(struct ItemInfo)), &info);
    ui_window_send_msg(&self->frame.base, NULL, UI_MSG_LIST_CHANGED, 0, 0, 0, 0);
    item_info_destruct(&info);
}

int item_list_packet_records(struct UIItemIdentifyWnd *self, struct ReplayBuffer *out,
                             int begin, int end, int tag, int packet_id)
{
    struct SnapshotWriter writer = {0};
    struct ListNode *node, *head;
    struct PACKET_ZC_ITEMIDENTIFY_LIST *packet;
    size_t count, size, at = 0;
    if (!self || !out || !(head = self->items.head) || self->items.size > (UINT16_MAX - 4) / 2)
        return 0;
    count = self->items.size;
    size = sizeof *packet + count * sizeof *packet->indexes;
    packet = malloc(size);
    if (!packet) return 0;
    packet->packet_type = (unsigned short)packet_id;
    packet->packet_length = (unsigned short)size;
    for (node = head->next; node != head; node = node->next) {
        if (at == count) { free(packet); return 0; }
        packet->indexes[at++] = (short)LIST_VALUE(node, struct ItemInfo)->index;
    }
    if (at != count) { free(packet); return 0; }
    snapshot_write(&writer, (uint16_t)begin, NULL, 0);
    snapshot_write(&writer, (uint16_t)tag, packet, size);
    snapshot_write(&writer, (uint16_t)end, NULL, 0);
    free(packet);
    return snapshot_finish(&writer, out);
}

int ui_item_identify_wnd_get_records(struct UIWindow *window, struct ReplayBuffer *out)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    return item_list_packet_records(self, out, REC_IDENTIFY_BEGIN, REC_IDENTIFY_END, REC_IDENTIFY_PACKET,
                                    PACKET_ITEM_IDENTIFY_LIST);
}

void item_list_load_packets(const char *data, unsigned size, int begin, int end, int tag,
                            void (*handler)(struct GameMode *, const unsigned char *))
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    struct GameMode *mode = game_current_mode();
    unsigned char *copy = NULL;
    size_t largest = 0;
    int used = ui_window_records_open(&reader, data, size, (uint16_t)begin);
    if (!mode || !handler || used < 0) return;
    while (snapshot_next(&reader, &record) == 1 && record.tag != end)
        if (record.tag == tag && record.value.size > largest) largest = record.value.size;
    if (largest && !(copy = malloc(largest))) return;
    snapshot_open(&reader, data, (size_t)used, (uint16_t)begin);
    while (snapshot_next(&reader, &record) == 1 && record.tag != end)
        if (record.tag == tag) {
            memcpy(copy, record.value.data, record.value.size);
            handler(mode, copy);
        }
    free(copy);
}

int ui_item_identify_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIItemIdentifyWnd *self = (struct UIItemIdentifyWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        ui_window_record_command(&self->frame.base, val1, val2, self->selected, val4);
        if (g_replayPlaying)
            self->selected = val3;
        if (val1 == UI_ID_OK) {
            if (self->selected != -1) {
                struct ItemInfo info;
                int found = val1;

                ui_item_identify_wnd_get_item(self, &info, self->selected, &found);
                if (found)
                    mode_send_message(g_modeMgr.curMode, GAME_REQ_IDENTIFY, info.index, 0, 0, 0);
                ui_window_mgr_post_quit1(&g_windowMgr, ITEM_IDENTIFY_WND_ID);
                item_info_destruct(&info);
            }
        } else if (val1 == UI_ID_CANCEL) {
            mode_send_message(g_modeMgr.curMode, GAME_REQ_IDENTIFY, -1, 0, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, ITEM_IDENTIFY_WND_ID);
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return item_list_scroll(self, msg, val1);
    case UI_MSG_LIST_ADD:
        item_list_add_inventory_item(self, val2);
        return 0;
    case UI_MSG_SET_INFO:
        item_list_set_info(self, &self->info, (struct WindowInfo *)val1);
        return 0;
    case UI_MSG_LIST_CHANGED:
        item_list_update_scroll(self);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    case UI_MSG_LIST_CLEAR:
        list_clear(&self->items, item_info_destroy);
        return 0;
    case UI_MSG_LOAD_RECORDS:
        {
            int record_bytes = ui_window_records_validate((const void *)val1, (size_t)(unsigned)val2);
            if (record_bytes <= 0) return 0;
            val2 = record_bytes;
        }
        item_list_load_packets((const char *)val1, (unsigned)val2, REC_IDENTIFY_BEGIN, REC_IDENTIFY_END,
                               REC_IDENTIFY_PACKET, game_mode_item_identify_list);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct ItemInfo *ui_item_identify_wnd_get_selected_item(struct UIItemIdentifyWnd *self, struct ItemInfo *out, int *found)
{
    return ui_item_identify_wnd_get_item(self, out, self->selected, found);
}
