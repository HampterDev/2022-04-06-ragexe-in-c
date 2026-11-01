#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "ui/misc/snap_mgr.h"
#include "ui/character/gauge.h"
#include "ui/character/gauge_sources.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <stdio.h>
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_std.h"
#include "ui/character/character_std.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "ui/character/gauge_internal.h"
#include "ui/wndmgr/wndmgr.h"

#define SESSION_PARAM_AP 0xe8
#define SESSION_PARAM_MAX_AP 0xe9

#define BASIC_INFO_SIZE 0xf8
#define BASIC_OFFSCREEN (-100)

struct BasicInfoLayout {
    int row_y;
    int compact_height;
    int expanded_height;
};

struct UIBasicInfoWnd {
    struct UIFrameWnd frame;
    int compact_height;
    int expanded_height;
    BYTE expanded;
    struct UIWindow *name;
    struct BasicInfoWndInfo *info;
    struct UIWindow *hp_gauge;
    struct UIWindow *sp_gauge;
    struct UIWindow *base_exp_bar;
    struct UIWindow *job_exp_bar;
    int row_y;
    struct UIWindow *ap_gauge;
    struct BasicInfoLayout layout;
    struct BasicInfoLayout ap_layout;
};
OFFSET_ASSERT(UIBasicInfoWnd, name, 0xa0);

void ui_basic_info_wnd_destroy(struct UIWindow *window);
void ui_basic_info_wnd_store_info(struct UIWindow *self);
void ui_basic_info_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_basic_info_wnd_on_draw(struct UIWindow *self);
void ui_basic_info_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_basic_info_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_basic_info_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_basic_info_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
int ui_basic_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_basic_info_wnd_ops = {
    .destroy = ui_basic_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_basic_info_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_basic_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_basic_info_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_basic_info_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_basic_info_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_basic_info_wnd_on_mouse_move,
    .on_mouse_hover = ui_basic_info_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_basic_info_wnd_send_msg,
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

#define BASIC_BELOW_WND 0xb6
#define BASIC_BELOW_MENU_ICON_WND 0x133

#define UI_MSG_COMMAND 6
#define UI_MSG_SET_INFO 0x22
#define UI_MSG_REFRESH 0x3c
#define BASIC_CMD_TOGGLE 0xc2
#define BASIC_CMD_CLOSE_ALL 0xd6

#define BASIC_RED 0xff

static struct UIWindow *basic_wnd(struct UIBasicInfoWnd *self)
{
    return &self->frame.base;
}

static int basic_job(void)
{
    return session_get_drawn_job(&g_session1);
}

static void basic_hang_below(struct UIBasicInfoWnd *self, struct UIWindow *wnd)
{
    int x = 0, y = 0;

    ui_window_to_screen(basic_wnd(self), &x, &y);
    ui_window_move(wnd, x, basic_wnd(self)->height + y);
}

static COLORREF basic_chip(int x, int y)
{
    int red, green, blue;

    ui_skin_color_chip(&g_skinMgr, x, y, &red, &green, &blue);
    return (COLORREF)(((BYTE)blue << 16) | ((BYTE)green << 8) | (BYTE)red);
}

struct UIBasicInfoWnd *ui_basic_info_wnd_construct(struct UIBasicInfoWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_basic_info_wnd_ops;
    self->info = NULL;
    self->expanded = 0;
    self->row_y = 0xd;
    self->compact_height = 0x37;
    self->expanded_height = 0x86;
    self->ap_layout.row_y = 0x1c;
    self->ap_layout.compact_height = 0x47;
    self->ap_layout.expanded_height = 0x95;
    self->layout.row_y = 0xd;
    self->layout.compact_height = 0x37;
    self->layout.expanded_height = 0x86;
    if (session_is_fourth_job(&g_session1, basic_job()) == 1) {
        self->row_y = self->ap_layout.row_y;
        self->compact_height = self->ap_layout.compact_height;
        self->expanded_height = self->ap_layout.expanded_height;
    }
    return self;
}

struct UIWindow *ui_basic_info_wnd_new(void)
{
    return (struct UIWindow *)ui_basic_info_wnd_construct(mem_alloc(sizeof(struct UIBasicInfoWnd)));
}

void ui_basic_info_wnd_destruct(struct UIBasicInfoWnd *self)
{
    self->frame.base.ops = &ui_basic_info_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_basic_info_wnd_destroy(struct UIWindow *window)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    ui_basic_info_wnd_destruct(self);
    mem_free(self);
}

void ui_basic_info_wnd_set_height(struct UIBasicInfoWnd *self, int height)
{
    struct UIWindow *wnd = basic_wnd(self), *below;

    if (height == self->compact_height) {
        ui_window_resize_base(wnd, wnd->width, height);
        below = ui_window_find(BASIC_BELOW_WND);
        if (below) {
            ui_window_set_show(below, 0);
            basic_hang_below(self, below);
        }
        ui_window_resize(self->name, wnd->width - 0x1e, 0xf);
        ui_static_text_set_text_ellipsis(self->name, session_character_name(&g_session1));
        ui_window_move(self->name, 0x11, 3);
        ui_window_move_base(self->hp_gauge, BASIC_OFFSCREEN, BASIC_OFFSCREEN);
        ui_window_move_base(self->sp_gauge, BASIC_OFFSCREEN, BASIC_OFFSCREEN);
        ui_window_move_base(self->ap_gauge, BASIC_OFFSCREEN, BASIC_OFFSCREEN);
        ui_window_move_base(self->base_exp_bar, BASIC_OFFSCREEN, BASIC_OFFSCREEN);
        ui_window_move_base(self->job_exp_bar, BASIC_OFFSCREEN, BASIC_OFFSCREEN);
    } else if (height == self->expanded_height) {
        ui_window_resize_base(wnd, wnd->width, height);
        below = ui_window_find(BASIC_BELOW_WND);
        if (below) {
            ui_window_set_show(below, 1);
            basic_hang_below(self, below);
        }
        ui_window_move_base(self->hp_gauge, 0x23, 0x35);
        ui_window_move_base(self->sp_gauge, 0x23, 0x44);
        if (session_is_fourth_job(&g_session1, basic_job()) == 1)
            ui_window_move_base(self->ap_gauge, 0x23, 0x53);
        ui_window_resize(self->name, wnd->width - 0x14, 0xf);
        ui_static_text_set_text_ellipsis(self->name, session_character_name(&g_session1));
        ui_window_move(self->name, 9, 0x16);
        if (session_get_max_base_level(&g_session1, basic_job()) == g_session1.charBaseLevel)
            ui_window_move_base(self->base_exp_bar, -200, 0x4e);
        else
            ui_window_move_base(self->base_exp_bar, 0x55, self->row_y + 0x4c);
        if (g_session1.jobLevel >= session_get_max_job_level(&g_session1, basic_job()))
            ui_window_move_base(self->job_exp_bar, -200, 0x58);
        else
            ui_window_move_base(self->job_exp_bar, 0x55, self->row_y + 0x58);
    }
    below = ui_window_find(BASIC_BELOW_MENU_ICON_WND);
    if (below)
        basic_hang_below(self, below);
}

void ui_basic_info_wnd_set_base_exp(struct UIBasicInfoWnd *self, __int64 current, __int64 maximum)
{
    uiint64_bar_graph_set_values((struct UIINT64BarGraph *)self->base_exp_bar, current, maximum);
}

void ui_basic_info_wnd_set_job_exp(struct UIBasicInfoWnd *self, __int64 current, __int64 maximum)
{
    uiint64_bar_graph_set_values((struct UIINT64BarGraph *)self->job_exp_bar, current, maximum);
}

static void basic_use_layout(struct UIBasicInfoWnd *self, const struct BasicInfoLayout *layout, int expanded)
{
    self->row_y = layout->row_y;
    self->compact_height = layout->compact_height;
    self->expanded_height = layout->expanded_height;
    ui_basic_info_wnd_set_height(self, expanded ? layout->expanded_height : layout->compact_height);
}

void ui_basic_info_wnd_apply_job_layout(struct UIBasicInfoWnd *self)
{
    int height = basic_wnd(self)->height;

    if (session_is_fourth_job(&g_session1, basic_job())) {
        if (height == self->compact_height)
            basic_use_layout(self, &self->ap_layout, 0);
        else if (height == self->expanded_height)
            basic_use_layout(self, &self->ap_layout, 1);
    } else {
        if (height == self->compact_height)
            basic_use_layout(self, &self->layout, 0);
        else if (height == self->expanded_height)
            basic_use_layout(self, &self->layout, 1);
        ui_window_move_base(self->ap_gauge, BASIC_OFFSCREEN, BASIC_OFFSCREEN);
    }
    ui_window_invalidate_base(&self->frame.base);
}

static struct UIWindow *basic_gauge(struct UIBasicInfoWnd *self, struct UIWindow *gauge,
    struct GaugeValue current, struct GaugeValue maximum)
{
    ui_value_gauge_set(gauge, current, maximum);
    ui_window_create(gauge, 0x87, 9);
    ui_window_move(gauge, BASIC_OFFSCREEN, BASIC_OFFSCREEN);
    ui_window_add_child(basic_wnd(self), gauge);
    return gauge;
}

static struct UIWindow *basic_exp_bar(struct UIBasicInfoWnd *self, int y, __int64 current, __int64 maximum)
{
    struct UIWindow *bar = (struct UIWindow *)uiint64_bar_graph_new();

    uiint64_bar_graph_set_no_warning((struct UIINT64BarGraph *)bar, 1);
    ui_window_create(bar, 0x70, 6);
    ui_window_move_base(bar, 0x54, y);
    uiint64_bar_graph_set_values((struct UIINT64BarGraph *)bar, current, maximum);
    ui_window_add_child(basic_wnd(self), bar);
    return bar;
}

void ui_basic_info_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    struct UIWindow *wnd = basic_wnd(self), *button;
    void *mem;

    (void)width;
    ui_window_add_sys_button(wnd);
    button = ui_window_add_mini_button(wnd, "Alt+V, Ctrl+V", 0);
    ui_window_move_base(button, wnd->width - 0xe, 3);

    mem = mem_alloc(sizeof(struct UIStaticText));
    self->name = mem ? ui_static_text_construct((struct UIWindow *)mem) : NULL;
    ui_window_create(self->name, wnd->width - 0x14, 0xf);
    ui_window_move(self->name, 9, 0x17);
    ui_static_text_set_text_ellipsis(self->name, session_character_name(&g_session1));
    ui_static_text_set_font(self->name, 0, 0xc, 0, 0);
    ui_window_add_child(wnd, self->name);

    self->hp_gauge = ui_value_gauge_create(VALUE_GAUGE_PLAYER);
    basic_gauge(self, self->hp_gauge, gauge_value_from_int(&g_session1.hp), gauge_value_from_int(&g_session1.maxHp));

    self->sp_gauge = ui_value_gauge_create(VALUE_GAUGE_PLAYER);
    basic_gauge(self, self->sp_gauge, gauge_value_from_int(&g_session1.sp), gauge_value_from_int(&g_session1.maxSp));

    self->ap_gauge = ui_value_gauge_create(VALUE_GAUGE_AP);
    basic_gauge(self, self->ap_gauge, gauge_ap_source(), gauge_max_ap_source());

    self->base_exp_bar = basic_exp_bar(self, self->row_y + 0x4c, g_session1.baseExp, g_session1.nextBaseExp);
    self->job_exp_bar = basic_exp_bar(self, self->row_y + 0x56, g_session1.jobExp, g_session1.nextJobExp);
    ui_basic_info_wnd_set_height(self, height);
}

static void basic_base_exp(__int64 *exp, __int64 *next)
{
    *exp = g_session1.baseExp;
    if (*exp < 0)
        *exp = 0;
    *next = g_session1.nextBaseExp;
    if (*next <= 0)
        *next = *exp + 1;
}

static COLORREF basic_low_colour(int value, int maximum)
{
    if (maximum > 0 && 0.25f > (float)value / (float)maximum)
        return BASIC_RED;
    return 0;
}

static int basic_small_font_job(int job, int with_17)
{
    if (with_17 && job == 0x17)
        return 1;
    return (job >= 0xfce && job <= 0xfd1) || (job >= 0x108f && job <= 0x1094) || job == 0x10ce || job == 0x10cf ||
           job == 0x10dc;
}

static void basic_draw_ap_compact(struct UIBasicInfoWnd *self)
{
    struct UIWindow *wnd = basic_wnd(self);
    struct String text;
    int face = 1, font_height = 0xe, job, level;
    COLORREF colour;
    __int64 exp, next;
    double percent;

    ui_window_fill_rect(wnd, 1, 0x11, wnd->width - 2, wnd->height - 0x12, basic_chip(2, 2));
    ui_window_draw_round_frame(wnd, 0, 0xe, wnd->width, wnd->height - 0xe, basic_chip(0xe, 6));
    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (g_serviceType == 0 && basic_small_font_job(basic_job(), 0)) {
        font_height = 0xb;
        face = 0;
    }

    level = g_session1.charBaseLevel;
    string_init(&text);
    string_format(&text, "%s %d / %s / %s %d", msg_str(0xbfe), level,
                  session_get_job_name(&g_session1, basic_job(), -1), msg_str(0xe2b), g_session1.jobLevel);
    ui_window_text_out(wnd, 8, 0x14, string_cstr(&text), 0, face, font_height, 0, 0, 0);
    string_free(&text);

    colour = basic_low_colour(g_session1.hp, g_session1.maxHp);
    string_format(&text, "%s %d / %d", msg_str(0xbfc), g_session1.hp, g_session1.maxHp);
    ui_window_text_out(wnd, 8, 0x24, string_cstr(&text), 0, face, font_height, colour, 0, 0);
    string_free(&text);

    basic_base_exp(&exp, &next);
    job = basic_job();
    if (g_session1.charBaseLevel < session_get_max_base_level(&g_session1, job))
        percent = (double)(__int64)((double)exp / (double)next * 1000.0) * 0.1;
    else
        percent = 100.0;
    string_format(&text, "| %s %.1lf%%", msg_str(0xc00), percent);
    ui_window_text_out(wnd, 0x88, 0x24, string_cstr(&text), 0, face, font_height, 0, 0, 0);
    string_free(&text);

    colour = basic_low_colour(g_session1.sp, g_session1.maxSp);
    string_format(&text, "%s %d / %d", msg_str(0xbfd), g_session1.sp, g_session1.maxSp);
    ui_window_text_out(wnd, 8, 0x34, string_cstr(&text), 0, face, font_height, colour, 0, 0);
    string_free(&text);

    string_format(&text, "| %s %d / %d", msg_str(0xe2c), session_get_param(&g_session1, SESSION_PARAM_AP),
                  session_get_param(&g_session1, SESSION_PARAM_MAX_AP));
    ui_window_text_out(wnd, 0x88, 0x34, string_cstr(&text), 0, face, font_height, 0, 0, 0);
    string_free(&text);
}

static void basic_draw_percent(struct UIBasicInfoWnd *self, int y, int value, int maximum)
{
    struct String text;

    string_init(&text);
    string_format(&text, "%d%%", value * 100 / maximum);
    ui_window_text_out(basic_wnd(self), 0xad, y, string_cstr(&text), 0, 1, 0xe, 0, 0, 0);
    string_free(&text);
}

static void basic_draw_ap_expanded(struct UIBasicInfoWnd *self)
{
    struct UIWindow *wnd = basic_wnd(self);
    struct String text, joined, zeny, weight;
    char money[0x40];
    int face, font_height, zeny_width, max_ap, divisor;

    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\w_basewin_bg2.bmp"), 0);
    ui_window_text_out(wnd, 0x12, 3, msg_str(0xee), 0, 0, 0xc, 0xffffff, 0, 0);
    ui_window_text_out(wnd, 0x11, 2, msg_str(0xee), 0, 0, 0xc, 0, 0, 0);
    if (g_serviceType == 0 || g_serviceType == 0xe) {
        face = 0;
        font_height = 0xc;
    } else {
        face = 1;
        font_height = 0xe;
    }
    ui_window_text_out(wnd, 9, 0x24, session_get_job_name(&g_session1, basic_job(), -1), 0, face,
                     font_height, 0, 0, 0);
    ui_window_text_out(wnd, 9, 0x33, msg_str(0xbfc), 0, 1, 0xe, 0, 0, 0);
    ui_window_text_out(wnd, 9, 0x41, msg_str(0xbfd), 0, 1, 0xe, 0, 0, 0);
    ui_window_text_out(wnd, 9, 0x50, msg_str(0xe2c), 0, 1, 0xe, 0, 0, 0);
    if (g_session1.maxHp > 0)
        basic_draw_percent(self, 0x33, g_session1.hp, g_session1.maxHp);
    if (g_session1.maxSp > 0)
        basic_draw_percent(self, 0x41, g_session1.sp, g_session1.maxSp);
    if (session_get_param(&g_session1, SESSION_PARAM_MAX_AP) > 0) {
        int ap = session_get_param(&g_session1, SESSION_PARAM_AP);

        max_ap = session_get_param(&g_session1, SESSION_PARAM_MAX_AP);
        basic_draw_percent(self, 0x50, ap, max_ap);
    }

    string_init(&text);
    string_format(&text, msg_str(0x7cb), g_session1.charBaseLevel);
    ui_window_text_out(wnd, 0x11, self->row_y + 0x47, string_cstr(&text), 0, 1, 0xe, 0, 0, 0);
    string_free(&text);

    string_format(&text, msg_str(0x7cc), g_session1.jobLevel);
    if (g_serviceType == 0xc) {
        struct String brazil;

        string_init(&brazil);
        string_format(&brazil, "Classe. %d", g_session1.jobLevel);
        string_assign(&text, string_cstr(&brazil), brazil.size);
        string_free(&brazil);
    }
    ui_window_text_out(wnd, 0x11, self->row_y + 0x53, string_cstr(&text), 0, 1, 0xe, 0, 0, 0);
    string_free(&text);

    memset(money, 0, sizeof money);
    format_money(g_session1.zeny, money);
    string_init(&zeny);
    string_format(&zeny, msg_str(0x7cd), money);
    string_init(&joined);
    string_assign_cstr(&joined, " | ");
    string_append(&joined, string_cstr(&zeny), zeny.size);
    string_free(&zeny);
    zeny = joined;
    ui_window_text_out_right(wnd, wnd->width - 5, self->row_y + 0x67, string_cstr(&zeny), 0, 1, 0xe, 0, 0);
    zeny_width = ui_window_text_width(wnd, string_cstr(&zeny), 0, 1, 0xe, 0, 0);

    string_init(&weight);
    if (g_serviceType == 0xc)
        string_format(&text, "Peso : %3d / %3d", g_session1.weight, g_session1.maxWeight);
    else
        string_format(&text, msg_str(0x7c8), g_session1.weight, g_session1.maxWeight);
    string_assign(&weight, string_cstr(&text), text.size);
    string_free(&text);
    divisor = g_session1.maxWeight > 1 ? g_session1.maxWeight : 1;
    ui_window_text_out_right(wnd, wnd->width - zeny_width - 5, self->row_y + 0x67, string_cstr(&weight), 0, 1,
                          0xe, g_session1.weight * 100 / divisor >= g_session1.weightPenaltyPercent ? BASIC_RED : 0, 0);
    string_free(&weight);
    string_free(&zeny);
}

static int basic_max_level(void)
{
    if (session_is_third_job(&g_session1, basic_job()))
        return g_session1.baseLevel3rd;
    if (session_is_expanded_super_novice_job(&g_session1, basic_job()))
        return g_session1.baseLevelExtend2;
    if (session_is_expanded_job(&g_session1, basic_job()))
        return g_session1.baseLevelUpperJob;
    if (session_is_doram_job(&g_session1, basic_job()) == 1)
        return g_session1.baseLevelDoram;
    return g_session1.baseLevel;
}

static COLORREF basic_low_colour100(int value, int maximum)
{
    return 25.0f > (float)value * 100.0f / (float)maximum ? BASIC_RED : 0;
}

static void basic_draw_compact(struct UIBasicInfoWnd *self)
{
    struct UIWindow *wnd = basic_wnd(self);
    char text[0x80], money[0x40];
    COLORREF colour;
    int face = 1, font_height = 0xe, x = 9, len, offset, width;
    __int64 exp, next;
    double percent;

    colour = basic_chip(0xe, 6);
    ui_window_draw_round_frame(wnd, 0, 0x11, wnd->width, wnd->height - 0x11, colour);
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, 5, colour);
    colour = basic_chip(2, 2);
    ui_window_fill_rect(wnd, 1, 0x11, wnd->width - 2, wnd->height - 0x13, colour);
    ui_window_fill_rect(wnd, 2, wnd->height - 2, wnd->width - 4, 1, colour);
    ui_window_draw_title_bar(wnd, 0, "", 0);

    basic_base_exp(&exp, &next);
    if (basic_max_level() == g_session1.charBaseLevel)
        percent = 100.0;
    else
        percent = (double)((float)(int)((double)exp / (double)next * 1000.0) * 0.1f);

    if (g_serviceType == 0xc) {
        len = sprintf(text, "Nv. %2d / %s / Nv. %2d / Exp. %lld %%", g_session1.charBaseLevel,
                          session_get_job_name(&g_session1, basic_job(), -1), g_session1.jobLevel,
                          (__int64)percent);
    } else {
        if (g_serviceType == 0 && basic_small_font_job(basic_job(), 1)) {
            face = 0;
            font_height = 0xb;
        }
        len = sprintf(text, "Lv.%2d / %s / Lv.%2d / Exp. %.1lf %%", g_session1.charBaseLevel,
                          session_get_job_name(&g_session1, basic_job(), -1), g_session1.jobLevel, percent);
        if (ui_window_text_width(wnd, text, len, face, font_height, 0, 0) + 9 > wnd->width) {
            len = sprintf(text, "Lv%2d / %s / Lv%2d / Exp%.1lf%%", g_session1.charBaseLevel,
                              session_get_job_name(&g_session1, basic_job(), -1), g_session1.jobLevel, percent);
            width = wnd->width - ui_window_text_width(wnd, text, len, face, font_height, 0, 0);
            x = width < 0 ? 3 : width / 2;
        }
    }
    ui_window_text_out(wnd, x, 0x14, text, len, face, font_height, 0, 0, 0);
    format_money(g_session1.zeny, money);

    offset = 0;
    if (g_session1.maxHp != 0) {
        colour = basic_low_colour100(g_session1.hp, g_session1.maxHp);
        len = sprintf(text, x == 9 ? "HP. %d / %d" : "HP %d / %d", g_session1.hp, g_session1.maxHp);
        ui_window_text_out(wnd, x, 0x24, text, len, 1, 0xe, colour, 0, 0);
        offset = ui_window_text_width(wnd, text, len, 1, 0xe, 0, 0);
        len = sprintf(text, " | ");
        ui_window_text_out(wnd, offset + x, 0x24, text, len, 1, 0xe, 0, 0, 0);
    }
    if (g_session1.maxSp >= 0) {
        colour = basic_low_colour100(g_session1.sp, g_session1.maxSp);
        offset += ui_window_text_width(wnd, text, len, 1, 0xe, 0, 0);
        len = sprintf(text, x == 9 ? "SP. %d / %d" : "SP %d / %d", g_session1.sp, g_session1.maxSp);
        ui_window_text_out(wnd, offset + x, 0x24, text, len, 1, 0xe, colour, 0, 0);
    }
}

static void basic_draw_expanded(struct UIBasicInfoWnd *self)
{
    struct UIWindow *wnd = basic_wnd(self);
    char text[0x80], money[0x40];
    const char *title;
    int face, font_height, zeny_width, divisor;

    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\basewin_bg2.bmp"), 0);
    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (g_serviceType == 0 || g_serviceType == 0xe) {
        face = 0;
        font_height = 0xc;
    } else {
        face = 1;
        font_height = 0xe;
    }
    ui_window_text_out(wnd, 9, 0x24, session_get_job_name(&g_session1, basic_job(), -1), 0, face,
                     font_height, 0, 0, 0);
    memset(text, 0, sizeof text);
    if (g_serviceType == 1) {
        ui_window_text_out(wnd, 0x12, 3, "Basic Info", 0, 0, 0xc, 0xffffff, 0, 0);
        title = "Basic Info";
    } else {
        ui_window_text_out(wnd, 0x12, 3, msg_str(0xee), 0, 0, 0xc, 0xffffff, 0, 0);
        title = msg_str(0xee);
    }
    ui_window_text_out(wnd, 0x11, 2, title, 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0x11, 0x33, "HP", 0, 1, 0xe, 0, 0, 0);
    ui_window_text_out(wnd, 0x11, 0x41, "SP", 0, 1, 0xe, 0, 0, 0);
    if (g_session1.maxHp != 0) {
        sprintf(text, "%d%%", g_session1.hp * 100 / g_session1.maxHp);
        ui_window_text_out(wnd, 0xad, 0x33, text, 0, 1, 0xe, 0, 0, 0);
    }
    if (g_session1.maxSp != 0) {
        sprintf(text, "%d%%", g_session1.sp * 100 / g_session1.maxSp);
        ui_window_text_out(wnd, 0xad, 0x41, text, 0, 1, 0xe, 0, 0, 0);
    }
    sprintf(text, g_serviceType == 0xc ? "Base. %d" : "Base Lv. %d", g_session1.charBaseLevel);
    ui_window_text_out(wnd, 0x11, self->row_y + 0x47, text, 0, 1, 0xe, 0, 0, 0);
    sprintf(text, g_serviceType == 0xc ? "Classe. %d" : "Job Lv. %d", g_session1.jobLevel);
    ui_window_text_out(wnd, 0x11, self->row_y + 0x53, text, 0, 1, 0xe, 0, 0, 0);

    memset(money, 0, sizeof money);
    format_money(g_session1.zeny, money);
    sprintf(text, "Zeny : %s", money);
    zeny_width = ui_window_text_width(wnd, text, (int)strlen(text), 1, 0xe, 0, 0);
    ui_window_text_out_right(wnd, wnd->width - 5, self->row_y + 0x67, text, (int)strlen(text), 1, 0xe, 0, 0);
    sprintf(text, g_serviceType == 0xc ? "Peso : %3d / %3d" : "Weight : %3d / %3d", g_session1.weight, g_session1.maxWeight);
    divisor = g_session1.maxWeight > 1 ? g_session1.maxWeight : 1;
    ui_window_text_out_right(wnd, wnd->width - zeny_width - 10, self->row_y + 0x67, text, (int)strlen(text), 1,
                          0xe, g_session1.weight * 100 / divisor >= g_session1.weightPenaltyPercent ? BASIC_RED : 0, 0);
}

void ui_basic_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    int height = basic_wnd(self)->height;

    if (session_is_fourth_job(&g_session1, basic_job()) == 1) {
        if (height == self->compact_height)
            basic_draw_ap_compact(self);
        else if (height == self->expanded_height)
            basic_draw_ap_expanded(self);
        return;
    }
    if (height == self->compact_height)
        basic_draw_compact(self);
    else if (height == self->expanded_height)
        basic_draw_expanded(self);
}

void ui_basic_info_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

void ui_basic_info_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    (void)x;
    if (y < 0x11)
        ui_basic_info_wnd_send_msg(&self->frame.base, basic_wnd(self), UI_MSG_COMMAND, BASIC_CMD_TOGGLE, 0, 0, 0);
}

void ui_basic_info_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    struct UIWindow *below;

    ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
    below = ui_window_find(BASIC_BELOW_MENU_ICON_WND);
    if (below)
        basic_hang_below(self, below);
    below = ui_window_find(BASIC_BELOW_WND);
    if (below)
        basic_hang_below(self, below);
}

void ui_basic_info_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    struct UIWindow *wnd = basic_wnd(self);
    char text[0x40];
    int top, sx, sy;

    if (wnd->height != self->expanded_height)
        return;
    top = wnd->height - 0x14;
    if (x < 0 || x >= 100 || y < top || y >= wnd->height)
        return;
    sprintf(text, "Weight %d%%", g_session1.maxWeight ? g_session1.weight * 100 / g_session1.maxWeight : 0);
    sx = 0;
    sy = top - 0x13;
    ui_window_to_screen(wnd, &sx, &sy);
    mode_show_tooltip(client_current_mode(), text, sx, sy, 0xffffffff, 0);
}

void ui_basic_info_wnd_store_info(struct UIWindow *window)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    struct UIWindow *wnd = basic_wnd(self);

    if (!self->info)
        return;
    self->info->rect.height = wnd->height;
    self->info->rect.width = wnd->width;
    self->info->rect.x = wnd->x;
    self->info->rect.y = wnd->y;
    self->info->expanded = self->expanded;
}

static void basic_toggle(int window_id)
{
    if (!ui_window_mgr_post_quit1(&g_windowMgr, window_id))
        ui_window_open(window_id);
}

static const int basic_closed_windows[] = {0xb, 0xd, 8, 0x141, 0xa, 0x45, 0x1b, 0x1e, 0x25, 0x58, 0x56, 0x57,
                                           0x28, 0x3b};

int ui_basic_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UIBasicInfoWnd *self = (struct UIBasicInfoWnd *)window;
    struct UIWindow *wnd = basic_wnd(self), *below;

    if (msg == UI_MSG_REFRESH) {
        if (wnd->height != self->expanded_height) {
            below = ui_window_find(BASIC_BELOW_WND);
            if (below)
                ui_window_set_show(below, self->expanded);
            ui_window_invalidate_base(wnd);
        }
        return 0;
    }
    if (msg == UI_MSG_SET_INFO) {
        self->info = (struct BasicInfoWndInfo *)val1;
        if (!basic_info_wnd_info_is_on_screen(self->info))
            basic_info_wnd_info_set_default(self->info);
        self->expanded = self->info->expanded != 0;
        ui_basic_info_wnd_set_height(self, self->info->rect.height);
        ui_frame_wnd_move(wnd, self->info->rect.x, self->info->rect.y);
        below = ui_window_find(BASIC_BELOW_WND);
        if (below) {
            ui_window_move(below, self->info->rect.x, self->info->rect.y + wnd->height);
            ui_window_set_show(below, self->expanded);
        }
        return 0;
    }
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);

    switch (val1) {
    case 0xb8:
        basic_toggle(0xb);
        break;
    case 0xb9:
        basic_toggle(0x9b);
        break;
    case 0xba:
        basic_toggle(8);
        break;
    case 0xbb:
        basic_toggle(0xa);
        break;
    case 0xbc:
        basic_toggle(0x25);
        break;
    case 0xbf:
        basic_toggle(0x45);
        break;
    case 0xd3:
        basic_toggle(0x8c);
        break;
    case 0x161:
        basic_toggle(0x141);
        break;
    case 0x16d:
        basic_toggle(0x3b);
        break;
    case 0xef:
        if (!ui_window_find(0xe3) && !ui_window_mgr_post_quit1(&g_windowMgr, 0x28) &&
            g_session1.cartType > 0)
            ui_window_open(0x28);
        break;
    case 0xce:
        if (ui_window_mgr_post_quit1(&g_windowMgr, 0x1b) || g_windowMgr.chat_room_wnd)
            break;
        if (!g_windowMgr.merchant_shop_make_wnd && !g_windowMgr.merchant_item_my_shop_wnd)
            ui_window_open(0x1b);
        else
            ui_window_mgr_message_box(&g_windowMgr, msg_str(0x24b), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        break;
    case BASIC_CMD_CLOSE_ALL: {
        size_t i;

        for (i = 0; i < sizeof basic_closed_windows / sizeof basic_closed_windows[0]; i++)
            ui_window_mgr_post_quit1(&g_windowMgr, basic_closed_windows[i]);
        ui_window_mgr_show_option_window(&g_windowMgr, 0x24, 0);
        ui_basic_info_wnd_set_height(self, self->compact_height);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        break;
    }
    case BASIC_CMD_TOGGLE: {
        int old_height = wnd->height;

        ui_basic_info_wnd_set_height(self,
                                 old_height == self->compact_height ? self->expanded_height : self->compact_height);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        ui_window_mgr_raise_window(&g_windowMgr, wnd);
        if (wnd->height != old_height)
            snap_mgr_move_chain(&g_snapMgr, wnd, 0, wnd->height - old_height, 3);
        break;
    }
    }
    return 0;
}
