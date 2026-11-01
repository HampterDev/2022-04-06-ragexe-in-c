#include "ui/chat/balloon.h"
#include "web/name_card_types.h"
#include <stdlib.h>
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "game/context.h"
#include "resources/emblem_types.h"
#include <string.h>
#include "ui/chat/chat.h"
#include "platform/clientinfo.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "fileio/res_mgr.h"
#include "diag/perf.h"

struct UIBalloonText;
struct UIWindow;
void ui_balloon_text_destroy(struct UIWindow *window);
void ui_balloon_text_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_balloon_text_ops = {
    .destroy = ui_balloon_text_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_balloon_text_is_hit_testable,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_balloon_text_on_process,
    .on_draw = ui_balloon_text_on_draw,
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
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct UITransBalloonText;
void ui_trans_balloon_text_destroy(struct UIWindow *window);
void ui_trans_balloon_text_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_trans_balloon_text_ops = {
    .destroy = ui_trans_balloon_text_destroy,
    .resize = ui_trans_balloon_text_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_trans_balloon_text_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_trans_balloon_text_get_backdrop_box,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_balloon_text_is_hit_testable,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_trans_balloon_text_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_balloon_text_on_process,
    .on_draw = ui_trans_balloon_text_on_draw,
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
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct UIAchBalloonText;
void ui_ach_balloon_text_destroy(struct UIWindow *window);
void ui_ach_balloon_text_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_ach_balloon_text_ops = {
    .destroy = ui_ach_balloon_text_destroy,
    .resize = ui_trans_balloon_text_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_trans_balloon_text_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_trans_balloon_text_get_backdrop_box,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_balloon_text_is_hit_testable,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_trans_balloon_text_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_balloon_text_on_process,
    .on_draw = ui_ach_balloon_text_on_draw,
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
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
void ui_char_info_balloon_text_destroy(struct UIWindow *window);
void ui_char_info_balloon_text_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_char_info_balloon_text_ops = {
    .destroy = ui_char_info_balloon_text_destroy,
    .resize = ui_trans_balloon_text_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_trans_balloon_text_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_trans_balloon_text_get_backdrop_box,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_balloon_text_is_hit_testable,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_trans_balloon_text_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_balloon_text_on_process,
    .on_draw = ui_char_info_balloon_text_on_draw,
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
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
void ui_name_balloon_text_destroy(struct UIWindow *window);
void ui_name_balloon_text_on_draw(struct UIWindow *self);
const struct UINameBalloonTextOps ui_name_balloon_text_ops = {
    .base = {
        .destroy = ui_name_balloon_text_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_balloon_text_is_hit_testable,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_window_set_show_base,
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_name_balloon_text_on_process,
        .on_draw = ui_name_balloon_text_on_draw,
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
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_name_balloon_text_set_text,
};

const struct UINameBalloonTextOps ui_vertical_name_balloon_text_ops = {
    .base = {
        .destroy = vertical_name_balloon_deleting_destruct,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_balloon_text_is_hit_testable,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_window_set_show_base,
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_name_balloon_text_on_process,
        .on_draw = ui_vertical_name_balloon_text_on_draw,
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
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_vertical_name_balloon_text_set_text,
};

#define BALLOON_LINE_COUNT(b) ((b)->lines.size)
#define BALLOON_LINE(b, i) (&(b)->lines.items[i])

struct UIBalloonText *ui_balloon_text_construct(struct UIBalloonText *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_balloon_text_ops;
    text_array_init(&self->lines);
    self->colour_codes = 0;
    self->bold = 0;
    self->font_height = 0xc;
    self->base.state_count = 0;
    self->colour = 0xffffff;
    self->outline = 0;
    return self;
}

void ui_balloon_text_destruct(struct UIBalloonText *self)
{
    self->base.ops = &ui_balloon_text_ops;
    text_array_clear(&self->lines);
    ui_window_destruct(&self->base);
}

void ui_balloon_text_destroy(struct UIWindow *window)
{
    struct UIBalloonText *self = (struct UIBalloonText *)window;
    ui_balloon_text_destruct(self);
    mem_free(self);
}

void ui_balloon_text_on_draw(struct UIWindow *window)
{
    struct UIBalloonText *self = (struct UIBalloonText *)window;
    struct UIWindow *wnd = &self->base;
    unsigned i;
    int y = 4;

    ui_window_clear_canvas(wnd, 0xffffff);
    for (i = 0; i < BALLOON_LINE_COUNT(self); i++) {
        ui_window_text_out(wnd, 6, y, owned_text_cstr(BALLOON_LINE(self, i)), 0, 0, self->font_height, self->colour,
                         0, 0);
        y += 4 + ui_window_text_height(wnd, owned_text_cstr(BALLOON_LINE(self, i)), 0, 0, self->font_height, 0,
                                     0);
    }
}

void ui_trans_balloon_text_destruct(struct UITransBalloonText *self)
{
    self->balloon.base.ops = &ui_trans_balloon_text_ops;
    free(self->line_colours.items);
    memset(&self->line_colours, 0, sizeof self->line_colours);
    ui_balloon_text_destruct(&self->balloon);
}

void ui_trans_balloon_text_destroy(struct UIWindow *window)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    ui_trans_balloon_text_destruct(self);
    mem_free(self);
}

static int trans_balloon_face(WORD font_code)
{
    switch (font_code) {
    case 1: return 0x15;
    case 2: return 0x14;
    case 3: return 0x16;
    case 4: return 0x17;
    case 5: return 0x18;
    case 6: return 0x19;
    case 7: return 0x1a;
    case 8: return 0x1b;
    case 9: return 0x1c;
    }
    return 0;
}

void ui_trans_balloon_text_on_draw(struct UIWindow *window)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    struct UIBalloonText *b = &self->balloon;
    struct UIWindow *wnd = &b->base;
    COLORREF colour = b->colour;
    int height = b->font_height, face, y = 4;
    unsigned i;

    if (!b->lines.size)
        return;
    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_draw_round_frame(wnd, 0, 0, wnd->width, wnd->height, self->frame_colour);
    if (b->font_code != 0)
        height = 0xc;
    face = trans_balloon_face(b->font_code);
    for (i = 0; i < BALLOON_LINE_COUNT(b); i++) {
        const char *text;

        if (i < self->line_colours.size
            && self->line_colours.items[i] != 0)
            colour = self->line_colours.items[i];
        else
            colour = b->colour;
        text = owned_text_cstr(BALLOON_LINE(b, i));
        if (b->colour_codes)
            ui_window_text_out_runs(wnd, 6, y, text, 0, &colour, face, height, 1);
        else
            ui_window_draw_shadow_text(wnd, 6, y, text, 0, colour, b->outline, face, height, 0);
        y += 4 + ui_window_text_height(wnd, owned_text_cstr(BALLOON_LINE(b, i)), 0, 0, b->font_height, 0, 0);
    }
}

struct UIAchBalloonText {
    struct UITransBalloonText trans;
    int font_face;
};
SIZE_ASSERT(UIAchBalloonText, 0xd0);

static struct UIAchBalloonText *ui_ach_balloon_text_construct(struct UIAchBalloonText *self)
{
    struct UITransBalloonText *t = &self->trans;

    ui_balloon_text_construct(&t->balloon);
    t->balloon.base.ops = &ui_trans_balloon_text_ops;
    memset(&t->line_colours, 0, sizeof t->line_colours);
    t->frame_colour = 0x5c5c5c;
    t->balloon.base.ops = &ui_ach_balloon_text_ops;
    self->font_face = 0;
    return self;
}

struct UIAchBalloonText *ui_ach_balloon_text_new(void)
{
    struct UIAchBalloonText *self = mem_alloc(sizeof *self);

    return self ? ui_ach_balloon_text_construct(self) : NULL;
}

void ui_ach_balloon_text_destroy(struct UIWindow *window)
{
    struct UIAchBalloonText *self = (struct UIAchBalloonText *)window;
    self->trans.balloon.base.ops = &ui_ach_balloon_text_ops;
    ui_trans_balloon_text_destruct(&self->trans);
    mem_free(self);
}

void ui_ach_balloon_text_on_draw(struct UIWindow *window)
{
    struct UIAchBalloonText *self = (struct UIAchBalloonText *)window;
    struct UIBalloonText *b = &self->trans.balloon;
    struct UIWindow *wnd = &b->base;
    int count, line_height, top, i;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    if (!b->lines.size)
        return;
    count = (int)BALLOON_LINE_COUNT(b);
    line_height = ui_window_text_height(wnd, owned_text_cstr(BALLOON_LINE(b, 0)), BALLOON_LINE(b, 0)->size,
                                      self->font_face, b->font_height, 1, 0) + 4;
    top = (wnd->height - line_height * count + 4) / 2;
    for (i = 0; i < count; i++) {
        struct OwnedText *line = BALLOON_LINE(b, i);
        int width = ui_window_text_width(wnd, owned_text_cstr(line), line->size, self->font_face,
                                       b->font_height, 1, 0);
        int y = i == 0 ? top : top - 4 + line_height * i;

        ui_window_draw_shadow_text(wnd, (wnd->width - width) / 2, y, owned_text_cstr(line), 0, b->colour,
                                b->outline, self->font_face, b->font_height, 1);
    }
}

void ui_char_info_balloon_text_on_draw(struct UIWindow *window)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    struct UIBalloonText *b = &self->balloon;
    struct UIWindow *wnd = &b->base;
    unsigned i;
    int y = 6;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    for (i = 0; i < BALLOON_LINE_COUNT(b); i++) {
        ui_window_draw_shadow_text(wnd, (i & 1) ? 0x32 : 6, y, owned_text_cstr(BALLOON_LINE(b, i)), 0, b->colour,
                                b->outline, 0, b->font_height, 0);
        if (i & 1)
            y += 4 + ui_window_text_height(wnd, owned_text_cstr(BALLOON_LINE(b, i)), 0, 0, b->font_height, 0,
                                         0);
    }
}

void ui_char_info_balloon_text_destroy(struct UIWindow *window)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    self->balloon.base.ops = &ui_char_info_balloon_text_ops;
    ui_trans_balloon_text_destruct(self);
    mem_free(self);
}

#define NAME_MARK_BOSS 1
#define NAME_MARK_MVP 2

struct UINameBalloonText *ui_name_balloon_text_construct(struct UINameBalloonText *self)
{
    ui_balloon_text_construct(&self->balloon);
    self->balloon.base.ops = &ui_name_balloon_text_ops.base;
    string_init(&self->emblem);
    self->mark = 0;
    self->emblem_frame = 0;
    self->balloon.colour = 0xffffff;
    self->balloon.outline = 0;
    self->text_length = 0;
    self->balloon.back_transparent = 0;
    return self;
}

void ui_name_balloon_text_destruct(struct UINameBalloonText *self)
{
    self->balloon.base.ops = &ui_name_balloon_text_ops.base;
    string_free(&self->emblem);
    ui_balloon_text_destruct(&self->balloon);
}

void ui_name_balloon_text_destroy(struct UIWindow *window)
{
    struct UINameBalloonText *self = (struct UINameBalloonText *)window;
    ui_name_balloon_text_destruct(self);
    mem_free(self);
}

struct UIVerticalNameBalloonText *vertical_name_balloon_construct(struct UIVerticalNameBalloonText *self)
{
    ui_name_balloon_text_construct(&self->name);
    self->name.balloon.base.ops = &ui_vertical_name_balloon_text_ops.base;
    return self;
}

void vertical_name_balloon_deleting_destruct(struct UIWindow *window)
{
    ui_name_balloon_text_destruct((struct UINameBalloonText *)window);
    mem_free(window);
}

static void *name_balloon_emblem(struct UINameBalloonText *self)
{
    const char *name = string_cstr(&self->emblem);
    BYTE *res;

    if (text_view_find(text_view(name, self->emblem.size), text_view(".bmp", 4), 0) != STRING_NPOS) {
        res = res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->emblem)));
        if (res)
            game_current_base_mode();
        return res;
    }
    name = string_cstr(&self->emblem);
    if (text_view_find(text_view(name, self->emblem.size), text_view(".gif", 4), 0) != STRING_NPOS)
        return res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->emblem)));
    res = res_mgr_get(g_resMgr(), string_cstr(&self->emblem));
    if (!res)
        return NULL;
    game_current_base_mode();
    return &((struct EmblemRes *)res)->picture.bmp;
}

void ui_name_balloon_text_on_process(struct UIWindow *window)
{
    struct UINameBalloonText *self = (struct UINameBalloonText *)window;
    void *res = object_as_type(res_mgr_get(g_resMgr(), string_cstr(&self->emblem)), OBJECT_EMBLEM_RESOURCE);

    if (res) {
        if (((struct EmblemRes *)res)->picture.changed == 1)
            ui_window_invalidate_base(&self->balloon.base);
        return;
    }
    res = object_as_type(res_mgr_get(g_resMgr(), string_cstr(&self->emblem)), OBJECT_GIF_RESOURCE);
    if (res && ((struct GifRes *)res)->changed == 1)
        ui_window_invalidate_base(&self->balloon.base);
}
void ui_name_balloon_text_on_draw(struct UIWindow *window)
{
    struct UINameBalloonText *self = (struct UINameBalloonText *)window;
    struct UIBalloonText *b = &self->balloon;
    struct UIWindow *wnd = &b->base;
    int x = 4, y = 4;
    unsigned i;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    if (self->emblem_frame == 1) {
        struct BitmapRes *frame = res_mgr_get(g_resMgr(), UI_SKIN "emblem_frame.bmp");

        if (frame) {
            ui_window_draw_bitmap(wnd, 2, (wnd->height - frame->height) / 2, frame, 1);
            x = frame->width + 4;
        }
    }
    if (self->emblem.size != 0 && res_mgr_is_available(g_resMgr(), string_cstr(&self->emblem))) {
        void *emblem = name_balloon_emblem(self);

        if (emblem) {
            ui_window_draw_bitmap(wnd, 4, (wnd->height - 0x18) / 2, emblem, 1);
            if (self->emblem_frame == 0)
                x += 0x1c;
        }
    } else if (self->mark == NAME_MARK_BOSS || self->mark == NAME_MARK_MVP) {
        struct BitmapRes *mark = res_mgr_get(g_resMgr(),
                                               self->mark == NAME_MARK_BOSS ? UI_SKIN "montype_boss.bmp"
                                                                            : UI_SKIN "montype_mvp.bmp");

        if (mark) {
            ui_window_draw_bitmap(wnd, 0, 0, mark, 1);
            x += mark->width;
        }
    }
    for (i = 0; i < BALLOON_LINE_COUNT(b); i++) {
        struct OwnedText *line = BALLOON_LINE(b, i);

        ui_window_text_out_outlined(wnd, x, y, owned_text_cstr(line), line->size, b->colour, b->outline, 0,
                                 g_nameFontHeight, g_showNameB, 0);
        y += 2 + g_nameFontHeight;
    }
}

static BOOL name_balloon_set_line(struct UIBalloonText *b, const char *text)
{
    struct TextArray replacement = {0};
    if (b->lines.size && !strcmp(text_array_at(&b->lines, 0).data, text)) return FALSE;
    if (!text_array_append(&replacement, text_view_cstr(text))) abort();
    text_array_clear(&b->lines);
    b->lines = replacement;
    return TRUE;
}

void ui_name_balloon_text_set_text(struct UINameBalloonText *self, const char *text)
{
    struct UIBalloonText *b = &self->balloon;
    struct OwnedText *line;
    int length;

    if (!name_balloon_set_line(b, text))
        return;
    line = &b->lines.items[b->lines.size - 1];
    length = ui_window_text_width(&b->base, owned_text_cstr(line), line->size, 0, g_nameFontHeight,
                                g_showNameB, 0) + 8;
    if (length <= 0x20)
        length = 0x20;
    self->text_length = length;
    ui_window_resize_base(&self->balloon.base, length, g_nameFontHeight + 8);
    ui_window_invalidate_base(&self->balloon.base);
}

void ui_vertical_name_balloon_text_on_draw(struct UIWindow *window)
{
    struct UIVerticalNameBalloonText *self = (struct UIVerticalNameBalloonText *)window;
    struct UINameBalloonText *n = &self->name;
    struct UIBalloonText *b = &n->balloon;
    struct UIWindow *wnd = &b->base;
    int x = 4, y = 4;
    unsigned i;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    if (n->emblem.size != 0 && res_mgr_is_available(g_resMgr(), string_cstr(&n->emblem))) {
        BYTE *res = res_mgr_get(g_resMgr(), string_cstr(&n->emblem));

        if (res) {
            ui_window_draw_bitmap(wnd, (wnd->width - 0x18) / 2, 0, &((struct EmblemRes *)res)->picture.bmp, 1);
            y = 0x1c;
        }
    }
    for (i = 0; i < BALLOON_LINE_COUNT(b); i++) {
        struct OwnedText *line = BALLOON_LINE(b, i);

        ui_window_draw_text_outlined_vertical(wnd, x, y, owned_text_cstr(line), line->size, b->colour, b->outline, 0,
                   g_nameFontHeight, g_showNameB);
        x += g_nameFontHeight + 2;
    }
}

void ui_vertical_name_balloon_text_set_text(struct UINameBalloonText *name, const char *text)
{
    struct UIVerticalNameBalloonText *self = (struct UIVerticalNameBalloonText *)name;
    struct UIBalloonText *b = &self->name.balloon;
    struct OwnedText *line;
    int length;

    if (!name_balloon_set_line(b, text))
        return;
    line = &b->lines.items[b->lines.size - 1];
    length = ui_window_vertical_text_length(&b->base, owned_text_cstr(line), 0, 0, g_nameFontHeight,
                                         g_showNameB) + 8;
    if (length <= 0x20)
        length = 0x20;
    self->text_length = length;
    ui_window_resize_base(&self->name.balloon.base, g_nameFontHeight + 8, length);
    ui_window_invalidate_base(&self->name.balloon.base);
}

static void line_colour_push(struct BalloonLineColours *colours, COLORREF colour)
{
    if (colours->size == colours->capacity) {
        size_t capacity = colours->capacity ? colours->capacity * 2 : 8;
        COLORREF *items;
        if (capacity < colours->capacity || capacity > (size_t)-1 / sizeof *items) abort();
        items = realloc(colours->items, capacity * sizeof *items);
        if (!items) abort();
        colours->items = items;
        colours->capacity = capacity;
    }
    colours->items[colours->size++] = colour;
}

static void balloon_clear_lines(struct UIBalloonText *b)
{
    text_array_resize(&b->lines, 0);
}

void ui_balloon_text_fit(struct UIBalloonText *self)
{
    int width = 0, height = 4;
    unsigned i;

    for (i = 0; i < BALLOON_LINE_COUNT(self); i++) {
        const char *line = owned_text_cstr(BALLOON_LINE(self, i));
        int w = ui_window_text_width(&self->base, line, 0, trans_balloon_face(self->font_code), self->font_height,
                                   0, 0);

        if (width <= w)
            width = w;
        height += 4 + ui_window_text_height(&self->base, line, 0, self->font_code == 3 ? 0x16 : 0,
                                          self->font_height, 0, 0);
    }
    ui_window_resize(&self->base, width + 0xc, height);
    ui_window_invalidate_base(&self->base);
}

void ui_balloon_text_set_text(struct UIBalloonText *self, const char *text, int line_chars)
{
    struct TextArray replacement = {0};
    if (strlen(text) == 0)
        return;
    if (self->colour_codes)
        text_wrap_bytes(text, &replacement, line_chars, 0);
    else
        text_split_lines(text, &replacement, line_chars);
    text_array_clear(&self->lines);
    self->lines = replacement;
    if (BALLOON_LINE_COUNT(self) != 0)
        ui_balloon_text_fit(self);
}

void ui_balloon_text_set_lines(struct UIBalloonText *self, const char *text, int line_chars)
{
    struct TextArray replacement = {0};
    if (strlen(text) == 0)
        return;
    text_split_lines(text, &replacement, line_chars);
    text_array_clear(&self->lines);
    self->lines = replacement;
}

void ui_balloon_text_set_font_height(struct UIBalloonText *self, int font_height)
{
    self->font_height = font_height;
    ui_balloon_text_fit(self);
}

struct UIWindow *ui_trans_balloon_text_construct(struct UIWindow *wnd)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)wnd;

    ui_balloon_text_construct(&self->balloon);
    wnd->ops = &ui_trans_balloon_text_ops;
    memset(&self->line_colours, 0, sizeof self->line_colours);
    self->frame_colour = 0x5c5c5c;
    return wnd;
}

void ui_trans_balloon_text_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    struct UIWindow *wnd = &self->balloon.base;

    self->backdrop.x = wnd->x;
    self->backdrop.y = wnd->y;
    self->backdrop.width = wnd->width;
    self->backdrop.height = wnd->height;
    self->backdrop.draw_edge = 0;
    self->backdrop.argb = 0x66000000;
    self->backdrop.edge_argb = 0;
    self->balloon.font_code = 0;
}

void ui_trans_balloon_text_add_line(struct UITransBalloonText *self, const char *text)
{
    if (!text_array_append(&self->balloon.lines, text_view_cstr(text))) abort();
    ui_balloon_text_fit(&self->balloon);
}

void ui_trans_balloon_text_add_colored_line(struct UITransBalloonText *self, const char *text, COLORREF colour)
{
    ui_trans_balloon_text_add_line(self, text);
    line_colour_push(&self->line_colours, colour);
}

void ui_trans_balloon_text_set_first_line(struct UITransBalloonText *self, const char *text, COLORREF colour)
{
    struct TextArray replacement = {0};
    if (!text_array_append(&replacement, text_view_cstr(text))) abort();
    text_array_clear(&self->balloon.lines);
    self->balloon.lines = replacement;
    self->line_colours.size = 0;
    ui_balloon_text_fit(&self->balloon);
    line_colour_push(&self->line_colours, colour);
}

void ui_trans_balloon_text_set_line(struct UITransBalloonText *self, const char *text, int index)
{
    struct UIBalloonText *b = &self->balloon;
    struct OwnedText *line;

    if (!b->lines.size)
        return;
    if (index == -1)
        index = (int)BALLOON_LINE_COUNT(b) - 1;
    if (index < 0 || (size_t)index >= b->lines.size) return;
    line = BALLOON_LINE(b, index);
    if (line->size == 0)
        return;
    if (!owned_text_assign(line, text_view_cstr(text))) abort();
    ui_window_invalidate_base(&self->balloon.base);
}

void ui_color_trans_balloon_text_on_draw(struct UIWindow *window)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    struct UIWindow *wnd = &self->balloon.base;
    COLORREF colour = (COLORREF)(UINT_PTR)self;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_text_out_runs(wnd, 6, 4, text_array_at(&self->balloon.lines, 0).data, 0, &colour, 0, 0xc, 0);
}

static const char name_bracket_format[] = "%s『%s』";

#define NAME_TEXT_BUF 0x104

static void name_copy_text(char *buf, const struct String *text)
{
    memset(buf, 0, NAME_TEXT_BUF);
    strncpy_s(buf, NAME_TEXT_BUF, string_cstr(text), text->size);
}

static void name_add_line(struct UIBalloonText *b, const char *text)
{
    if (!text_array_append(&b->lines, text_view_cstr(text))) abort();
}

void ui_name_balloon_text_set_name_text(struct UINameBalloonText *self, const struct ActorNameCard *info,
                                              BYTE strip_hash, BYTE mark)
{
    struct UIBalloonText *b = &self->balloon;
    struct String name;
    char buf[NAME_TEXT_BUF], buf2[NAME_TEXT_BUF], line[NAME_TEXT_BUF];
    int width = 0x20, height;
    size_t i;

    balloon_clear_lines(b);
    string_copy(&name, &info->name);
    if (strip_hash && name.size) {
        const char *hash = memchr(name.data, '#', name.size);

        if (hash)
            string_truncate(&name, (size_t)(hash - name.data));
    }
    if (g_showNameA) {
        if (string_cstr(&info->npc_title)[0])
            name_add_line(b, string_cstr(&info->npc_title));
        if (string_cstr(&info->position)[0] == 0) {
            self->text_length = 0;
        } else {
            name_copy_text(buf, &info->position);
            name_add_line(b, buf);
        }
        name_add_line(b, string_cstr(&name));
    } else {
        if (string_cstr(&info->npc_title)[0])
            name_add_line(b, string_cstr(&info->npc_title));
        if (string_cstr(&info->achievement_title)[0]) {
            struct String full;

            string_init(&full);
            string_format(&full, "[%s] %s", string_cstr(&info->achievement_title), string_cstr(&name));
            string_assign(&name, string_cstr(&full), full.size);
            string_free(&full);
        }
        if (string_cstr(&info->party)[0] == 0) {
            name_add_line(b, string_cstr(&name));
        } else {
            name_copy_text(buf, &info->party);
            memset(line, 0, sizeof line);
            sprintf_s(line, sizeof line, g_serviceType ? "%s (%s)" : name_bracket_format, string_cstr(&name), buf);
            name_add_line(b, line);
        }
        if (string_cstr(&info->guild)[0]) {
            name_copy_text(buf2, &info->guild);
            name_copy_text(buf, &info->position);
            memset(line, 0, sizeof line);
            sprintf_s(line, sizeof line, g_serviceType ? "%s [%s]" : name_bracket_format, buf2, buf);
            name_add_line(b, line);
        }
    }
    for (i = 0; i < b->lines.size; i++) {
        int w = ui_window_text_width(&b->base, text_array_at(&b->lines, i).data, 0, 0, g_nameFontHeight, g_showNameB, 0) + 8;

        if (w > width)
            width = w;
    }
    self->text_length = width;
    self->mark = mark;
    height = (int)BALLOON_LINE_COUNT(b) * (g_nameFontHeight + 2) + 6;
    if (mark && info->group_id == 0) {
        const char *path = mark == NAME_MARK_BOSS  ? UI_SKIN "montype_boss.bmp"
                           : mark == NAME_MARK_MVP ? UI_SKIN "montype_mvp.bmp"
                                                   : NULL;
        struct BitmapRes *bitmap = path ? res_mgr_get(g_resMgr(), path) : NULL;

        if (bitmap) {
            if ((unsigned)height <= (unsigned)bitmap->height)
                height = bitmap->height;
            ui_window_resize_base(&self->balloon.base, bitmap->width + self->text_length, height);
        }
    } else {
        ui_window_resize_base(&self->balloon.base, width, height);
    }
    ui_window_invalidate_base(&self->balloon.base);
    string_free(&name);
}

void ui_name_balloon_text_set_emblem(struct UINameBalloonText *self, const char *name, BYTE frame)
{
    struct UIWindow *wnd = &self->balloon.base;
    struct String path, arg;
    int pos;

    string_clear(&self->emblem);
    self->emblem_frame = frame;
    string_init(&path);
    string_assign_cstr(&path, name);
    if (path.size == 0 && !frame) {
        string_free(&path);
        return;
    }
    pos = string_find(&path, ".bmp", 0);
    if (pos != -1) {
        string_replace(&path, pos, path.size - pos, ".gif", 4);
        string_copy(&arg, &path);
        if (!res_mgr_exists(g_resMgr(), arg))
            string_assign_cstr(&path, name);
    }
    string_copy(&arg, &path);
    if (res_mgr_exists(g_resMgr(), arg)) {
        string_assign(&self->emblem, string_cstr(&path), path.size);
        if (!frame) {
            ui_window_resize_base(wnd, self->text_length + 0x19, wnd->height > 0x18 ? wnd->height : 0x18);
            ui_window_invalidate_base(wnd);
            string_free(&path);
            return;
        }
    } else if (!frame) {
        string_free(&path);
        return;
    }
    if (frame == 1)
        ui_window_resize_base(wnd, self->text_length + 0x1d, wnd->height > 0x1c ? wnd->height : 0x1c);
    ui_window_invalidate_base(wnd);
    string_free(&path);
}

void ui_trans_balloon_text_resize(struct UIWindow *window, int width, int height)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    ui_window_resize_base(&self->balloon.base, width, height);
    self->backdrop.width = self->balloon.base.width;
    self->backdrop.height = self->balloon.base.height;
}

void ui_trans_balloon_text_move(struct UIWindow *window, int x, int y)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;
    ui_window_move_base(&self->balloon.base, x, y);
    self->backdrop.x = self->balloon.base.x;
    self->backdrop.y = self->balloon.base.y;
}

BYTE ui_trans_balloon_text_get_backdrop_box(struct UIWindow *window, struct UIBackdropBox *out)
{
    struct UITransBalloonText *self = (struct UITransBalloonText *)window;

    *out = self->backdrop;
    return 1;
}

struct TextView ui_trans_balloon_text_get_line(const struct UITransBalloonText *self, size_t index)
{
    return text_array_at(&self->balloon.lines, index);
}
