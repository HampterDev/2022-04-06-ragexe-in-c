#include "ui/controls/item_tag_button.h"
#include "ui/controls/controls.h"
#include "core/memory.h"
#include "core/object_type_tables.h"

#define TAG_OPEN "<"
#define TAG_CLOSE ">"

const struct UIButtonOps ui_item_tag_button_ops = {
    .base = {
        .destroy = ui_item_tag_button_destroy,
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
        .on_draw = ui_item_tag_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_item_tag_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_text_button_on_mouse_move,
        .on_mouse_hover = ui_item_tag_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_text_button_on_l_btn_up,
        .on_r_btn_down = ui_text_button_on_r_btn_down,
        .on_r_btn_up = ui_text_button_on_r_btn_up,
        .on_wheel = ui_text_button_on_wheel,
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

#define UI_MSG_ITEM_TAG_CLICK 0x62

void ui_item_tag_button_destroy(struct UIWindow *window)
{
    struct UIItemTagButton *self = (struct UIItemTagButton *)window;
    self->text.button.base.ops = &ui_item_tag_button_ops.base;
    item_info_destruct(&self->item);
    ui_text_button_destruct(&self->text);
    mem_free(self);
}

void ui_item_tag_button_on_draw(struct UIWindow *window)
{
    struct UIItemTagButton *self = (struct UIItemTagButton *)window;
    struct UITextButton *btn = &self->text;
    struct UIWindow *wnd = &btn->button.base;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (wnd->parent)
        ui_window_invalidate(wnd->parent);
    if (btn->outlined)
        ui_window_text_out_outlined(wnd, 1, self->text_y, string_cstr(&btn->label), 0, btn->text_colour,
                                 btn->outline_colour, btn->font_face, btn->font_height, 1, 0);
    else
        ui_window_text_out(wnd, 1, self->text_y, string_cstr(&btn->label), 0, btn->font_face,
                         btn->font_height, btn->text_colour, 0, 0);
    if (btn->underlined)
        ui_window_fill_rect(wnd, 0, wnd->height - 1, wnd->width, 1, btn->text_colour);
}

void ui_item_tag_button_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemTagButton *self = (struct UIItemTagButton *)window;
    struct UIWindow *wnd = &self->text.button.base;

    if (self->text.button.disabled)
        return;
    if (x < 0 || x >= wnd->width || y < 0 || y >= wnd->height) {
        wnd->state = UI_BTN_UP;
    } else {
        wnd->state = UI_BTN_HOVER;
        if (wnd->parent) {
            ui_window_send_msg(wnd->parent, wnd, UI_MSG_ITEM_TAG_CLICK, wnd->id, self->text.value, (intptr_t)&self->item,
                     0);
            ui_window_invalidate_base(wnd);
            return;
        }
    }
    ui_window_invalidate_base(wnd);
}

void ui_item_tag_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemTagButton *self = (struct UIItemTagButton *)window;
    struct UIButton *btn = &self->text.button;
    struct String tag;

    string_init(&tag);
    string_assign(&tag, TAG_OPEN, 1);
    string_append(&tag, string_cstr(&btn->tooltip), btn->tooltip.size);
    string_append(&tag, TAG_CLOSE, 1);
    if (!string_equal(&btn->text, &tag))
        ui_button_on_mouse_hover((struct UIWindow *)(btn), x, y);
    string_free(&tag);
}

struct UIItemTagButton *ui_item_tag_button_construct(struct UIItemTagButton *self)
{
    ui_text_button_construct(&self->text);
    self->text.button.base.ops = &ui_item_tag_button_ops.base;
    item_info_construct(&self->item);
    self->aid = 0;
    self->text_y = 1;
    return self;
}
