#include "ui/controls/edit.h"
#include "ui/chat/edit_history.h"
#include "ui/items/item_tags.h"
#include "platform/language.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "core/markup_tags.h"
#include <string.h>
#include "ui/chat/chat.h"
#include "ui/ui_items.h"
#include "diag/perf.h"
#include "ui/chat/chat_window.h"

struct UIChatEditCtrl;
struct UIWindow;
void ui_chat_edit_ctrl_on_draw(struct UIWindow *self);
void ui_chat_edit_ctrl_on_l_btn_down(struct UIWindow *self, int x, int y);
const struct UIEditCtrlOps ui_chat_edit_ctrl_ops = {
    .base = {
        .destroy = ui_chat_edit_ctrl_destroy,
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
        .on_draw = ui_chat_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_chat_edit_ctrl_on_l_btn_down,
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
        .send_msg = ui_chat_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
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

const struct UIEditCtrlOps ui_whisper_edit_ctrl_ops = {
    .base = {
        .destroy = ui_whisper_edit_ctrl_destroy,
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
        .on_focus = ui_whisper_edit_ctrl_on_focus,
        .on_focus_lost = ui_whisper_edit_ctrl_on_focus_lost,
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
        .send_msg = ui_whisper_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_whisper_edit_ctrl_set_text,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};

void ui_chat_edit_ctrl_on_draw(struct UIWindow *window)
{
    struct UIChatEditCtrl *self = (struct UIChatEditCtrl *)window;
    ui_edit_ctrl_on_draw((struct UIWindow *)(&self->edit));
    if (!self->popup)
        return;
    ui_item_tag_ctrl_set_text(self->popup, string_view(&self->edit.text), self->edit.base.width);
    ui_item_tag_ctrl_update(self->popup);
    if (self->popup->ordered_count == 0) {
        if (self->popup)
            ui_window_destroy(&self->popup->edit.base);
        self->popup = NULL;
    }
}

void ui_chat_edit_ctrl_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIChatEditCtrl *self = (struct UIChatEditCtrl *)window;
    struct UIWindow *wnd = &self->edit.base;

    if (!self->edit.disabled)
        ui_window_mgr_set_focus_edit(&g_windowMgr, wnd);
    ui_window_on_l_btn_down(wnd->parent, x + wnd->x, y + wnd->y);
}

int ui_chat_edit_ctrl_has_item_link(struct UIChatEditCtrl *self, const struct String *text)
{
    const char *chars = string_cstr(text);

    (void)self;
    if (text_view_find(text_view(chars, text->size), text_view(string_cstr(&text_tag_ITEML), text_tag_ITEML.size), 0) != STRING_NPOS)
        return 1;
    if (text_view_find(text_view(chars, text->size), text_view(string_cstr(&text_tag_ITEML_END), text_tag_ITEML_END.size), 0) != STRING_NPOS)
        return 1;
    return 0;
}

int ui_chat_edit_ctrl_has_navi_link(struct UIChatEditCtrl *self, const struct String *text)
{
    const char *chars = string_cstr(text);

    (void)self;
    if (text_view_find(text_view(chars, text->size), text_view(string_cstr(&text_tag_NAVIL), text_tag_NAVIL.size), 0) != STRING_NPOS)
        return 1;
    if (text_view_find(text_view(chars, text->size), text_view(string_cstr(&text_tag_NAVIL_END), text_tag_NAVIL_END.size), 0) != STRING_NPOS)
        return 1;
    return 0;
}

#define ITEM_TAG_LINK_COLOUR 0x12b8a4

void ui_chat_edit_ctrl_add_item_link(struct UIWindow *wnd, const struct ItemInfo *info)
{
    struct UIChatEditCtrl *self = (struct UIChatEditCtrl *)wnd;
    struct String name = {0};
    struct String out = {0};
    BYTE fits;

    if (!self->popup) {
        void *mem = mem_try_alloc(sizeof(struct UIItemTagCtrl));

        self->popup = mem ? ui_item_tag_on_chat_construct(mem) : NULL;
        if (!self->popup)
            return;
    }
    item_info_get_display_name((struct ItemInfo *)info, &name);
    fits = ui_item_tag_ctrl_can_add(self->popup, string_view(&name), wnd->width);
    string_free(&name);
    if (!fits)
        return;
    string_assign(&self->popup->text, string_cstr(&self->edit.text), self->edit.text.size);
    item_info_get_display_name((struct ItemInfo *)info, &name);
    ui_item_tag_ctrl_append_tag(self->popup, &out, string_view(&name));
    language_cancel_input(g_language);
    language_replace_selection(g_language, string_cstr(&out));
    string_free(&out);
    string_free(&name);
    ui_item_tag_ctrl_add_item(self->popup, info, ITEM_TAG_LINK_COLOUR);
}
