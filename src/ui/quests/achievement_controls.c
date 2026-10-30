#include "ui/controls/listbox.h"
#include "ui/controls/scroll.h"
#include "ui/controls/text_viewer.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/quests/achievement_internal.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_names.h"

struct UIAchScrollBar;
struct UIWindow;
void ui_ach_scroll_bar_destroy(struct UIWindow *window);
void ui_ach_scroll_bar_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_ach_scroll_bar_ops = {
    .destroy = ui_ach_scroll_bar_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ach_scroll_bar_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIAchTV;
void ui_ach_tv_destroy(struct UIWindow *window);
void ui_ach_tv_on_draw(struct UIWindow *self);
void ui_ach_tv_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_ach_tv_clear(struct UIListBox *base_list);
const struct UIListBoxOps ui_ach_tv_ops = {
    .base = {
        .destroy = ui_ach_tv_destroy,
        .resize = ui_list_box_resize,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_window_set_show_base,
        .on_create = ui_text_viewer_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_ach_tv_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_ach_tv_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_text_viewer_ignore_click,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_text_viewer_ignore_click,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_text_viewer_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_ach_tv_clear,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

struct UICustomGageBar;
void ui_custom_gage_bar_destroy(struct UIWindow *window);
void ui_custom_gage_bar_on_draw(struct UIWindow *self);
void ui_custom_gage_bar_on_mouse_hover(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui_custom_gage_bar_ops = {
    .destroy = ui_custom_gage_bar_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_custom_gage_bar_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_custom_gage_bar_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIToolTipBitmapWnd;
void ui_tool_tip_bitmap_wnd_destroy(struct UIWindow *window);
void ui_tool_tip_bitmap_wnd_on_draw(struct UIWindow *self);
void ui_tool_tip_bitmap_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_tool_tip_bitmap_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui_tool_tip_bitmap_wnd_ops = {
    .destroy = ui_tool_tip_bitmap_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_tool_tip_bitmap_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_tool_tip_bitmap_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_tool_tip_bitmap_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ach_draw(struct UIWindow *wnd, int x, int y, const char *path, int keep_alpha)
{
    ui_window_draw_bitmap(wnd, x, y, ui_skin_bitmap(path), keep_alpha);
}

BYTE ach_has_reward(const struct AchievementInfo *info)
{
    return info->reward_buff > 0 || info->reward_title > 0 || info->reward_item > 0;
}

struct UIAchScrollBar *ui_ach_scroll_bar_construct(struct UIAchScrollBar *self)
{
    ui_scroll_bar_construct(&self->bar);
    self->bar.base.ops = &ui_ach_scroll_bar_ops;
    self->bar.button_size = 0xb;
    self->bar.thickness = 0xb;
    return self;
}

static void ui_ach_scroll_bar_destruct(struct UIAchScrollBar *self)
{
    self->bar.base.ops = &ui_ach_scroll_bar_ops;
    ui_scroll_bar_destruct(&self->bar);
}

void ui_ach_scroll_bar_destroy(struct UIWindow *window)
{
    struct UIAchScrollBar *self = (struct UIAchScrollBar *)window;
    ui_ach_scroll_bar_destruct(self);
    mem_free(self);
}

void ui_ach_scroll_bar_on_draw(struct UIWindow *window)
{
    struct UIAchScrollBar *self = (struct UIAchScrollBar *)window;
    struct UIScrollBar *bar = &self->bar;
    struct BitmapRes *mid;
    int i, y, bottom;

    ui_window_clear_canvas(&bar->base, UI_TRANSPARENT);
    if (!bar->max_pos)
        return;
    mid = ui_skin_bitmap(ACH_DIR "scroll_v_bg_mid.bmp");
    for (i = 0; i < bar->base.height / bar->thickness; i++)
        ui_window_draw_bitmap(&bar->base, 0, bar->thickness * i, mid, 0);
    ach_draw(&bar->base, 0, bar->base.height - bar->thickness, ACH_DIR "scroll_v_bg_down.bmp", 0);
    ach_draw(&bar->base, 0, 0, ACH_DIR "scroll_v_arrow_up.bmp", 1);
    ach_draw(&bar->base, 0, bar->base.height - bar->button_size, ACH_DIR "scroll_v_arrow_down.bmp", 1);
    y = ui_scroll_bar_thumb_pos(bar) + bar->button_size;
    ach_draw(&bar->base, 0, y, ACH_DIR "scroll_v_thumb_up.bmp", 1);
    bottom = y - 4 + ui_scroll_bar_get_thumb_size(bar);
    for (y += 4; y < bottom; y += 4)
        ach_draw(&bar->base, 0, y, ACH_DIR "scroll_v_thumb_mid.bmp", 1);
    ach_draw(&bar->base, 0, bottom, ACH_DIR "scroll_v_thumb_down.bmp", 1);
}

struct UIAchScrollBar *ach_new_scroll_bar(void)
{
    struct UIAchScrollBar *bar = mem_alloc(sizeof *bar);

    return bar ? ui_ach_scroll_bar_construct(bar) : NULL;
}

struct UICustomGageBar *ui_custom_gage_bar_construct(struct UICustomGageBar *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_custom_gage_bar_ops;
    self->value = -1;
    self->max = -1;
    string_init(&self->label);
    self->bar_colour = 0xce7b52;
    self->back_colour = 0xffffff;
    self->outer_colour = 0xefe7e7;
    self->inner_colour = 0xadadad;
    return self;
}

static void ui_custom_gage_bar_destruct(struct UICustomGageBar *self)
{
    self->base.ops = &ui_custom_gage_bar_ops;
    string_free(&self->label);
    ui_window_destruct(&self->base);
}

void ui_custom_gage_bar_destroy(struct UIWindow *window)
{
    struct UICustomGageBar *self = (struct UICustomGageBar *)window;
    ui_custom_gage_bar_destruct(self);
    mem_free(self);
}

struct UICustomGageBar *ach_new_gauge(int width, int height)
{
    struct UICustomGageBar *gauge = mem_try_alloc(sizeof *gauge);

    if (gauge)
        ui_custom_gage_bar_construct(gauge);
    ui_window_on_create_base(&gauge->base, width, height);
    ui_window_resize_base(&gauge->base, width, height);
    gauge->value = 0;
    gauge->max = 100;
    return gauge;
}

void ui_custom_gage_bar_on_draw(struct UIWindow *window)
{
    struct UICustomGageBar *self = (struct UICustomGageBar *)window;
    int w = self->base.width, h = self->base.height;
    float percent;

    if (self->value < 0 || self->max < 0 || self->value > self->max)
        return;
    ui_window_draw_outline(&self->base, 0, 0, w, h, self->outer_colour);
    ui_window_draw_outline(&self->base, 1, 1, w - 2, h - 2, self->inner_colour);
    ui_window_fill_rect(&self->base, 2, 2, w - 4, h - 4, self->back_colour);
    percent = (float)self->value * 100.0f / (float)self->max;
    if (percent > 0.0f)
        ui_window_fill_rect(&self->base, 2, 2, (int)percent * w / 100, h - 4, self->bar_colour);
}

void ui_custom_gage_bar_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UICustomGageBar *self = (struct UICustomGageBar *)window;
    struct String text;
    int sx = 0, sy = -20;

    string_init(&text);
    string_format(&text, "%s( %d / %d )", string_cstr(&self->label), self->value, self->max);
    ui_window_to_screen(&self->base, &sx, &sy);
    mode_show_tooltip(client_current_mode(), string_cstr(&text), sx, sy, 0xffffffff, 0);
    string_free(&text);
}

struct UIToolTipBitmapWnd *ui_tool_tip_bitmap_wnd_construct(struct UIToolTipBitmapWnd *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_tool_tip_bitmap_wnd_ops;
    string_init(&self->tooltip);
    self->bitmap = NULL;
    self->item_id = 0;
    self->item_x = 0;
    self->item_y = 0;
    return self;
}

static void ui_tool_tip_bitmap_wnd_destruct(struct UIToolTipBitmapWnd *self)
{
    self->base.ops = &ui_tool_tip_bitmap_wnd_ops;
    string_free(&self->tooltip);
    ui_window_destruct(&self->base);
}

void ui_tool_tip_bitmap_wnd_destroy(struct UIWindow *window)
{
    struct UIToolTipBitmapWnd *self = (struct UIToolTipBitmapWnd *)window;
    ui_tool_tip_bitmap_wnd_destruct(self);
    mem_free(self);
}

static struct BitmapRes *ach_item_icon(int item_id)
{
    struct ItemInfo info;
    char path[0x104];
    struct BitmapRes *bitmap;

    item_info_construct(&info);
    item_info_set_item_id(&info, item_id);
    memset(path, 0, sizeof path);
    session_get_item_bitmap_file_name(string_cstr(&info.id_text), path, 1);
    bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
    item_info_destruct(&info);
    return bitmap;
}

void ui_tool_tip_bitmap_wnd_set(struct UIToolTipBitmapWnd *self, const char *path, const char *tooltip,
                                       int item_id)
{
    if (tooltip) {
        struct String text;

        string_init(&text);
        string_assign_cstr(&text, tooltip);
        string_free(&self->tooltip);
        self->tooltip = text;
        string_init(&text);
        string_free(&text);
    }
    self->bitmap = ui_skin_bitmap(path);
    if (self->bitmap)
        ui_window_resize_base(&self->base, self->bitmap->width, self->bitmap->height);
    self->item_id = item_id;
}

void ui_tool_tip_bitmap_wnd_on_draw(struct UIWindow *window)
{
    struct UIToolTipBitmapWnd *self = (struct UIToolTipBitmapWnd *)window;
    struct BitmapRes *bitmap;

    if (self->bitmap)
        ui_window_draw_bitmap(&self->base, 0, 0, self->bitmap, 0);
    if (self->item_id <= 0)
        return;
    self->item_x = 0x14;
    self->item_y = 0xf;
    bitmap = ach_item_icon(self->item_id);
    if (bitmap)
        ui_window_draw_bitmap(&self->base, 0x14, 0xf, bitmap, 1);
}

void ui_tool_tip_bitmap_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIToolTipBitmapWnd *self = (struct UIToolTipBitmapWnd *)window;
    ui_window_on_mouse_move_base(&self->base, x, y);
    if (self->tooltip.size > 0) {
        x = 0;
        y = -20;
        ui_window_to_screen(&self->base, &x, &y);
        mode_show_tooltip(client_current_mode(), string_cstr(&self->tooltip), x, y, 0xffffffff, 0);
    }
}

void ui_tool_tip_bitmap_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIToolTipBitmapWnd *self = (struct UIToolTipBitmapWnd *)window;
    struct ItemInfo info;
    struct UIWindow *wnd;

    if (self->item_id <= 0 || self->item_x > x || x > self->item_x + 0x18 || self->item_y > y ||
        y > self->item_y + 0x18)
        return;
    item_info_construct(&info);
    item_info_set_item_id(&info, self->item_id);
    info.identified = 1;
    wnd = ui_window_open(ITEM_INFO_WND_ID);
    ui_window_send(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
    item_info_destruct(&info);
}

struct UIAchTV *ui_ach_tv_construct(struct UIAchTV *self)
{
    ui_text_viewer_construct(&self->viewer.list.base);
    self->viewer.list.base.ops = &ui_ach_tv_ops.base;
    self->no_click = 0;
    self->colour = 0;
    self->outline = 0;
    self->underline = 0;
    return self;
}

static void ui_ach_tv_destruct(struct UIAchTV *self)
{
    self->viewer.list.base.ops = &ui_ach_tv_ops.base;
    self->viewer.list.base.ops = &ui_text_viewer_ops.base;
    vec_free(&self->viewer.embeds);
    ui_list_box_destruct(&self->viewer.list);
}

void ui_ach_tv_destroy(struct UIWindow *window)
{
    struct UIAchTV *self = (struct UIAchTV *)window;
    ui_ach_tv_destruct(self);
    mem_free(self);
}

struct UIAchTV *ach_new_tv(int nothrow)
{
    struct UIAchTV *tv = nothrow ? mem_try_alloc(sizeof *tv) : mem_alloc(sizeof *tv);

    return tv ? ui_ach_tv_construct(tv) : NULL;
}

void ui_ach_tv_on_draw(struct UIWindow *window)
{
    struct UIAchTV *self = (struct UIAchTV *)window;
    struct UIListBox *list = &self->viewer.list;
    unsigned end, count;
    int i, line;

    if (list->transparent == 1) {
        ui_window_clear_canvas(&list->base, UI_TRANSPARENT);
        if (list->base.parent)
            ui_window_invalidate(list->base.parent);
    } else {
        ui_window_clear_canvas(&list->base, self->viewer.background);
    }
    if (list->line_height == list->extra_lines)
        end = list->base.height + list->top;
    else
        end = list->base.height / list->line_height - list->extra_lines + list->top;
    count = VEC_COUNT(&list->items, struct String);
    if (end < count)
        count = end;
    if (list->top >= (int)count)
        return;
    for (i = 0, line = list->top; i < (int)count - list->top; i++, line++) {
        struct String *text = &VEC_AT(&list->items, struct String, line);
        int y = list->line_height * i + 4;

        if (!self->outline)
            ui_window_text_out(&list->base, 3, y, string_cstr(text), text->size, 0, 0xc, self->colour, 0, 0);
        else
            ui_window_text_out_outlined(&list->base, 3, y, string_cstr(text), text->size, self->colour,
                                     self->outline, 0, 0xc, 0, 0);
        if (self->underline == 1) {
            int width = ui_window_text_width(&list->base, string_cstr(text), 0, 0, 0xc, 0, 0);

            ui_window_fill_rect(&list->base, 2, list->line_height * i + 0xa, width + 1, 1, self->colour);
        }
    }
}

void ui_ach_tv_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIAchTV *self = (struct UIAchTV *)window;
    struct UIWindow *parent = self->viewer.list.base.parent;
    int id = self->viewer.list.base.id;

    if (self->no_click == 1 || id == -1 || !parent)
        return;
    ui_window_send_msg(parent, NULL, ACH_MSG_GO_TO, id, 0, 0, 0);
}

void ui_ach_tv_clear(struct UIListBox *base_list)
{
    struct UIAchTV *self = (struct UIAchTV *)base_list;
    ui_text_viewer_clear_items(&self->viewer.list);
    self->underline = 0;
}
