#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include <mmsystem.h>
#include "core/msg_str.h"
#include "diag/perf.h"

#define UI_WND_ITEM_OBTAIN 0x3a
#define UI_MSG_OBTAINED 0x24
#define UI_MSG_OBTAIN_IDENTIFIED 0x4c
#define OBTAIN_SHOW_MS 5000

#define MSG_OBTAIN_ZENY 0x147
#define MSG_OBTAIN_ITEM 0x99
#define MSG_OBTAIN_AMOUNT 0x2b8

#define OBTAIN_STEP 28
#define OBTAIN_MIN_WIDTH 0x22

struct UINotifyItemObtainWnd {
    struct UIFrameWnd frame;
    struct String icon_id;
    struct String message;
    DWORD shown_at;
    int zeny;
    struct ItemInfo info;
    BYTE identified;
};

struct UIWindow;
void ui_notify_item_obtain_wnd_destroy(struct UIWindow *window);
BYTE ui_notify_item_obtain_wnd_is_hit_testable(struct UIWindow *self);
void ui_notify_item_obtain_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_notify_item_obtain_wnd_on_process(struct UIWindow *self);
void ui_notify_item_obtain_wnd_on_draw(struct UIWindow *self);
int ui_notify_item_obtain_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_notify_item_obtain_wnd_ops = {
    .destroy = ui_notify_item_obtain_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_notify_item_obtain_wnd_is_hit_testable,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_notify_item_obtain_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_notify_item_obtain_wnd_on_process,
    .on_draw = ui_notify_item_obtain_wnd_on_draw,
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
    .send_msg = ui_notify_item_obtain_wnd_send_msg,
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

struct UINotifyItemObtainWnd *ui_notify_item_obtain_wnd_construct(struct UINotifyItemObtainWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_notify_item_obtain_wnd_ops;
    string_init(&self->icon_id);
    string_init(&self->message);
    item_info_construct(&self->info);
    self->identified = 0;
    return self;
}

struct UIWindow *ui_notify_item_obtain_wnd_new(void)
{
    return (struct UIWindow *)ui_notify_item_obtain_wnd_construct(mem_alloc(sizeof(struct UINotifyItemObtainWnd)));
}

void ui_notify_item_obtain_wnd_destruct(struct UINotifyItemObtainWnd *self)
{
    self->frame.base.ops = &ui_notify_item_obtain_wnd_ops;
    item_info_destruct(&self->info);
    string_free(&self->message);
    string_free(&self->icon_id);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_notify_item_obtain_wnd_destroy(struct UIWindow *window)
{
    struct UINotifyItemObtainWnd *self = (struct UINotifyItemObtainWnd *)window;
    ui_notify_item_obtain_wnd_destruct(self);
    mem_free(self);
}

BYTE ui_notify_item_obtain_wnd_is_hit_testable(struct UIWindow *self)
{
    return 0;
}

void ui_notify_item_obtain_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

void ui_notify_item_obtain_wnd_on_process(struct UIWindow *window)
{
    struct UINotifyItemObtainWnd *self = (struct UINotifyItemObtainWnd *)window;
    if (timeGetTime() > self->shown_at + OBTAIN_SHOW_MS)
        ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_ITEM_OBTAIN);
}

void ui_notify_item_obtain_wnd_on_draw(struct UIWindow *window)
{
    struct UINotifyItemObtainWnd *self = (struct UINotifyItemObtainWnd *)window;
    struct ItemNameRuns runs = {0};
    int zeny, max_width;

    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
    if (self->icon_id.size != 0)
        ui_window_draw_item_icon_by_name(&self->frame.base, 0xd, 5, string_cstr(&self->icon_id),
                                    self->identified);
    zeny = self->zeny;
    max_width = self->frame.base.width - 0x31;
    item_info_name_runs(&self->frame.base, &self->info, &runs, 1, 0);
    string_append_format(&runs.text, msg_str(MSG_OBTAIN_AMOUNT), self->info.count);
    item_name_runs_end(&runs);
    ui_window_draw_runs_shortened(&self->frame.base, zeny == 0 ? 0x29 : 0xd, 0xa, max_width, &runs, 0, 12);
    string_free(&runs.text);
}

static void obtain_notice(struct UINotifyItemObtainWnd *self, int zeny, int amount, const char *id_text)
{
    struct String line = {0};
    int width;

    self->zeny = zeny;
    if (zeny) {
        char money[0x20];

        string_format(&line, msg_str(MSG_OBTAIN_ZENY), format_money(amount, money));
        string_assign(&self->message, string_cstr(&line), line.size);
        string_clear(&self->icon_id);
    } else {
        struct ItemInfo info;
        struct String name = {0};

        string_assign_cstr(&self->icon_id, id_text);
        item_info_construct(&info);
        string_assign_cstr(&info.id_text, id_text);
        info.identified = self->identified;
        info.refine = 0;
        item_info_get_display_name(&info, &name);
        string_format(&line, msg_str(MSG_OBTAIN_ITEM), string_cstr(&name), amount);
        string_free(&name);
        string_assign(&self->message, string_cstr(&line), line.size);
        item_info_destruct(&info);
    }
    string_free(&line);
    width = ui_window_text_width(&self->frame.base, string_cstr(&self->message), 0, 0, 12, 0, 0) + 0x36;
    if (width != width / OBTAIN_STEP * OBTAIN_STEP)
        width += OBTAIN_STEP;
    if (width < OBTAIN_MIN_WIDTH)
        width = OBTAIN_MIN_WIDTH;
    if (width != self->frame.base.width)
        ui_window_resize_base(&self->frame.base, width, self->frame.base.height);
    ui_window_invalidate_base(&self->frame.base);
    self->shown_at = timeGetTime();
}

int ui_notify_item_obtain_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UINotifyItemObtainWnd *self = (struct UINotifyItemObtainWnd *)window;
    switch (msg) {
    case UI_MSG_SET_INFO:
        item_info_assign(&self->info, (const struct ItemInfo *)val1);
        return 0;
    case UI_MSG_OBTAINED:
        obtain_notice(self, val1, val2, (const char *)val3);
        return 0;
    case UI_MSG_OBTAIN_IDENTIFIED:
        self->identified = val1 != 0;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
