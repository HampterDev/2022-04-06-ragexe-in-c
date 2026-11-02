#include "ui/controls/check_radio.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "diag/perf.h"

enum {
    RADIO_LOOK_UP = 0,
    RADIO_LOOK_HOVER = 1,
    RADIO_LOOK_CHOSEN = 2,
    RADIO_LOOK_DISABLED = 3,
};

struct UIRadioLabelStyle {
    int look;
    int x, y;
    int font_height;
    BYTE bold;
    COLORREF colour;
    BYTE shadow;
};

struct UIBmpRadioBtn {
    struct UIWindow base;
    struct String bitmaps[4];
    BYTE hover;
    struct String tooltip;
    BYTE disabled;
    int value;
    struct UIRadioLabelStyle *styles;
    size_t style_count, style_capacity;
    struct String label;
};
const struct UIWindowOps ui_bmp_radio_btn_ops = {
    .destroy = ui_bmp_radio_btn_destroy,
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
    .on_draw = ui_bmp_radio_btn_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_bmp_radio_btn_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_bmp_radio_btn_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_bmp_radio_btn_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_bmp_radio_btn_hover_on,
    .on_hover_leave = ui_bmp_radio_btn_hover_off,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define DEFAULT_FONT_HEIGHT 0xc
#define TOOLTIP_RISE (-20)
#define DISABLED_SHADOW 0xe2e2e2
#define DISABLED_TEXT 0x6f6f6f
#define CHOSEN_SHADOW 0xffffff
#define HOVER_SHADOW 0xc7a796
#define UP_SHADOW 0xd7d7d7

void ui_bmp_radio_btn_destroy(struct UIWindow *window)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)window;
    int i;

    self->base.ops = &ui_bmp_radio_btn_ops;
    string_free(&self->label);
    free(self->styles);
    string_free(&self->tooltip);
    for (i = 3; i >= 0; i--)
        string_free(&self->bitmaps[i]);
    ui_window_destruct(&self->base);
    mem_free(self);
}

static void label_out(struct UIBmpRadioBtn *self, int x, int y, int height, COLORREF colour, int bold)
{
    ui_window_text_out(&self->base, x, y, string_cstr(&self->label), 0, 0, height, colour, bold, 0);
}

void ui_bmp_radio_btn_on_draw(struct UIWindow *window)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)window;
    struct String *bitmap;
    struct UIRadioLabelStyle *s, *end;
    int look, cx, cy;
    COLORREF shadow;

    ui_window_clear_canvas(&self->base, UI_TRANSPARENT);
    if (self->disabled == 1)
        bitmap = &self->bitmaps[RADIO_LOOK_DISABLED];
    else if (self->base.state == 1)
        bitmap = &self->bitmaps[RADIO_LOOK_CHOSEN];
    else if (self->hover == 1)
        bitmap = &self->bitmaps[RADIO_LOOK_HOVER];
    else
        bitmap = &self->bitmaps[RADIO_LOOK_UP];
    ui_window_draw_bitmap(&self->base, 0, 0, ui_skin_bitmap(string_cstr(bitmap)), 0);
    if (!self->label.size)
        return;

    if (self->disabled == 1) {
        look = RADIO_LOOK_DISABLED;
        shadow = DISABLED_SHADOW;
    } else if (self->base.state == 1) {
        look = RADIO_LOOK_CHOSEN;
        shadow = CHOSEN_SHADOW;
    } else if (self->hover == 1) {
        look = RADIO_LOOK_HOVER;
        shadow = HOVER_SHADOW;
    } else {
        look = RADIO_LOOK_UP;
        shadow = UP_SHADOW;
    }
    end = self->style_count ? self->styles + self->style_count : self->styles;
    for (s = self->styles; s != end && s->look != look; s++)
        ;

    if (s != end) {
        if (look >= RADIO_LOOK_CHOSEN) {
            if (s->shadow == 1)
                label_out(self, s->x, s->y, s->font_height, shadow, look == RADIO_LOOK_CHOSEN ? s->bold : 0);
            label_out(self, s->x + 1, s->y + 1, s->font_height, s->colour, s->bold);
        } else {
            if (s->shadow == 1)
                label_out(self, s->x + 1, s->y + 1, s->font_height, shadow, s->bold);
            label_out(self, s->x, s->y, s->font_height, s->colour, s->bold);
        }
        return;
    }
    cx = (int)((double)(self->base.width - ui_window_text_width(&self->base, string_cstr(&self->label), 0, 0,
                                                              DEFAULT_FONT_HEIGHT, 0, 0)) * 0.5);
    cy = (int)((double)(self->base.height - ui_window_text_height(&self->base, string_cstr(&self->label), 0,
                                                                0, DEFAULT_FONT_HEIGHT, 0, 0)) * 0.5);
    if (look >= RADIO_LOOK_CHOSEN) {
        label_out(self, cx, cy, DEFAULT_FONT_HEIGHT, shadow, 0);
        label_out(self, cx + 1, cy + 1, DEFAULT_FONT_HEIGHT, look == RADIO_LOOK_DISABLED ? DISABLED_TEXT : 0, 0);
    } else {
        label_out(self, cx + 1, cy + 1, DEFAULT_FONT_HEIGHT, shadow, 0);
        label_out(self, cx, cy, DEFAULT_FONT_HEIGHT, 0, 0);
    }
}

void ui_bmp_radio_btn_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)window;
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, &self->base, UI_MSG_COMMAND, self->base.id, self->value, 0, 0);
}

void ui_bmp_radio_btn_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)window;
    int sx = 0, sy = TOOLTIP_RISE;

    if (!self->tooltip.size)
        return;
    ui_window_to_screen(&self->base, &sx, &sy);
    mode_show_tooltip(client_current_mode(), string_cstr(&self->tooltip), sx, sy, 0xffffffff, 0);
}

void ui_bmp_radio_btn_hover_on(struct UIWindow *window)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)window;
    self->hover = 1;
    ui_window_invalidate_base(&self->base);
}

void ui_bmp_radio_btn_hover_off(struct UIWindow *window)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)window;
    self->hover = 0;
    ui_window_invalidate_base(&self->base);
}

int ui_bmp_radio_btn_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)window;
    if (msg == UI_MSG_SET_STATE) {
        self->base.state = val1;
        ui_window_invalidate_base(&self->base);
    }
    return 0;
}

BYTE ui_bmp_radio_btn_set_bitmap(struct UIWindow *wnd, const char *path, int look)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)wnd;
    struct BitmapRes *bmp = res_mgr_get(g_resMgr(), path);

    if (!bmp)
        return 0;
    if (bmp->width > self->base.width)
        self->base.width = bmp->width;
    if (bmp->height > self->base.height)
        self->base.height = bmp->height;
    ui_window_resize_base(&self->base, self->base.width, self->base.height);
    if ((unsigned)look <= RADIO_LOOK_DISABLED)
        string_assign_cstr(&self->bitmaps[look], path);
    return 1;
}

#define RADIO_CENTRED (-1000)

void ui_bmp_radio_btn_set_text_style(struct UIWindow *wnd, int look, int height,
                                           COLORREF colour, BYTE bold, BYTE shadow, int x, int y)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)wnd;
    struct UIRadioLabelStyle style;
    size_t i;

    if (x == RADIO_CENTRED)
        x = (int)((double)(self->base.width - ui_window_text_width(wnd, string_cstr(&self->label),
                                                                 self->label.size, 0, height, bold, 0)) * 0.5);
    if (y == RADIO_CENTRED)
        y = (int)((double)(self->base.height - ui_window_text_height(wnd, string_cstr(&self->label),
                                                                   self->label.size, 0, height, bold, 0)) * 0.5);
    style.look = look;
    style.x = x;
    style.y = y;
    style.font_height = height;
    style.bold = bold;
    style.colour = colour;
    style.shadow = shadow;

    for (i = 0; i < self->style_count; ++i) {
        if (self->styles[i].look == look) {
            self->styles[i] = style;
            return;
        }
    }
    if (self->style_count == self->style_capacity) {
        size_t limit = (size_t)-1 / sizeof *self->styles;
        size_t capacity;
        struct UIRadioLabelStyle *styles;
        if (self->style_count == limit)
            abort();
        capacity = self->style_capacity > limit / 2 ? limit : self->style_capacity * 2;
        if (capacity < 4)
            capacity = 4;
        styles = realloc(self->styles, capacity * sizeof *styles);
        if (!styles)
            abort();
        self->styles = styles;
        self->style_capacity = capacity;
    }
    self->styles[self->style_count++] = style;
}

static struct UIWindow *ui_bmp_radio_btn_construct(struct UIWindow *wnd, int value)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)wnd;
    int i;

    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_bmp_radio_btn_ops;
    for (i = 0; i < 4; i++)
        string_init(&self->bitmaps[i]);
    string_init(&self->tooltip);
    self->styles = NULL;
    self->style_count = 0;
    self->style_capacity = 0;
    string_init(&self->label);
    self->base.state = 0;
    self->base.id = 0xcf;
    self->hover = 0;
    self->disabled = 0;
    self->value = value;
    return wnd;
}

struct UIWindow *ui_bmp_radio_btn_create(int value)
{
    struct UIBmpRadioBtn *self = mem_alloc(sizeof *self);
    return self ? ui_bmp_radio_btn_construct(&self->base, value) : NULL;
}

void ui_bmp_radio_btn_set_caption(struct UIWindow *wnd, struct TextView text)
{
    struct UIBmpRadioBtn *self = (struct UIBmpRadioBtn *)wnd;
    string_assign(&self->label, text.data, text.size);
    ui_window_invalidate(wnd);
}
