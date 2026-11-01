#include "ui/controls/static.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/controls/controls.h"
#include "ui/game.h"
#include <string.h>
#include "ui/ui_std.h"
#include "ui/controls/controls_std.h"
#include "base.h"
#include "core/skin_path.h"
#include "diag/perf.h"

struct UIStaticText;
struct UIWindow;
const struct UIWindowOps ui_static_text_ops = {
    .destroy = ui_static_text_destroy,
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
    .on_draw = ui_static_text_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_static_text_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_static_text_on_l_btn_dbl_clk,
    .on_mouse_move = ui_static_text_on_mouse_move,
    .on_mouse_hover = ui_static_text_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_static_text_on_mouse_shape,
    .on_l_btn_up = ui_static_text_on_l_btn_up,
    .on_r_btn_down = ui_static_text_on_r_btn_down,
    .on_r_btn_up = ui_static_text_on_r_btn_up,
    .on_wheel = ui_static_text_on_wheel,
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

struct UIStaticBitmap;
void ui_static_bitmap_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_static_bitmap_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_static_bitmap_on_r_btn_down(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui_static_bitmap_ops = {
    .destroy = ui_static_bitmap_destroy,
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
    .on_draw = ui_static_bitmap_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_static_bitmap_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_static_bitmap_on_l_btn_dbl_clk,
    .on_mouse_move = ui_static_bitmap_on_mouse_move,
    .on_mouse_hover = ui_static_bitmap_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_static_bitmap_on_mouse_shape,
    .on_l_btn_up = ui_static_bitmap_on_l_btn_up,
    .on_r_btn_down = ui_static_bitmap_on_r_btn_down,
    .on_r_btn_up = ui_static_bitmap_on_r_btn_up,
    .on_wheel = ui_static_bitmap_on_wheel,
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

struct UIStaticTextLF;
const struct UIWindowOps ui_static_text_lf_ops = {
    .destroy = ui_static_text_lf_destroy,
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
    .on_draw = ui_static_text_lf_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_static_text_lf_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_static_text_lf_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_static_text_lf_on_wheel,
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

#define UI_PASS_TO_PARENT(name, slot)                                           \
    void name(struct UIWindow *self, int x, int y) \
    {                                                                           \
        self->parent->ops->slot(self->parent, self->x + x, self->y + y);                  \
    }

void ui_static_text_destruct(struct UIStaticText *self)
{
    self->base.ops = &ui_static_text_ops;
    string_free(&self->tooltip);
    string_free(&self->text);
    ui_window_destruct(&self->base);
}

void ui_static_text_destroy(struct UIWindow *window)
{
    struct UIStaticText *self = (struct UIStaticText *)window;
    ui_static_text_destruct(self);
    mem_free(self);
}

void ui_static_text_on_draw(struct UIWindow *window)
{
    struct UIStaticText *self = (struct UIStaticText *)window;
    struct UIWindow *wnd = &self->base;

    ui_window_clear_canvas(wnd, self->background);
    if (self->white_shadow)
        ui_window_text_out(wnd, 1, 1, string_cstr(&self->text), 0, self->font_face, self->font_height,
                         RGB(0xff, 0xff, 0xff), 0, 0);
    if (self->outline) {
        ui_window_text_out(wnd, -1, 0, string_cstr(&self->text), 0, self->font_face, self->font_height,
                         self->outline_colour, 0, 0);
        ui_window_text_out(wnd, 1, 0, string_cstr(&self->text), 0, self->font_face, self->font_height,
                         self->outline_colour, 0, 0);
        ui_window_text_out(wnd, 0, 1, string_cstr(&self->text), 0, self->font_face, self->font_height,
                         self->outline_colour, 0, 0);
    }
    ui_window_text_out(wnd, 0, 0, string_cstr(&self->text), 0, self->font_face, self->font_height,
                     RGB(self->r, self->g, self->b), self->font_arg3, self->font_arg4);
}

UI_PASS_TO_PARENT(ui_static_text_on_l_btn_down, on_l_btn_down)
UI_PASS_TO_PARENT(ui_static_text_on_l_btn_dbl_clk, on_l_btn_dbl_clk)

void ui_static_text_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIStaticText *self = (struct UIStaticText *)window;
    struct UIWindow *wnd = &self->base;

    ui_static_text_on_mouse_hover(wnd, x, y);
    ui_window_on_mouse_move(wnd->parent, wnd->x + x, wnd->y + y);
}

void ui_static_text_on_mouse_hover(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIStaticText *self = (struct UIStaticText *)window;
    int tip_x, tip_y;

    if (!self->has_tooltip)
        return;
    tip_x = -6;
    tip_y = -4;
    ui_window_to_screen(&self->base, &tip_x, &tip_y);
    mode_show_tooltip(client_current_mode(), string_cstr(&self->tooltip), tip_x, tip_y, 0xffffffff, 0);
}

UI_PASS_TO_PARENT(ui_static_text_on_mouse_shape, on_mouse_shape)
UI_PASS_TO_PARENT(ui_static_text_on_l_btn_up, on_l_btn_up)
UI_PASS_TO_PARENT(ui_static_text_on_r_btn_down, on_r_btn_down)
UI_PASS_TO_PARENT(ui_static_text_on_r_btn_up, on_r_btn_up)

void ui_static_text_on_wheel(struct UIWindow *window, int delta)
{
    struct UIStaticText *self = (struct UIStaticText *)window;
    ui_window_on_wheel(self->base.parent, delta);
}

struct String *ui_static_text_get_text(struct UIWindow *self, struct String *out)
{
    string_copy(out, &((struct UIStaticText *)self)->text);
    return out;
}

struct UIStaticBitmap *ui_static_bitmap_construct(struct UIStaticBitmap *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_static_bitmap_ops;
    string_init(&self->bitmap);
    string_init(&self->tooltip);
    self->hover_to_parent = 0;
    self->tip_x = 0;
    self->tip_y = -20;
    return self;
}

void ui_static_bitmap_destruct(struct UIStaticBitmap *self)
{
    self->base.ops = &ui_static_bitmap_ops;
    string_free(&self->tooltip);
    string_free(&self->bitmap);
    ui_window_destruct(&self->base);
}

void ui_static_bitmap_destroy(struct UIWindow *window)
{
    struct UIStaticBitmap *self = (struct UIStaticBitmap *)window;
    ui_static_bitmap_destruct(self);
    mem_free(self);
}

void ui_static_bitmap_set_bitmap(struct UIStaticBitmap *self, const char *name)
{
    struct BitmapRes *bitmap;
    const char *path;

    string_assign_cstr(&self->bitmap, name);
    path = string_cstr(&self->bitmap);
    if (g_serviceType)
        path = ui_skin_path(path);
    bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
    self->base.width = bitmap->width;
    self->base.height = bitmap->height;
}

void ui_static_bitmap_on_draw(struct UIWindow *window)
{
    struct UIStaticBitmap *self = (struct UIStaticBitmap *)window;
    if (!self->bitmap.size)
        return;
    switch (self->base.state) {
    case 0:
        ui_window_draw_bitmap(&self->base, 0, 0, ui_skin_bitmap(string_cstr(&self->bitmap)), 0);
        break;
    case 1:
        ui_window_clear_canvas(&self->base, UI_TRANSPARENT);
        break;
    }
}

UI_PASS_TO_PARENT(ui_static_bitmap_on_l_btn_down, on_l_btn_down)
UI_PASS_TO_PARENT(ui_static_bitmap_on_l_btn_dbl_clk, on_l_btn_dbl_clk)
UI_PASS_TO_PARENT(ui_static_bitmap_on_mouse_move, on_mouse_move)

void ui_static_bitmap_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIStaticBitmap *self = (struct UIStaticBitmap *)window;
    if (self->hover_to_parent)
        ui_window_on_mouse_hover(self->base.parent, x, y);
    if (self->tooltip.size) {
        x = self->tip_x;
        y = self->tip_y;
        ui_window_to_screen(&self->base, &x, &y);
        mode_show_tooltip(client_current_mode(), string_cstr(&self->tooltip), x, y, 0xffffffff, 0);
    }
}

UI_PASS_TO_PARENT(ui_static_bitmap_on_mouse_shape, on_mouse_shape)
UI_PASS_TO_PARENT(ui_static_bitmap_on_l_btn_up, on_l_btn_up)
UI_PASS_TO_PARENT(ui_static_bitmap_on_r_btn_down, on_r_btn_down)
UI_PASS_TO_PARENT(ui_static_bitmap_on_r_btn_up, on_r_btn_up)

void ui_static_bitmap_on_wheel(struct UIWindow *window, int delta)
{
    struct UIStaticBitmap *self = (struct UIStaticBitmap *)window;
    ui_window_on_wheel(self->base.parent, delta);
}

void ui_static_text_lf_destruct(struct UIStaticTextLF *self)
{
    self->base.ops = &ui_static_text_lf_ops;
    string_free(&self->text);
    ui_window_destruct(&self->base);
}

void ui_static_text_lf_destroy(struct UIWindow *window)
{
    struct UIStaticTextLF *self = (struct UIStaticTextLF *)window;
    ui_static_text_lf_destruct(self);
    mem_free(self);
}

static void destroy_string(void *value) { string_free(value); }

void ui_static_text_lf_set_text(struct UIStaticTextLF *self, struct String text)
{
    struct List lines, dropped;
    struct String copy;

    string_assign(&self->text, string_cstr(&text), text.size);
    list_init(&lines);
    string_copy(&copy, &text);
    ui_window_wrap_text(&self->base, &dropped, copy, &lines, self->base.width, self->font_face, self->line_height,
                        self->bold);
    list_free(&dropped, NULL);
    ui_window_resize_base(&self->base, self->base.width,
             (int)(lines.size - 1) * self->line_gap + self->line_height * (int)lines.size);
    ui_window_invalidate_base(&self->base);
    list_free(&lines, destroy_string);
    string_free(&text);
}

void ui_static_text_lf_on_draw(struct UIWindow *window)
{
    struct UIStaticTextLF *self = (struct UIStaticTextLF *)window;
    ui_window_clear_canvas(&self->base, UI_TRANSPARENT);
    ui_window_text_out_wrapped(&self->base, 0, 0, &self->text, self->base.width, self->line_gap, self->align,
                            self->font_face, self->line_height, self->colour, self->bold);
}

void ui_static_text_lf_on_l_btn_down(struct UIWindow *self, int x, int y)
{
    if (self->parent)
        ui_window_on_l_btn_down(self->parent, x, y);
}

void ui_static_text_lf_on_l_btn_up(struct UIWindow *self, int x, int y)
{
    if (self->parent)
        ui_window_on_l_btn_up(self->parent, x, y);
}

void ui_static_text_lf_on_wheel(struct UIWindow *self, int delta)
{
    if (self->parent)
        ui_window_on_wheel(self->parent, delta);
}

void ui_static_bitmap_set_tool_tip(struct UIWindow *self, const char *text)
{
    if (text)
        string_assign_cstr(&((struct UIStaticBitmap *)self)->tooltip, text);
}

#define STATIC_TEXT_ELLIPSIS "..."

struct UIWindow *ui_static_text_construct(struct UIWindow *wnd)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;

    ui_window_construct(wnd, 0);
    wnd->ops = &ui_static_text_ops;
    string_init(&self->text);
    string_init(&self->tooltip);
    self->font_height = 12;
    self->font_face = 0;
    self->font_arg3 = 0;
    self->background = UI_TRANSPARENT;
    ui_window_invalidate(wnd);
    self->has_tooltip = 0;
    self->white_shadow = 0;
    self->outline = 0;
    self->r = 0;
    self->g = 0;
    self->b = 0;
    string_clear(&self->text);
    self->outline_colour = 0x969696;
    self->font_arg4 = 0;
    return wnd;
}

void ui_static_text_set_font(struct UIWindow *wnd, int face, int height, BYTE font_arg3,
                                     BYTE font_arg4)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;

    self->font_face = face;
    self->font_height = height;
    self->font_arg3 = font_arg3;
    self->font_arg4 = font_arg4;
}

void ui_static_text_set_text(struct UIWindow *wnd, const char *text, int outline)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;

    string_assign_cstr(&self->text, text);
    string_assign_cstr(&self->tooltip, text);
    self->outline = outline;
    self->has_tooltip = 0;
    ui_window_invalidate(wnd);
}

void ui_static_text_set_text_colour(struct UIWindow *wnd, int red, int green, int blue)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;

    self->r = red;
    self->g = green;
    self->b = blue;
    ui_window_invalidate(wnd);
}

void ui_static_text_set_text_fit(struct UIWindow *wnd, const char *text)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;
    int height;

    self->has_tooltip = 0;
    string_assign_cstr(&self->tooltip, text);
    string_assign_cstr(&self->text, text);
    ui_window_invalidate(wnd);
    height = wnd->height;
    ui_window_resize(wnd,
             ui_window_text_width(wnd, text, 0, self->font_face, self->font_height, self->font_arg3, 0) + 2,
             height);
}

void ui_static_text_set_number(struct UIWindow *wnd, int value)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;
    char text[0x28];

    sprintf(text, "%d", value);
    string_assign_cstr(&self->text, text);
    string_assign_cstr(&self->tooltip, text);
    self->has_tooltip = 0;
    ui_window_invalidate(wnd);
}

void ui_static_text_set_text_ellipsis(struct UIWindow *wnd, const char *text)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;
    struct String cut, shown;
    int width;

    string_assign_cstr(&self->tooltip, text);
    self->has_tooltip = 0;
    if (!wnd->canvas) {
        string_assign_cstr(&self->text, text);
        ui_window_invalidate(wnd);
        return;
    }
    width = ui_window_text_width(wnd, text, 0, self->font_face, self->font_height, self->font_arg3, 0);
    ui_window_text_width(wnd, STATIC_TEXT_ELLIPSIS, 0, self->font_face, self->font_height, self->font_arg3, 0);
    if (wnd->width >= width) {
        string_assign_cstr(&self->text, text);
        ui_window_invalidate(wnd);
        return;
    }
    self->has_tooltip = 1;
    string_assign_cstr(&self->text, text);
    string_init(&cut);
    string_assign_cstr(&cut, text);
    do {
        unsigned n = cut.size;
        int last = n <= 1;

        if (!last) {
            const char *p = cut.data;

            n = (signed char)p[n - 2] < 0 && (signed char)p[n - 1] < 0 ? n - 2 : n - 1;
            string_truncate(&cut, n);
        }
        string_init(&shown);
        string_reserve(&shown, cut.size + 3);
        string_append(&shown, string_cstr(&cut), cut.size);
        string_append(&shown, STATIC_TEXT_ELLIPSIS, 3);
        string_free(&self->text);
        string_move(&self->text, &shown);
        if (last)
            break;
    } while (wnd->width < ui_window_text_width(wnd, string_cstr(&self->text), 0, self->font_face,
                                             self->font_height, self->font_arg3, 0));
    string_free(&cut);
    ui_window_invalidate(wnd);
}

void ui_static_text3_destruct(struct UIStaticText3 *self)
{
    self->st.base.ops = &ui_static_text3_ops;
    ui_static_text_destruct(&self->st);
}

void ui_static_text3_destroy(struct UIWindow *window)
{
    struct UIStaticText3 *self = (struct UIStaticText3 *)window;
    ui_static_text3_destruct(self);
    mem_free(self);
}

void ui_static_text3_on_draw(struct UIWindow *window)
{
    struct UIStaticText3 *self = (struct UIStaticText3 *)window;
    struct UIStaticText *st = &self->st;
    struct UIWindow *wnd = &st->base;
    int x = self->x_offset;

    ui_window_clear_canvas(wnd, st->background);
    if (st->white_shadow)
        ui_window_text_out(wnd, x + 2, 2, string_cstr(&st->text), 0, st->font_face, st->font_height,
                         RGB(0xff, 0xff, 0xff), st->font_arg3, st->font_arg4);
    if (st->outline) {
        ui_window_text_out(wnd, x, 1, string_cstr(&st->text), 0, st->font_face, st->font_height,
                         st->outline_colour, st->font_arg3, st->font_arg4);
        ui_window_text_out(wnd, x + 1, 0, string_cstr(&st->text), 0, st->font_face, st->font_height,
                         st->outline_colour, st->font_arg3, st->font_arg4);
        ui_window_text_out(wnd, x + 2, 1, string_cstr(&st->text), 0, st->font_face, st->font_height,
                         st->outline_colour, st->font_arg3, st->font_arg4);
        ui_window_text_out(wnd, x + 1, 2, string_cstr(&st->text), 0, st->font_face, st->font_height,
                         st->outline_colour, st->font_arg3, st->font_arg4);
    }
    ui_window_text_out(wnd, x + 1, 1, string_cstr(&st->text), 0, st->font_face, st->font_height,
                     RGB((BYTE)st->r, (BYTE)st->g, (BYTE)st->b), st->font_arg3, st->font_arg4);
}

static void static_text_take(struct UIStaticText *self, const char *text, int outline)
{
    string_assign_cstr(&self->text, text);
    string_assign_cstr(&self->tooltip, text);
    self->outline = outline;
    self->has_tooltip = 0;
}

void ui_static_text_set_text_or_shrink2(struct UIWindow *wnd, const char *text, int outline)
{
    struct UIStaticText *self = (struct UIStaticText *)wnd;

    static_text_take(self, text, outline);
    if (wnd->width < ui_window_text_width(wnd, text, 0, 0, self->font_height, self->font_arg3, 0))
        ui_static_text_set_text_ellipsis(wnd, text);
    else
        ui_window_resize(wnd, ui_window_text_width(wnd, text, 0, 0, self->font_height, self->font_arg3, 0),
                 wnd->height);
    ui_window_invalidate(wnd);
}

const struct UIWindowOps ui_static_text3_ops = {
    .destroy = ui_static_text3_destroy,
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
    .on_draw = ui_static_text3_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_static_text_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_static_text_on_l_btn_dbl_clk,
    .on_mouse_move = ui_static_text_on_mouse_move,
    .on_mouse_hover = ui_static_text_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_static_text_on_mouse_shape,
    .on_l_btn_up = ui_static_text_on_l_btn_up,
    .on_r_btn_down = ui_static_text_on_r_btn_down,
    .on_r_btn_up = ui_static_text_on_r_btn_up,
    .on_wheel = ui_static_text_on_wheel,
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
