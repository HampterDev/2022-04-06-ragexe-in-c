#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/quests/achievement_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"

struct UIAchUpperWnd;
void ui_ach_upper_wnd_destroy(struct UIWindow *window);
void ui_ach_upper_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_ach_upper_wnd_on_draw(struct UIWindow *self);
int ui_ach_upper_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_ach_upper_wnd_ops = {
    .destroy = ui_ach_upper_wnd_destroy,
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
    .on_create = ui_ach_upper_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ach_upper_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_ach_upper_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define UPPER_TEXT_COLOUR 0x582708
#define UPPER_LEVEL_COLOUR 0x44c58

struct UIAchUpperWnd *ui_ach_upper_wnd_construct(struct UIAchUpperWnd *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_ach_upper_wnd_ops;
    string_init(&self->caption);
    self->points = 0;
    self->points_max = 0;
    self->count = 0;
    self->count_max = 0;
    self->points_gauge = NULL;
    self->count_gauge = NULL;
    self->radios[0] = NULL;
    self->radios[1] = NULL;
    self->order = 0;
    return self;
}

static void ui_ach_upper_wnd_destruct(struct UIAchUpperWnd *self)
{
    self->base.ops = &ui_ach_upper_wnd_ops;
    string_free(&self->caption);
    ui_window_destruct(&self->base);
}

void ui_ach_upper_wnd_destroy(struct UIWindow *window)
{
    struct UIAchUpperWnd *self = (struct UIAchUpperWnd *)window;
    ui_ach_upper_wnd_destruct(self);
    mem_free(self);
}

void ui_ach_upper_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIAchUpperWnd *self = (struct UIAchUpperWnd *)window;
    const char *label;
    int i, y;

    self->points_gauge = ach_new_gauge(0x54, 0xa);
    ui_window_move_base(&self->points_gauge->base, 0x2f, 0x18);
    label = msg_str(0xc21);
    string_assign_cstr(&self->points_gauge->label, label);
    ui_window_add_child(&self->base, &self->points_gauge->base);
    self->count_gauge = ach_new_gauge(0x7c, 8);
    ui_window_move_base(&self->count_gauge->base, 0x265, 0x1d);
    ui_window_add_child(&self->base, &self->count_gauge->base);
    for (i = 0, y = 0xa; y < 0x28; i++, y += 0xf) {
        struct UIRadioBtn *radio = mem_alloc(sizeof *radio);

        self->radios[i] = radio ? ui_radio_btn_construct(radio) : NULL;
        ui_window_create((struct UIWindow *)self->radios[i], 0xc, 0xc);
        ui_window_move_base(&self->radios[i]->base, 0x96, y);
        ui_radio_btn_send_msg(&self->radios[i]->base, &self->base, UI_MSG_SET_STATE, i == self->order, 0, 0, 0);
        ui_window_add_child(&self->base, (struct UIWindow *)self->radios[i]);
    }
}

static int upper_is_summary(struct UIAchUpperWnd *self)
{
    const char *summary = msg_str(0xa77);

    return string_equal_cstr(&self->caption, summary);
}

static void upper_number(struct UIAchUpperWnd *self, int x, int y, int width, int value, COLORREF colour)
{
    struct String text;

    string_init(&text);
    string_format(&text, "%d", value);
    ui_window_text_out_centred(&self->base, x, y, string_cstr(&text), width, 0, 0xc, colour, 1);
    string_free(&text);
}

static void upper_ratio(struct UIAchUpperWnd *self, int x, int y, const char *fmt, int a, int b, COLORREF colour)
{
    struct String text;

    string_init(&text);
    string_format(&text, fmt, a, b);
    ui_window_text_out_right(&self->base, x, y, string_cstr(&text), 0, 0, 0xb, colour, 0);
    string_free(&text);
}

void ui_ach_upper_wnd_on_draw(struct UIWindow *window)
{
    struct UIAchUpperWnd *self = (struct UIAchUpperWnd *)window;
    int i, shown;

    if (upper_is_summary(self)) {
        ach_draw(&self->base, 0, 0, ACH_DIR "bg_upper.bmp", 0);
        upper_ratio(self, 0x82, 0xd, "%d/%d", self->points, self->points_max, 0);
        upper_number(self, 0x161, 6, 0x2e, g_session1.achTotalPoints, UPPER_TEXT_COLOUR);
        upper_number(self, 0x10, 0x13, 0xf, g_session1.achRank, UPPER_LEVEL_COLOUR);
        shown = 0;
    } else {
        ach_draw(&self->base, 0, 0, ACH_DIR "bg_upper.bmp", 0);
        ach_draw(&self->base, 0x23f, 3, ACH_DIR "upper_trophy.bmp", 1);
        ui_window_text_out(&self->base, 0xa9, 0xb, msg_str(0xc22), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out(&self->base, 0xa9, 0x1a, msg_str(0xc23), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out(&self->base, 0x266, 0xf, string_cstr(&self->caption), 0, 0, 0xb,
                         UPPER_TEXT_COLOUR, 0, 0);
        upper_ratio(self, 0x82, 0xd, "%d/%d", self->points, self->points_max, 0);
        upper_ratio(self, 0x2e1, 0xf, "(%d/%d)", self->count, self->count_max, UPPER_TEXT_COLOUR);
        upper_number(self, 0x161, 6, 0x2e, g_session1.achTotalPoints, UPPER_TEXT_COLOUR);
        upper_number(self, 0x10, 0x13, 0xf, g_session1.achRank, UPPER_LEVEL_COLOUR);
        shown = 1;
    }
    for (i = 0; i < 2; i++)
        ui_window_set_show_base(&self->radios[i]->base, shown);
    ui_window_set_show_base(&self->count_gauge->base, shown);
}

int ui_ach_upper_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIAchUpperWnd *self = (struct UIAchUpperWnd *)window;
    int i;

    if (msg == UI_MSG_COMMAND && val1 == UI_ID_RADIO) {
        for (i = 0; i < 2; i++) {
            ui_window_invalidate_base(&self->radios[i]->base);
            if (sender == (struct UIWindow *)self->radios[i]) {
                ui_radio_btn_send_msg(&self->radios[i]->base, &self->base, UI_MSG_SET_STATE, 1, 0, 0, 0);
                self->order = i;
                if (self->base.parent)
                    ui_window_send_msg(self->base.parent, &self->base, ACH_MSG_SELECT_TAB, -1, -1, 0, 0);
            } else {
                ui_radio_btn_send_msg(&self->radios[i]->base, &self->base, UI_MSG_SET_STATE, 0, 0, 0, 0);
            }
        }
    }
    ui_window_invalidate_base(&self->base);
    return 0;
}

void ui_ach_upper_wnd_set_count(struct UIAchUpperWnd *self, int count, int max)
{
    self->count = count;
    self->count_gauge->value = count;
    self->count_max = max;
    self->count_gauge->max = max;
    ui_window_invalidate_base(&self->count_gauge->base);
}

void ui_ach_upper_wnd_set_points(struct UIAchUpperWnd *self, int points, int max)
{
    self->points = points;
    self->points_gauge->value = points;
    self->points_max = max;
    self->points_gauge->max = max;
    ui_window_invalidate_base(&self->points_gauge->base);
}
