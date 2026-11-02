#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include "ui/items/item_tags_internal.h"
#include "diag/perf.h"

static const struct ItemTagActions plain_tag_actions, chat_tag_actions, mail_tag_actions, megaphone_tag_actions;
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include "ui/mail/rodex_compose.h"

#define TAG_OPEN "<"
#define TAG_CLOSE ">"

struct UIWindow;
void ui_item_tag_button_destroy(struct UIWindow *window);
void ui_item_tag_button_on_draw(struct UIWindow *self);
void ui_item_tag_button_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_tag_button_on_mouse_hover(struct UIWindow *self, int x, int y);
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

void ui_item_tag_ctrl_destroy(struct UIWindow *window);
const struct UIEditCtrlOps ui_item_tag_ctrl_ops = {
    .base = {
        .destroy = ui_item_tag_ctrl_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};

void ui_item_tag_ctrl_destruct(struct UIItemTagCtrl *self)
{
    self->edit.base.ops = &ui_item_tag_ctrl_ops.base;
    self->actions = &plain_tag_actions;
    ui_item_tag_ctrl_clear_tags(self);
    string_free(&self->linked_text);
    string_free(&self->text);
    ui_edit_ctrl_destruct(&self->edit);
}

void ui_item_tag_ctrl_destroy(struct UIWindow *window)
{
    struct UIItemTagCtrl *self = (struct UIItemTagCtrl *)window;
    ui_item_tag_ctrl_destruct(self);
    mem_free(self);
}

BYTE ui_item_tag_ctrl_can_tag(struct UIItemTagCtrl *self, struct TextView text, int width)
{
    (void)self;
    (void)text;
    (void)width;
    return 1;
}

void ui_item_tag_ctrl_detach_tag(struct UIItemTagCtrl *self, struct UIWindow *button)
{
    struct UIWindow *edit = ui_window_mgr_get_focus_edit(&g_windowMgr);

    (void)self;
    if (edit)
        ui_window_remove_child(edit, button, 1);
}

void ui_item_tag_ctrl_place_tag(struct UIItemTagCtrl *self, struct UIWindow *button,
                                       struct TextView before, int x)
{
    (void)self;
    (void)button;
    (void)before;
    (void)x;
}

const struct UIEditCtrlOps ui_item_tag_on_mail_ops = {
    .base = {
        .destroy = ui_item_tag_on_mail_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};

#define MAIL_MAX_TAGS 3
#define MAIL_FONT_HEIGHT 0xc

void ui_item_tag_on_mail_destroy(struct UIWindow *window)
{
    struct UIItemTagOnMail *self = (struct UIItemTagOnMail *)window;
    self->ctrl.edit.base.ops = &ui_item_tag_on_mail_ops.base;
    self->ctrl.actions = &mail_tag_actions;
    ui_item_tag_ctrl_clear_tags(&self->ctrl);
    text_array_clear(&self->lines);
    ui_item_tag_ctrl_destruct(&self->ctrl);
    mem_free(self);
}

BYTE ui_item_tag_on_mail_can_tag(struct UIItemTagCtrl *ctrl, struct TextView text, int width)
{
    struct UIItemTagOnMail *self = (struct UIItemTagOnMail *)ctrl;
    struct UIEditCtrl3 *body;
    struct String tag = {0}, cut = {0};
    unsigned max_chars;
    BYTE fits;

    if (self->ctrl.tags.size > MAIL_MAX_TAGS - 1)
        return 0;
    string_init(&tag);
    string_assign(&tag, TAG_OPEN, 1);
    self->ctrl.width = width;
    ui_window_fit_plain_text(&self->ctrl.edit.base, &cut, text, width / 2);
    string_append(&tag, string_cstr(&cut), cut.size);
    string_free(&cut);
    string_append(&tag, TAG_CLOSE, 1);
    body = ui_rodex_compose_body(g_windowMgr.rodex_compose_wnd);
    max_chars = (unsigned)body->edit.max_chars;
    string_assign(&self->ctrl.text, string_cstr(&body->edit.text), body->edit.text.size);
    string_append(&self->ctrl.text, string_cstr(&tag), tag.size);
    fits = self->ctrl.text.size <= max_chars;
    string_free(&tag);
    return fits;
}

void ui_item_tag_on_mail_attach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button)
{
    (void)ctrl;
    if (g_windowMgr.rodex_compose_wnd)
        ui_window_add_child(g_windowMgr.rodex_compose_wnd, button);
}

void ui_item_tag_on_mail_detach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button)
{
    (void)ctrl;
    if (g_windowMgr.rodex_compose_wnd)
        ui_window_remove_child(g_windowMgr.rodex_compose_wnd, button, 1);
}

void ui_item_tag_on_mail_set_lines(struct UIItemTagOnMail *self, const struct TextArray *lines)
{
    size_t i;
    if (&self->lines == lines) return;
    if (!text_array_resize(&self->lines, lines->size)) abort();
    for (i = 0; i < lines->size; ++i)
        if (!owned_text_assign(&self->lines.items[i], text_array_at(lines, i))) abort();
}

static int mail_text_width(const char *text)
{
    return ui_window_text_width(g_windowMgr.rodex_compose_wnd, text, 0, 0, MAIL_FONT_HEIGHT, 0, 0);
}

void ui_item_tag_on_mail_place_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button,
                             struct TextView before, int x)
{
    struct UIItemTagOnMail *self = (struct UIItemTagOnMail *)ctrl;
    struct UIEditCtrl3 *body;
    size_t remaining = before.size, i;
    int full_lines = 0, full_width = 0, width, first_line, rows;
    (void)x;
    if (!g_windowMgr.rodex_compose_wnd) return;
    body = ui_rodex_compose_body(g_windowMgr.rodex_compose_wnd);
    first_line = body->first_line;
    rows = body->edit.base.height / body->line_height;
    for (i = 0; i < self->lines.size; ++i) {
        struct TextView line = text_array_at(&self->lines, i);
        if (remaining < line.size) break;
        remaining -= line.size;
        full_width += mail_text_width(line.data);
        ++full_lines;
    }
    {
        struct String prefix = {0};
        string_assign(&prefix, before.data, before.size);
        width = mail_text_width(string_cstr(&prefix));
        string_free(&prefix);
    }
    if (full_lines - first_line >= 0 && full_lines < first_line + rows)
        ui_window_move(button, body->edit.base.x + 2 + width - full_width,
                  (full_lines - first_line) * body->line_height + body->edit.base.y);
    else
        ui_window_move(button, 1000, 1000);
}

const struct UIEditCtrlOps ui_item_tag_on_chat_ops = {
    .base = {
        .destroy = ui_item_tag_on_chat_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};
struct UIItemTagOnMegaPhone;
const struct UIEditCtrlOps ui_item_tag_on_mega_phone_ops = {
    .base = {
        .destroy = ui_item_tag_on_mega_phone_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};
static const struct ItemTagActions plain_tag_actions = {
    ui_item_tag_ctrl_can_tag, ui_item_tag_on_chat_attach_tag, ui_item_tag_ctrl_detach_tag, ui_item_tag_ctrl_place_tag
};
static const struct ItemTagActions chat_tag_actions = {
    ui_item_tag_on_chat_can_tag, ui_item_tag_on_chat_attach_tag, ui_item_tag_on_chat_detach_tag, ui_item_tag_on_chat_place_tag
};
static const struct ItemTagActions mail_tag_actions = {
    ui_item_tag_on_mail_can_tag, ui_item_tag_on_mail_attach_tag, ui_item_tag_on_mail_detach_tag, ui_item_tag_on_mail_place_tag
};
static const struct ItemTagActions megaphone_tag_actions = {
    ui_item_tag_on_mega_phone_can_tag, ui_item_tag_on_mega_phone_attach_tag, ui_item_tag_on_mega_phone_detach_tag, ui_item_tag_on_mega_phone_place_tag
};

BYTE ui_item_tag_ctrl_can_add(struct UIItemTagCtrl *self, struct TextView name, int width)
{
    return self->actions->can_add(self, name, width);
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

struct UIItemTagCtrl *ui_item_tag_ctrl_construct(struct UIItemTagCtrl *self)
{
    ui_edit_ctrl_construct(&self->edit.base);
    self->edit.base.ops = &ui_item_tag_ctrl_ops.base;
    self->actions = &plain_tag_actions;
    self->ordered_count = 0;
    self->tags.head = NULL;
    self->tags.size = 0;
    string_init(&self->text);
    string_init(&self->linked_text);
    self->width = 0;
    return self;
}

struct UIItemTagCtrl *ui_item_tag_on_chat_construct(struct UIItemTagCtrl *self)
{
    ui_item_tag_ctrl_construct(self);
    self->edit.base.ops = &ui_item_tag_on_chat_ops.base;
    self->actions = &chat_tag_actions;
    return self;
}

struct UIItemTagOnMegaPhone *ui_item_tag_on_mega_phone_construct(struct UIItemTagOnMegaPhone *self)
{
    ui_item_tag_ctrl_construct(&self->ctrl);
    self->ctrl.edit.base.ops = &ui_item_tag_on_mega_phone_ops.base;
    self->ctrl.actions = &megaphone_tag_actions;
    return self;
}

struct UIItemTagOnMail *ui_item_tag_on_mail_construct(struct UIItemTagOnMail *self)
{
    ui_item_tag_ctrl_construct(&self->ctrl);
    self->ctrl.edit.base.ops = &ui_item_tag_on_mail_ops.base;
    self->ctrl.actions = &mail_tag_actions;
    text_array_init(&self->lines);
    return self;
}
