#include "ui/controls/buttons.h"
#include "ui/character/selection_buttons.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "ui/items/items.h"
#include "diag/perf.h"

#define UI_MSG_HAIR_SELECT 0x0d
#define UI_MSG_HAIR_REFRESH 0x28
#define UI_BTN_DISABLED 3

#define HUMAN_HAIR_STYLES 0x17

#define MAKER_SKIN UI_SKIN "make_character_ver2\\"

struct UIHairBitmap {
    struct UIBitmapButton button;
};
struct UIWindow;
void ui_hair_bitmap_destroy(struct UIWindow *window);
void ui_hair_bitmap_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_hair_bitmap_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_hair_bitmap_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_hair_bitmap_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_hair_bitmap_track_mouse(struct UIWindow *self, int x, int y);
void ui_hair_bitmap_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_hair_bitmap_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_hair_bitmap_ops = {
    .base = {
        .destroy = ui_hair_bitmap_destroy,
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
        .on_draw = ui_bitmap_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_hair_bitmap_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_hair_bitmap_on_l_btn_dbl_clk,
        .on_mouse_move = ui_hair_bitmap_on_mouse_move,
        .on_mouse_hover = ui_hair_bitmap_on_mouse_hover,
        .track_mouse = ui_hair_bitmap_track_mouse,
        .on_mouse_shape = ui_hair_bitmap_on_mouse_shape,
        .on_l_btn_up = ui_hair_bitmap_on_l_btn_up,
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
    .set_show = ui_button_set_show,
};

void ui_hair_bitmap_destroy(struct UIWindow *window)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    self->button.button.base.ops = &ui_hair_bitmap_ops.base;
    ui_bitmap_button_destruct(&self->button);
    mem_free(self);
}

#define HAIR_BITMAP_FORWARD(slot, self, x, y)                                              \
    do {                                                                                   \
        struct UIWindow *parent_ = (self)->button.button.base.parent;                      \
        if (parent_)                                                                       \
            parent_->ops->slot(parent_, x, y);                                                 \
    } while (0)

void ui_hair_bitmap_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    HAIR_BITMAP_FORWARD(on_l_btn_down, self, x, y);
}

void ui_hair_bitmap_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    HAIR_BITMAP_FORWARD(on_l_btn_dbl_clk, self, x, y);
}

void ui_hair_bitmap_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    HAIR_BITMAP_FORWARD(on_mouse_move, self, x, y);
}

void ui_hair_bitmap_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    HAIR_BITMAP_FORWARD(on_mouse_hover, self, x, y);
}

void ui_hair_bitmap_track_mouse(struct UIWindow *window, int x, int y)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    HAIR_BITMAP_FORWARD(track_mouse, self, x, y);
}

void ui_hair_bitmap_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    HAIR_BITMAP_FORWARD(on_mouse_shape, self, x, y);
}

void ui_hair_bitmap_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIHairBitmap *self = (struct UIHairBitmap *)window;
    HAIR_BITMAP_FORWARD(on_l_btn_up, self, x, y);
}

void ui_hair_style_button_destroy(struct UIWindow *window);
void ui_hair_style_button_on_create(struct UIWindow *self, int width, int height);
intptr_t ui_hair_style_button_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIButtonOps ui_hair_style_button_ops = {
    .base = {
        .destroy = ui_hair_style_button_destroy,
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
        .on_create = ui_hair_style_button_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_bitmap_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_hair_style_button_send_msg,
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
    .set_show = ui_button_set_show,
};

struct UIHairStyleButton *ui_hair_style_button_construct(struct UIHairStyleButton *self, int style, int male)
{
    ui_bitmap_button_construct(&self->button);
    self->style = style;
    self->button.button.base.ops = &ui_hair_style_button_ops.base;
    self->male = male;
    self->picture = NULL;
    self->selected = 0;
    return self;
}

void ui_hair_style_button_destroy(struct UIWindow *window)
{
    struct UIHairStyleButton *self = (struct UIHairStyleButton *)window;
    self->button.button.base.ops = &ui_hair_style_button_ops.base;
    ui_bitmap_button_destruct(&self->button);
    mem_free(self);
}

void ui_hair_style_button_on_create(struct UIWindow *window, int width, int height)
{
    struct UIHairStyleButton *self = (struct UIHairStyleButton *)window;
    struct UIHairBitmap *picture = mem_try_alloc(sizeof *picture);

    (void)width;
    (void)height;
    if (picture) {
        ui_bitmap_button_construct(&picture->button);
        picture->button.button.base.ops = &ui_hair_bitmap_ops.base;
    }
    self->picture = picture;
    ui_window_create(&picture->button.button.base, 0x24, 0x25);
    ui_window_add_child(&self->button.button.base, &self->picture->button.button.base);
    ui_hair_style_button_send_msg(&self->button.button.base, NULL, UI_MSG_HAIR_REFRESH, 0, 0, 0, 0);
}

static void hair_style_refresh(struct UIHairStyleButton *self)
{
    char path[0x80];

    memset(path, 0, sizeof path);
    if (self->style >= HUMAN_HAIR_STYLES) {
        if (self->picture) {
            ui_button_set_show((struct UIButton *)(self->picture), 0);
            ui_window_invalidate_children(&self->button.button.base);
        }
        return;
    }
    ui_button_set_show((struct UIButton *)(self->picture), 1);
    snprintf(path, sizeof path,
             self->male == 0 ? MAKER_SKIN "img_hairStyle_girl%02d.bmp" : MAKER_SKIN "img_hairStyle%02d.bmp",
             self->style + 1);
    if (self->picture) {
        ui_bitmap_button_set_bitmap(&self->picture->button, path, UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(&self->picture->button, path, UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(&self->picture->button, path, UI_BTN_DOWN, 0);
        ui_bitmap_button_set_bitmap(&self->picture->button, MAKER_SKIN "img_hairStyle_none.bmp", UI_BTN_DISABLED,
                                 0);
    }
    ui_window_invalidate_children(&self->button.button.base);
}

static void hair_style_select(struct UIHairStyleButton *self, int selected)
{
    static const char select_bmp[] = MAKER_SKIN "bt_hairStyle_select.bmp";
    BYTE was = self->selected;

    self->selected = selected != 0;
    if (was == self->selected)
        return;
    if (selected) {
        ui_bitmap_button_set_bitmap(&self->button, select_bmp, UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(&self->button, select_bmp, UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(&self->button, select_bmp, UI_BTN_DOWN, 0);
    } else {
        ui_bitmap_button_set_bitmap(&self->button, MAKER_SKIN "bt_hairStyle_normal.bmp", UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(&self->button, MAKER_SKIN "bt_hairStyle_over.bmp", UI_BTN_HOVER, 0);
    }
    ui_bitmap_button_set_bitmap(&self->button, select_bmp, UI_BTN_DOWN, 0);
    ui_window_invalidate_base(&self->button.button.base);
}

intptr_t ui_hair_style_button_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                         intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIHairStyleButton *self = (struct UIHairStyleButton *)window;
    switch (msg) {
    case UI_MSG_HAIR_SELECT:
        hair_style_select(self, val1);
        return 0;
    case UI_MSG_HAIR_REFRESH:
        hair_style_refresh(self);
        return 0;
    }
    ui_window_send_msg_base(&self->button.button.base, sender, msg, val1, val2, val3, val4);
    return 0;
}
