#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <string.h>
#include "ui/shops/shops.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "platform/clock.h"

struct UIMileageWnd;
struct UIWindow;
void ui_mileage_wnd_destroy(struct UIWindow *window);
void ui_mileage_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_mileage_wnd_on_process(struct UIWindow *self);
void ui_mileage_wnd_on_draw(struct UIWindow *self);
void ui_mileage_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_mileage_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_mileage_wnd_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused);
void ui_mileage_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_mileage_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_mileage_wnd_ops = {
    .destroy = ui_mileage_wnd_destroy,
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
    .on_create = ui_mileage_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_mileage_wnd_on_process,
    .on_draw = ui_mileage_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_mileage_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_mileage_wnd_on_mouse_move,
    .on_mouse_hover = ui_mileage_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_mileage_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_mileage_wnd_send_msg,
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

#define PKT_CZ_DYNAMICNPC_CREATE_REQUEST 0x0a16
#define MILEAGE_MAX_POINTS 300
#define MILEAGE_HOUR 3600
#define MSG_MILEAGE_RULES 0xa3b

struct UIMileageWnd {
    struct UIFrameWnd frame;
    DWORD last_tick;
    int points;
    int seconds_left;
    int grade;
    struct String tooltip;
};

struct UIMileageWnd *ui_mileage_wnd_construct(struct UIMileageWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_mileage_wnd_ops;
    self->last_tick = 0;
    self->points = 0;
    self->seconds_left = 0;
    self->grade = 1;
    string_init(&self->tooltip);
    return self;
}

struct UIWindow *ui_mileage_wnd_new(void)
{
    return (struct UIWindow *)ui_mileage_wnd_construct(mem_alloc(sizeof(struct UIMileageWnd)));
}

static void ui_mileage_wnd_destruct(struct UIMileageWnd *self)
{
    self->frame.base.ops = &ui_mileage_wnd_ops;
    string_free(&self->tooltip);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_mileage_wnd_destroy(struct UIWindow *window)
{
    struct UIMileageWnd *self = (struct UIMileageWnd *)window;
    ui_mileage_wnd_destruct(self);
    mem_free(self);
}

void ui_mileage_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMileageWnd *self = (struct UIMileageWnd *)window;
    struct String tmp;

    string_init(&tmp);
    string_format(&tmp, msg_str(MSG_MILEAGE_RULES), MILEAGE_MAX_POINTS);
    string_free(&self->tooltip);
    string_move(&self->tooltip, &tmp);
}

void ui_mileage_wnd_on_process(struct UIWindow *window)
{
    struct UIMileageWnd *self = (struct UIMileageWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    DWORD now, elapsed, seconds;

    if (!wnd->visible || self->points >= MILEAGE_MAX_POINTS || self->seconds_left == 0)
        return;
    now = platform_ticks_ms();
    elapsed = now - self->last_tick;
    if (elapsed < 1000)
        return;
    seconds = elapsed / 1000;
    self->seconds_left -= seconds;
    if (self->seconds_left < 0)
        self->seconds_left = 0;
    self->last_tick = now - (elapsed - seconds * 1000);
    ui_window_invalidate_base(wnd);
    g_session1.pcCafeSecondsServed = MILEAGE_HOUR - self->seconds_left;
}

void ui_mileage_wnd_on_draw(struct UIWindow *window)
{
    struct UIMileageWnd *self = (struct UIMileageWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String text, tmp;
    const char *s;

    ui_window_clear_canvas(wnd, RGB(0xff, 0, 0xff));
    string_init(&text);

    string_init(&tmp);
    string_format(&tmp, UI_SKIN "basic_interface\\mileage_bg%d.bmp", self->grade);
    string_free(&text);
    string_move(&text, &tmp);
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(string_cstr(&text)), 1);

    if (self->points >= MILEAGE_MAX_POINTS)
        string_assign_cstr(&text, "-- : --");
    else {
        string_init(&tmp);
        string_format(&tmp, "%02d : %02d", self->seconds_left / 60, self->seconds_left % 60);
        string_free(&text);
        string_move(&text, &tmp);
    }
    s = string_cstr(&text);
    ui_window_text_out_outlined(wnd, (wnd->width - ui_window_text_width(wnd, s, 0, 0, 12, 0, 0)) >> 1, 15,
                             s, 0, 0xffffff, 0, 0, 12, 0, 0);

    string_init(&tmp);
    string_format(&tmp, "%d", self->points);
    s = string_cstr(&tmp);
    string_assign_cstr(&text, s);
    string_free(&tmp);
    s = string_cstr(&text);
    ui_window_text_out_outlined(wnd, wnd->width - ui_window_text_width(wnd, s, 0, 0, 12, 0, 0) - 9,
                             wnd->height - 16, s, 0, 0xffffff, 0, 0, 12, 0, 0);
    string_free(&text);
}

void ui_mileage_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_mileage_wnd_on_mouse_move(struct UIWindow *self, int x, int y)
{
}

void ui_mileage_wnd_on_mouse_hover(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIMileageWnd *self = (struct UIMileageWnd *)window;
    int x, y;

    if (self->tooltip.size == 0)
        return;
    x = 10;
    y = -20;
    ui_window_to_screen(&self->frame.base, &x, &y);
    mode_show_tooltip(g_modeMgr.curMode, string_cstr(&self->tooltip), x, y, -1, 0);
}

void ui_mileage_wnd_on_l_btn_up(struct UIWindow *self, int x, int y)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char npc_name[24];
    } pkt;
#pragma pack(pop)

    pkt.id = PKT_CZ_DYNAMICNPC_CREATE_REQUEST;
    memset(pkt.npc_name, 0, sizeof pkt.npc_name);
    snprintf(pkt.npc_name, sizeof pkt.npc_name, "GOLDPCCAFE");
    send_packet(&pkt, sizeof pkt);
}

int ui_mileage_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                    int val2, int val3, int val4)
{
    struct UIMileageWnd *self = (struct UIMileageWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg != UI_MSG_SET_INFO)
        return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);
    g_session1.pcCafeActive = val1;
    ui_window_set_show_base(wnd, val1);
    if (val1) {
        self->points = val2;
        g_session1.pcCafePoints = val2;
        self->grade = val4;
        g_session1.pcCafeGrade = val4;
        self->seconds_left = MILEAGE_HOUR - val3 < 0 ? 0 : MILEAGE_HOUR - val3;
        self->last_tick = platform_ticks_ms();
        ui_window_set_show_base(wnd, g_session1.showGoldPcIcon);
    }
    ui_window_invalidate_base(wnd);
    return 0;
}
