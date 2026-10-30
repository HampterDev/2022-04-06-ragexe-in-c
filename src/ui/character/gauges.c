#include "ui/character/gauge.h"
#include "core/object_type_tables.h"
#include "ui/character/character.h"
#include "ui/character/character_std.h"
#include "diag/perf.h"

struct UIGage;
void ui_gage_destroy(struct UIWindow *window);
const struct UIWindowOps ui_gage_ops = {
    .destroy = ui_gage_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_query_false,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_window_on_draw_base,
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
struct UIPcGage;
void ui_pc_gage_destroy(struct UIWindow *window);
void ui_pc_gage_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_pc_gage_ops = {
    .destroy = ui_pc_gage_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_query_false,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_pc_gage_on_draw,
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
struct UIMonsterGage;
void ui_monster_gage_destroy(struct UIWindow *window);
void ui_monster_gage_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_monster_gage_ops = {
    .destroy = ui_monster_gage_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_query_false,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_monster_gage_on_draw,
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
struct UIRechargeGage;
void ui_recharge_gage_destroy(struct UIWindow *window);
void ui_recharge_gage_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_recharge_gage_ops = {
    .destroy = ui_recharge_gage_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_query_false,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_recharge_gage_on_draw,
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
struct UIPlayerGage;
void ui_player_gage_destroy(struct UIWindow *window);
void ui_player_gage_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_player_gage_ops = {
    .destroy = ui_player_gage_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 1,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_query_false,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_player_gage_on_draw,
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

#define GAUGE_LOW_PERCENT 25.0f

struct UIGage *ui_gage_construct(struct UIGage *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_gage_ops;
    self->clear_colour = 0x9b1914;
    self->back_colour = 0x444444;
    self->bar_colour = 0x25ea16;
    self->second_colour = 0xdb651d;
    self->low_colour = 0x1616ea;
    self->food_colour = 0xe1e4ff;
    self->food_low_colour = 0x1616ea;
    self->ap_colour = 0x60c0ff;
    return self;
}

void ui_gage_destruct(struct UIGage *self)
{
    self->base.ops = &ui_gage_ops;
    ui_window_destruct(&self->base);
}

void ui_gage_destroy(struct UIWindow *window)
{
    struct UIGage *self = (struct UIGage *)window;
    ui_gage_destruct(self);
    mem_free(self);
}

static struct UIPcGage *ui_pc_gage_construct(struct UIPcGage *self)
{
    ui_gage_construct(&self->gage);
    self->gage.base.ops = &ui_pc_gage_ops;
    self->hp = 0;
    self->max_hp = 0;
    self->sp = 0;
    self->max_sp = 0;
    self->two_bars = 0;
    return self;
}

struct UIPcGage *ui_pc_gage_new(void)
{
    struct UIPcGage *self = mem_alloc(sizeof *self);

    return self ? ui_pc_gage_construct(self) : NULL;
}

void ui_pc_gage_destruct(struct UIPcGage *self)
{
    self->gage.base.ops = &ui_pc_gage_ops;
    ui_gage_destruct(&self->gage);
}

void ui_pc_gage_destroy(struct UIWindow *window)
{
    struct UIPcGage *self = (struct UIPcGage *)window;
    ui_pc_gage_destruct(self);
    mem_free(self);
}

void ui_pc_gage_on_draw(struct UIWindow *window)
{
    struct UIPcGage *self = (struct UIPcGage *)window;
    struct UIWindow *wnd = &self->gage.base;
    float hp_percent, sp_percent;
    int half;

    ui_window_clear_canvas(wnd, self->gage.clear_colour);
    if (!self->two_bars) {
        ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, self->gage.back_colour);
        if (!self->max_hp)
            return;
        hp_percent = (float)(self->hp * 100 / self->max_hp);
        if (hp_percent == 0.0f)
            return;
        ui_window_fill_rect(wnd, 1, 1, (int)((float)wnd->width * hp_percent / 100.0f - 2.0f), wnd->height - 2,
                          GAUGE_LOW_PERCENT > hp_percent ? self->gage.low_colour : self->gage.bar_colour);
        return;
    }
    half = (wnd->height - 3) / 2;
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, half, self->gage.back_colour);
    ui_window_fill_rect(wnd, 1, wnd->height / 2 + 1, wnd->width - 2, (wnd->height - 3) / 2,
                      self->gage.back_colour);
    if (!self->max_hp || !self->max_sp)
        return;
    hp_percent = (float)(self->hp * 100 / self->max_hp);
    sp_percent = (float)(self->sp * 100 / self->max_sp);
    if (hp_percent != 0.0f)
        ui_window_fill_rect(wnd, 1, 1, (int)((float)wnd->width * hp_percent / 100.0f - 2.0f),
                          (wnd->height - 3) / 2,
                          GAUGE_LOW_PERCENT > hp_percent ? self->gage.low_colour : self->gage.bar_colour);
    if (sp_percent != 0.0f)
        ui_window_fill_rect(wnd, 1, wnd->height / 2 + 1, (int)((float)wnd->width * sp_percent / 100.0f - 2.0f),
                          (wnd->height - 3) / 2, self->gage.second_colour);
}

struct UIMonsterGage {
    struct UIPcGage pc;
    int low_percent;
};
SIZE_ASSERT(UIMonsterGage, 0xb4);

int ui_monster_gauge_hp(const struct UIWindow *window)
{
    const struct UIMonsterGage *gauge = (const struct UIMonsterGage *)window;
    return gauge ? gauge->pc.hp : -1;
}

int ui_monster_gauge_max_hp(const struct UIWindow *window)
{
    const struct UIMonsterGage *gauge = (const struct UIMonsterGage *)window;
    return gauge ? gauge->pc.max_hp : -1;
}

int ui_monster_gauge_two_bars(const struct UIWindow *window)
{
    const struct UIMonsterGage *gauge = (const struct UIMonsterGage *)window;
    return gauge ? gauge->pc.two_bars : 0;
}

void ui_monster_gage_destruct(struct UIMonsterGage *self)
{
    self->pc.gage.base.ops = &ui_monster_gage_ops;
    ui_pc_gage_destruct(&self->pc);
}

void ui_monster_gage_destroy(struct UIWindow *window)
{
    struct UIMonsterGage *self = (struct UIMonsterGage *)window;
    ui_monster_gage_destruct(self);
    mem_free(self);
}

void ui_monster_gage_on_draw(struct UIWindow *window)
{
    struct UIMonsterGage *self = (struct UIMonsterGage *)window;
    struct UIWindow *wnd = &self->pc.gage.base;
    float percent;

    ui_window_clear_canvas(wnd, self->pc.gage.clear_colour);
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, self->pc.gage.back_colour);
    if (self->pc.max_hp <= 0)
        return;
    percent = (float)self->pc.hp * 100.0f / (float)self->pc.max_hp;
    if (0.0f >= percent)
        return;
    ui_window_fill_rect(wnd, 1, 1, (int)((float)(wnd->width - 2) * percent / 100.0f), wnd->height - 2,
                      (float)self->low_percent > percent ? self->pc.gage.low_colour : self->pc.gage.bar_colour);
}

struct UIRechargeGage {
    struct UIGage gage;
    int value;
    int maximum;
};
SIZE_ASSERT(UIRechargeGage, 0xa4);

void ui_recharge_gage_destroy(struct UIWindow *window)
{
    struct UIRechargeGage *self = (struct UIRechargeGage *)window;
    self->gage.base.ops = &ui_recharge_gage_ops;
    ui_gage_destruct(&self->gage);
    mem_free(self);
}

void ui_recharge_gage_on_draw(struct UIWindow *window)
{
    struct UIRechargeGage *self = (struct UIRechargeGage *)window;
    struct UIWindow *wnd = &self->gage.base;
    float percent;

    ui_window_clear_canvas(wnd, self->gage.clear_colour);
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, self->gage.back_colour);
    if (!self->maximum)
        return;
    percent = (float)(self->value * 100 / self->maximum);
    if (percent == 0.0f)
        return;
    ui_window_fill_rect(wnd, 1, 1, (int)((float)wnd->width * percent / 100.0f - 2.0f), wnd->height - 2,
                      self->gage.bar_colour);
}

enum {
    PLAYER_GAGE_HP, PLAYER_GAGE_MAX_HP,
    PLAYER_GAGE_SP, PLAYER_GAGE_MAX_SP,
    PLAYER_GAGE_FOOD, PLAYER_GAGE_MAX_FOOD,
    PLAYER_GAGE_AP, PLAYER_GAGE_MAX_AP,
    PLAYER_GAGE_GETTERS
};

struct UIPlayerGage {
    struct UIGage gage;
    int layout;
    struct GaugeValue getters[PLAYER_GAGE_GETTERS];
};
OFFSET_ASSERT(UIPlayerGage, getters, 0xa0);

static struct UIPlayerGage *ui_player_gage_construct(struct UIPlayerGage *self)
{
    int i;

    ui_gage_construct(&self->gage);
    self->gage.base.ops = &ui_player_gage_ops;
    self->layout = 0;
    for (i = 0; i < PLAYER_GAGE_GETTERS; i++)
        self->getters[i] = (struct GaugeValue){0};
    return self;
}

void ui_player_gage_destruct(struct UIPlayerGage *self)
{
    self->gage.base.ops = &ui_player_gage_ops;
    self->gage.base.ops = &ui_gage_ops;
    ui_window_destruct(&self->gage.base);
}

void ui_player_gage_destroy(struct UIWindow *window)
{
    struct UIPlayerGage *self = (struct UIPlayerGage *)window;
    ui_player_gage_destruct(self);
    mem_free(self);
}

#define GAUGE_MAX(a, b) ((a) < (b) ? (b) : (a))

static float player_ratio(struct UIPlayerGage *self, int current, int int_denominator)
{
    struct GaugeValue *cur = &self->getters[current], *max = &self->getters[current + 1];
    __int64 denominator = GAUGE_MAX(GAUGE_MAX(gauge_value_read(max), (__int64)1), gauge_value_read(cur));

    if (int_denominator)
        return (float)((double)gauge_value_read(cur) / (double)(int)denominator);
    return (float)((double)gauge_value_read(cur) / (double)denominator);
}

static void player_bar(struct UIPlayerGage *self, int y, float ratio, COLORREF colour)
{
    struct UIWindow *wnd = &self->gage.base;

    ui_window_fill_rect(wnd, 1, y, wnd->width - 2, 3, self->gage.back_colour);
    ui_window_fill_rect(wnd, 1, y, (int)((float)(wnd->width - 2) * ratio), 3, colour);
}

static void player_hp_bar(struct UIPlayerGage *self, int int_denominator)
{
    float ratio = player_ratio(self, PLAYER_GAGE_HP, int_denominator);

    player_bar(self, 1, ratio, 0.25f > ratio ? self->gage.low_colour : self->gage.bar_colour);
}

static void player_food_bar(struct UIPlayerGage *self, int y)
{
    float ratio = player_ratio(self, PLAYER_GAGE_FOOD, 0);

    player_bar(self, y, ratio, 0.25f > ratio ? self->gage.food_low_colour : self->gage.food_colour);
}

static int player_has(struct UIPlayerGage *self, int first, int last)
{
    int i;

    for (i = first; i <= last; i++)
        if (!self->getters[i].read)
            return 0;
    return 1;
}

void ui_player_gage_on_draw(struct UIWindow *window)
{
    struct UIPlayerGage *self = (struct UIPlayerGage *)window;
    ui_window_clear_canvas(&self->gage.base, self->gage.clear_colour);
    switch (self->layout) {
    case 7:
        if (player_has(self, PLAYER_GAGE_FOOD, PLAYER_GAGE_MAX_FOOD))
            player_food_bar(self, 1);
        break;
    case 8:
        if (!player_has(self, PLAYER_GAGE_HP, PLAYER_GAGE_MAX_FOOD))
            break;
        player_hp_bar(self, 0);
        player_bar(self, 5, player_ratio(self, PLAYER_GAGE_SP, 0), self->gage.second_colour);
        player_food_bar(self, 9);
        break;
    case 9:
    case 10:
        if (!player_has(self, PLAYER_GAGE_HP, PLAYER_GAGE_MAX_SP))
            break;
        player_hp_bar(self, 0);
        player_bar(self, 5, player_ratio(self, PLAYER_GAGE_SP, 1), self->gage.second_colour);
        break;
    case 13:
    case 14:
        if (player_has(self, PLAYER_GAGE_HP, PLAYER_GAGE_MAX_HP))
            player_hp_bar(self, 0);
        break;
    default:
        if (!player_has(self, PLAYER_GAGE_HP, PLAYER_GAGE_MAX_SP))
            break;
        player_hp_bar(self, 1);
        player_bar(self, 5, player_ratio(self, PLAYER_GAGE_SP, 1), self->gage.second_colour);
        if (player_has(self, PLAYER_GAGE_AP, PLAYER_GAGE_MAX_AP))
            player_bar(self, 9, player_ratio(self, PLAYER_GAGE_AP, 1), self->gage.ap_colour);
        break;
    }
}

void ui_gage_set_back_colour(struct UIGage *self, COLORREF colour)
{
    self->back_colour = colour;
}

void ui_pc_gage_set_hp(struct UIPcGage *self, int hp, int max_hp)
{
    self->hp = hp;
    self->max_hp = max_hp;
    ui_window_invalidate_base(&self->gage.base);
}

void ui_pc_gage_set_sp(struct UIPcGage *self, int sp, int max_sp)
{
    self->sp = sp;
    self->max_sp = max_sp;
    ui_window_invalidate_base(&self->gage.base);
}

void ui_pc_gage_set_two_bars(struct UIPcGage *self, int two_bars)
{
    self->two_bars = two_bars;
}

void ui_recharge_gage_set_amount(struct UIRechargeGage *self, int value, int maximum)
{
    self->value = value;
    self->maximum = maximum;
    ui_window_invalidate_base(&self->gage.base);
}

struct UIWindow *ui_player_gauge_create(void)
{
    struct UIPlayerGage *self = mem_alloc(sizeof(*self));
    return self ? &ui_player_gage_construct(self)->gage.base : NULL;
}

void ui_player_gauge_set(struct UIWindow *window, enum PlayerGaugeBar bar,
    struct GaugeValue current, struct GaugeValue maximum)
{
    struct UIPlayerGage *self = (struct UIPlayerGage *)window;
    self->getters[2 * bar] = current;
    self->getters[2 * bar + 1] = maximum;
}

void ui_player_gauge_set_layout(struct UIWindow *window, int layout)
{
    ((struct UIPlayerGage *)window)->layout = layout;
}

static struct UIMonsterGage *ui_monster_gage_construct(struct UIMonsterGage *self)
{
    ui_gage_construct(&self->pc.gage);
    self->pc.gage.base.ops = &ui_pc_gage_ops;
    self->pc.hp = 0;
    self->pc.max_hp = 0;
    self->pc.sp = 0;
    self->pc.max_sp = 0;
    self->pc.two_bars = 0;
    self->pc.gage.base.ops = &ui_monster_gage_ops;
    self->pc.gage.clear_colour = 0x9c1810;
    self->pc.gage.back_colour = 0x424142;
    self->pc.gage.bar_colour = 0xe700ff;
    self->pc.gage.low_colour = 0xffff;
    self->low_percent = 0x19;
    self->pc.two_bars = 1;
    return self;
}

struct UIMonsterGage *ui_monster_gage_new(void)
{
    struct UIMonsterGage *self = mem_alloc(sizeof *self);

    return self ? ui_monster_gage_construct(self) : NULL;
}

static struct UIRechargeGage *ui_recharge_gage_construct(struct UIRechargeGage *self)
{
    ui_gage_construct(&self->gage);
    self->gage.base.ops = &ui_recharge_gage_ops;
    self->value = 0;
    self->maximum = 100;
    return self;
}

struct UIRechargeGage *ui_recharge_gage_new(void)
{
    struct UIRechargeGage *self = mem_alloc(sizeof *self);

    return self ? ui_recharge_gage_construct(self) : NULL;
}
