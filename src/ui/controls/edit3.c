#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "ui/chat/readable_links.h"
#include <stdint.h>
#include "iteminfo/item_link.h"
#include <stdlib.h>
#include "ui/items/item_tags.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "core/object_type_tables.h"
#include "iteminfo/item_table.h"
#include "core/memory.h"
#include "core/markup_tags.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include <string.h>
#include "diag/perf.h"

struct UIEditCtrl3;
struct UIWindow;
const struct UIEditCtrl3Ops ui_edit_ctrl3_ops = {
    .edit = {
        .base = {
            .destroy = ui_edit_ctrl3_destroy,
            .resize = ui_edit_ctrl3_resize,
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
            .on_create = ui_edit_ctrl3_on_create,
            .on_destroy = ui_window_on_destroy_base,
            .on_process = ui_window_on_process_base,
            .on_draw = ui_edit_ctrl3_on_draw,
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
            .on_wheel = ui_edit_ctrl3_on_wheel,
            .snap = ui_window_snap_base,
            .send_msg = ui_edit_ctrl3_send_msg,
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
        .set_text = ui_edit_ctrl3_set_text,
        .draw_text = ui_edit_ctrl3_draw_text,
        .truncate = ui_edit_ctrl_truncate_to_ime,
    },
    .update_bar = ui_edit_ctrl3_update_bar_default,
};

#define EDIT3_FONT_HEIGHT 0xc
#define TAG_COLOUR 0x82f2e1
#define TAG_BUTTON_ID 0x1c8

static int line_width(const struct UIEditCtrl3 *self)
{
    int width = self->edit.base.width - 6;
    if (self->scrolls) width += 3 - self->bar->base.width;
    return width;
}

static int text_width(struct UIEditCtrl3 *self, const char *text)
{
    return ui_window_text_width(&self->edit.base, text, 0, 0, EDIT3_FONT_HEIGHT, 0, 0);
}

static void strip_links(struct UIEditCtrl3 *self, struct String *out, struct TextView text)
{
    size_t at;
    ui_chat_his_box_readable_item_links(&self->edit.base, out, text);
    at = string_find(out, "<ITEML>", 0);
    if (at != SIZE_MAX)
        string_assign(out, string_cstr(out) + at, out->size - at);
}

struct EditItemLine {
    struct UIEditCtrl3 *edit;
    int index;
};

static void add_item_tag(void *context, const struct ItemInfo *item,
                         const char *name, const char *caption, const char *prefix)
{
    struct EditItemLine *line = context;
    struct UIEditCtrl3 *self = line->edit;
    struct UIItemTagButton *button = mem_try_alloc(sizeof *button);
    if (!button) return;
    button = ui_item_tag_button_construct(button);
    item_info_assign(&button->item, item);
    ui_text_button_set_text(&button->text.button.base, caption, TAG_COLOUR, 0, EDIT3_FONT_HEIGHT, 0, 0);
    string_assign_cstr(&button->text.button.tooltip, name);
    button->text.hover_colour = TAG_COLOUR;
    button->text.button.base.id = TAG_BUTTON_ID;
    ui_text_button_fit_width(&button->text, text_width(self, caption) + 1, button->text.button.base.height);
    ui_window_add_child(&self->edit.base, &button->text.button.base);
    item_tag_buttons_append(&self->tag_buttons, button);
    button->line = line->index;
    button->line_x = text_width(self, prefix) - self->edit.text_y;
}

static void set_linked_text(struct UIEditCtrl3 *self, const char *text)
{
    struct String shown = {0}, full = {0}, piece = {0};
    struct TextArray lines = {0};
    size_t i;

    string_assign_cstr(&full, text);
    strip_links(self, &shown, string_view(&full));
    ui_window_break_coloured_lines(&self->edit.base, string_cstr(&shown), &lines, &self->line_flags, line_width(self));
    ui_edit_ctrl3_rewrap(self, &lines);
    self->line_count = (int)lines.size;
    if (self->extra_line) ++self->line_count;
    ui_edit_ctrl3_update_bar(self);
    if (self->scrolls) {
        text_array_resize(&lines, 0);
        strip_links(self, &shown, string_view(&full));
        ui_window_break_coloured_lines(&self->edit.base, string_cstr(&shown), &lines, &self->line_flags, line_width(self));
        ui_edit_ctrl3_rewrap(self, &lines);
    }
    for (i = 0; i < lines.size; ++i) {
        struct EditItemLine context = {self, (int)i};
        item_link_build_line(&self->edit.base, &piece, &full, text_array_at(&lines, i).data, add_item_tag, &context);
        if (!owned_text_assign(&lines.items[i], string_view(&piece))) abort();
    }
    ui_edit_ctrl_set_text(&self->edit, string_cstr(&shown));
    string_free(&piece);
    string_free(&full);
    string_free(&shown);
    text_array_clear(&lines);
}

static void edit3_reset(struct UIEditCtrl3 *self)
{
    text_array_resize(&self->shown_lines, 0);
    byte_array_resize(&self->line_flags, 0);
    string_free(&self->line_above);
    string_free(&self->line_below);
}

struct UIWindow *ui_edit_ctrl3_construct(struct UIWindow *wnd)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)wnd;

    ui_edit_ctrl_construct(wnd);
    wnd->ops = &ui_edit_ctrl2_ops.base;
    wnd->ops = &ui_edit_ctrl3_ops.edit.base;
    memset(&self->tag_buttons, 0, sizeof self->tag_buttons);
    text_array_init(&self->lines);
    text_array_init(&self->shown_lines);
    byte_array_init(&self->line_flags);
    self->bar = NULL;
    self->scrolls = 0;
    self->line_height = 0xe;
    self->line_count = 0;
    self->caret_line_len = self->caret_col = self->caret_rest = self->caret_x = self->step = self->step_caret = 0;
    self->first_line = 0;
    self->scrolled = self->stepped_up = self->stepped_down = 0;
    self->caret_eol = self->extra_line = 0;
    string_init(&self->line_above);
    string_init(&self->line_below);
    return wnd;
}

void ui_edit_ctrl3_destruct(struct UIWindow *wnd)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)wnd;

    wnd->ops = &ui_edit_ctrl3_ops.edit.base;
    edit3_reset(self);
    byte_array_clear(&self->line_flags);
    text_array_clear(&self->shown_lines);
    text_array_clear(&self->lines);
    item_tag_buttons_clear(&self->tag_buttons);
    wnd->ops = &ui_edit_ctrl2_ops.base;
    ui_edit_ctrl_destruct(&self->edit);
}

void ui_edit_ctrl3_destroy(struct UIWindow *window)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    ui_edit_ctrl3_destruct(&self->edit.base);
    mem_free(self);
}

void ui_edit_ctrl3_resize(struct UIWindow *window, int width, int height)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    ui_window_resize_base(&self->edit.base, width, height);
    ui_window_move(&self->bar->base, self->edit.base.width - 10, 0);
    ui_window_resize(&self->bar->base, self->bar->base.width, height);
    ui_edit_ctrl3_update_bar(self);
}

void ui_edit_ctrl3_on_create(struct UIWindow *window, int width, int height)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    struct UIScrollBar *bar = mem_alloc(sizeof *bar);

    if (bar)
        bar = ui_scroll_bar3_construct(bar);
    self->bar = bar;
    ui_scroll_bar_create(bar, width - 10, 0, UI_SCROLL_VERT, height);
    ui_window_add_child(&self->edit.base, &self->bar->base);
}

void ui_edit_ctrl3_on_draw(struct UIWindow *wnd)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    ui_window_clear_canvas(wnd, RGB((BYTE)self->back_r, (BYTE)self->back_g, (BYTE)self->back_b));
    ui_edit_ctrl_draw_text(self);
    if (wnd->parent)
        ui_window_invalidate(wnd->parent);
}

void ui_edit_ctrl3_on_wheel(struct UIWindow *window, int delta)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    if (!self->scrolls)
        return;
    ui_window_send_msg(&self->edit.base, &self->bar->base, UI_MSG_SCROLL_V, delta < 0 ? 3 : -3, 0, 0, 0);
}

void ui_edit_ctrl3_set_text(struct UIEditCtrl *base_edit, const char *text)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)base_edit;
    struct UIEditCtrl *edit = &self->edit;

    string_assign_cstr(&edit->text, text);
    ui_edit_ctrl_truncate(edit);
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == &edit->base) {
        language_cancel_input(g_language);
        language_cancel_input(g_language);
        language_replace_selection(g_language, string_cstr(&edit->text));
    }
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == &edit->base) {
        language_set_selection(g_language, 0, (int)g_language->input_length);
    } else {
        edit->sel_start = 0;
        edit->sel_end = (int)edit->text.size;
    }
    if (text_view_find(string_view(&edit->text), string_view(&text_tag_ITEML), 0) != STRING_NPOS ||
        text_view_find(string_view(&edit->text), string_view(&text_tag_ITEML_END), 0) != STRING_NPOS)
        set_linked_text(self, string_cstr(&edit->text));
    if (edit->base.parent)
        ui_window_invalidate(edit->base.parent);
}

void ui_edit_ctrl3_update_bar_default(struct UIEditCtrl3 *self)
{
    int page = self->edit.base.height / self->line_height;
    int over = self->line_count - page;

    if (over == 1 && self->first_line == 0)
        over = 0;
    if (over < 0)
        over = 0;
    self->scrolls = over != 0;
    if (over) {
        ui_scroll_bar_set_range(self->bar, over, page, self->line_count);
        ui_scroll_bar_set_pos(self->bar, self->first_line);
    } else {
        ui_scroll_bar_set_range(self->bar, 0, page, self->line_count);
        self->first_line = 0;
    }
    ui_window_invalidate(&self->bar->base);
}
