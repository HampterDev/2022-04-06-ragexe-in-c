#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include <stdlib.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"

#define UI_MSG_PARAM_REFRESH 0x3c
#define UI_MSG_PARAM_ITEM 0x40
#define MSG_PARAM_FIRST 0x10d
#define MSG_PARAM_SECOND 0x10e
#define EQUIP_ENTRIES 8

#define MARK_UP_COLOUR RGB(0xff, 0, 0)
#define MARK_DOWN_COLOUR RGB(0, 0, 0xff)
#define MARK_SAME_COLOUR RGB(0x50, 0x50, 0x50)

struct UIItemParamChangeDisplayWnd {
    struct UIFrameWnd frame;
    int first_now;
    int first_after;
    int second_now;
    int second_after;
    int item_first, item_second;
    int worn_first, worn_second;
    struct String item_id;
    int identified;
    struct WindowInfo *info;
};
OFFSET_ASSERT(UIItemParamChangeDisplayWnd, item_first, 0xa4);
OFFSET_ASSERT(UIItemParamChangeDisplayWnd, item_id, 0xb4);

struct UIWindow;
void ui_item_param_change_display_wnd_destroy(struct UIWindow *window);
void ui_item_param_change_display_wnd_store_info(struct UIWindow *self);
void ui_item_param_change_display_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_param_change_display_wnd_on_draw(struct UIWindow *self);
int ui_item_param_change_display_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_item_param_change_display_wnd_ops = {
    .destroy = ui_item_param_change_display_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_param_change_display_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_param_change_display_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_param_change_display_wnd_on_draw,
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
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_param_change_display_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIItemParamChangeDisplayWnd *
ui_item_param_change_display_wnd_construct(struct UIItemParamChangeDisplayWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_item_param_change_display_wnd_ops;
    string_init(&self->item_id);
    self->item_first = 0;
    self->item_second = 0;
    self->worn_first = 0;
    self->worn_second = 0;
    self->first_after = 0;
    self->second_after = 0;
    self->first_now = 0;
    self->second_now = 0;
    self->info = NULL;
    self->identified = 0;
    string_assign_cstr(&self->item_id, "512");
    return self;
}

struct UIWindow *ui_item_param_change_display_wnd_new(void)
{
    return (struct UIWindow *)ui_item_param_change_display_wnd_construct(mem_alloc(sizeof(struct UIItemParamChangeDisplayWnd)));
}

void ui_item_param_change_display_wnd_destruct(struct UIItemParamChangeDisplayWnd *self)
{
    self->frame.base.ops = &ui_item_param_change_display_wnd_ops;
    string_free(&self->item_id);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_item_param_change_display_wnd_destroy(struct UIWindow *window)
{
    struct UIItemParamChangeDisplayWnd *self = (struct UIItemParamChangeDisplayWnd *)window;
    ui_item_param_change_display_wnd_destruct(self);
    mem_free(self);
}

void ui_item_param_change_display_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemParamChangeDisplayWnd *self = (struct UIItemParamChangeDisplayWnd *)window;
    ui_item_param_change_display_wnd_send_msg(&self->frame.base, NULL, UI_MSG_PARAM_REFRESH, 0, 0, 0, 0);
}

void ui_item_param_change_display_wnd_store_info(struct UIWindow *window)
{
    struct UIItemParamChangeDisplayWnd *self = (struct UIItemParamChangeDisplayWnd *)window;
    if (!self->info)
        return;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
    self->info->width = self->frame.base.width;
    self->info->height = self->frame.base.height;
}

static void draw_number(struct UIItemParamChangeDisplayWnd *self, int x, int value)
{
    char text[0x80];

    sprintf(text, "%d", value);
    ui_window_text_out(&self->frame.base, x, 0xa, text, 0, 0, 12, 0, 0, 0);
}

static void draw_label(struct UIItemParamChangeDisplayWnd *self, int x, int msg)
{
    char text[0x80];

    sprintf(text, msg_str(msg));
    ui_window_text_out(&self->frame.base, x, 0xa, text, 0, 0, 12, 0, 0, 0);
}

static void draw_mark(struct UIItemParamChangeDisplayWnd *self, int x, int now, int after)
{
    struct UIWindow *w = &self->frame.base;

    if (now < after) {
        ui_window_fill_triangle(w, x + 4, 0xb, x, 0x13, x + 8, 0x13, MARK_UP_COLOUR);
    } else if (now == after) {
        ui_window_fill_triangle(w, x, 0xd, x + 8, 0xd, x, 0x11, MARK_SAME_COLOUR);
        ui_window_fill_triangle(w, x + 8, 0xd, x, 0x11, x + 8, 0x11, MARK_SAME_COLOUR);
    } else {
        ui_window_fill_triangle(w, x, 0xb, x + 8, 0xb, x + 4, 0x13, MARK_DOWN_COLOUR);
    }
}

void ui_item_param_change_display_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemParamChangeDisplayWnd *self = (struct UIItemParamChangeDisplayWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
    draw_label(self, 0x14, MSG_PARAM_FIRST);
    draw_number(self, 0x46, self->first_now);
    draw_mark(self, 0x5a, self->first_now, self->first_after);
    draw_number(self, 0x6e, self->first_after);
    draw_label(self, 0x98, MSG_PARAM_SECOND);
    draw_number(self, 0xca, self->second_now);
    draw_mark(self, 0xde, self->second_now, self->second_after);
    draw_number(self, 0xf2, self->second_after);
}

static void sum_worn(struct UIItemParamChangeDisplayWnd *self)
{
    struct ItemInfo item;
    int seen = 0, n;

    self->first_now = 0;
    self->second_now = 0;
    for (n = 0; n < EQUIP_ENTRIES; n++) {
        session_get_equipped_item(&g_session1, &item, n);
        if (item.index != 0 && item.count == 1) {
            const struct ItemTableEntry *rec = item_table_get(atoi(string_cstr(&item.id_text)));

            if (seen & rec->slotTableValue) {
                seen |= rec->slotTableValue;
            } else {
                seen |= rec->slotTableValue;
                if (rec->params.count == 2) {
                    self->first_now += rec->params.data[0];
                    self->second_now += rec->params.data[1];
                }
            }
        }
        item_info_destruct(&item);
    }
}

static struct ItemInfo *worn_at(struct ItemInfo *out, int location)
{
    struct ItemInfo item;
    int n;

    if (location == -1) {
        item_info_construct_with_arg(out);
        return out;
    }
    for (n = 0; n < EQUIP_ENTRIES; n++) {
        session_get_equipped_item(&g_session1, &item, n);
        if (item.index != 0 && item.count == 1 && (item.equip_location & location)) {
            item_info_copy(out, &item);
            item_info_destruct(&item);
            return out;
        }
        item_info_destruct(&item);
    }
    item_info_construct_with_arg(out);
    return out;
}

static void sum_after(struct UIItemParamChangeDisplayWnd *self)
{
    struct ItemInfo worn;

    self->worn_first = 0;
    self->worn_second = 0;
    worn_at(&worn, item_table_get(atoi(string_cstr(&self->item_id)))->slotTableValue);
    if (worn.index != 0) {
        const struct ItemTableEntry *rec = item_table_get(atoi(string_cstr(&worn.id_text)));

        if (rec->params.count == 2) {
            self->worn_first = rec->params.data[0];
            self->worn_second = rec->params.data[1];
        }
    }
    item_info_destruct(&worn);
    self->first_after = self->item_first + self->first_now - self->worn_first;
    self->second_after = self->item_second + self->second_now - self->worn_second;
    ui_window_invalidate_base(&self->frame.base);
}

static int info_usable(const struct WindowInfo *info)
{
    return info->width == 0x118 && info->height == 0x22 && info->x >= 0 &&
           info->x <= g_renderer->width - 0x118 && info->y >= 0 &&
           info->y <= g_renderer->height - 0x22;
}

static void info_reset(struct WindowInfo *info)
{
    info->x = 0x118;
    info->y = 0xf6;
    info->width = 0x118;
    info->height = 0x22;
}

int ui_item_param_change_display_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                   int msg, int val1, int val2, int val3, int val4)
{
    struct UIItemParamChangeDisplayWnd *self = (struct UIItemParamChangeDisplayWnd *)window;
    switch (msg) {
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!info_usable(self->info))
            info_reset(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;

    case UI_MSG_PARAM_REFRESH:
        sum_worn(self);
        sum_after(self);
        return 0;

    case UI_MSG_PARAM_ITEM: {
        const char *id_text = (const char *)val1;
        const struct ItemTableEntry *rec;

        sum_worn(self);
        string_assign_cstr(&self->item_id, id_text);
        self->identified = val2;
        rec = item_table_get(atoi(string_cstr(&self->item_id)));
        if (rec->params.count == 2) {
            self->item_first = rec->params.data[0];
            self->item_second = rec->params.data[1];
        }
        sum_after(self);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
