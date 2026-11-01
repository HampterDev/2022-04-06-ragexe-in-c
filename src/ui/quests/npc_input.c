#include "ui/controls/edit.h"
#include "ui/chat/markup_links.h"
#include "ui/items/item_tags.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <stdlib.h>
#include "ui/quests/quests.h"
#include <string.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/quests/npc_input.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/items/item_tags_internal.h"

#define MSG_NPC_WHILE_MAIL_OPEN 0xaaa

static __inline void ui_window_draw_text(struct UIWindow *self, int x, int y, const char *text, int height,
                                       COLORREF colour)
{
    ui_window_text_out(self, x, y, text, 0, 0, height, colour, 0, 0);
}

#define NPC_NUMBER_WND_ID 0x38

#define NPC_INPUT_OK_ID 0xb0
#define NPC_TEXT_EDIT_ID 0x1e4
#define NPC_INPUT_MSG_NPC_ID 0x1c
#define MODE_MSG_NPC_NUMBER 0x63

#define NPC_NUMBER_MAX_CHARS 12
#define NPC_TEXT_MAX_CHARS 0x46

struct UINpcEditDialogWnd;
void ui_npc_edit_dialog_wnd_destroy(struct UIWindow *window);
void ui_npc_edit_dialog_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_npc_edit_dialog_wnd_on_draw(struct UIWindow *self);
int ui_npc_edit_dialog_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_npc_edit_dialog_wnd_ops = {
    .destroy = ui_npc_edit_dialog_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_npc_edit_dialog_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_npc_edit_dialog_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_npc_edit_dialog_wnd_send_msg,
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
struct UINpcTextEditDialogWnd;
void ui_npc_text_edit_dialog_wnd_destroy(struct UIWindow *window);
void ui_npc_text_edit_dialog_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_npc_text_edit_dialog_wnd_on_draw(struct UIWindow *self);
int ui_npc_text_edit_dialog_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_npc_text_edit_dialog_wnd_ops = {
    .destroy = ui_npc_text_edit_dialog_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_npc_text_edit_dialog_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_npc_text_edit_dialog_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_npc_text_edit_dialog_wnd_send_msg,
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

struct UINpcEditDialogWnd {
    struct UIFrameWnd frame;
    struct UIEditCtrl *edit;
    int npc_id;
};
SIZE_ASSERT(UINpcEditDialogWnd, 0x9c);

struct UINpcTextEditDialogWnd {
    struct UINpcEditDialogWnd base;
    struct UIItemTagCtrl *popup;
};
SIZE_ASSERT(UINpcTextEditDialogWnd, 0xa0);

#define NPC_INPUT_BTN_DIR UI_SKIN

static void npc_input_create(struct UINpcEditDialogWnd *self, int max_chars, const char *text, int edit_id)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIEditCtrl *edit = mem_alloc(sizeof(struct UIEditCtrl));
    struct UIBitmapButton *ok;
    int ok_x, ok_y;

    if (edit)
        edit = (struct UIEditCtrl *)ui_edit_ctrl_construct(&edit->base);
    self->edit = edit;
    ui_window_create(&edit->base, wnd->width - 0x4d, 0x10);
    ui_window_move_base(&self->edit->base, 0xf, 0x16);
    self->edit->max_chars = max_chars;
    ui_edit_ctrl_set_back_colour(&self->edit->base, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_text(self->edit, text);
    ui_window_add_child(wnd, &self->edit->base);
    if (edit_id)
        self->edit->base.id = edit_id;
    ui_window_mgr_set_focus_edit(&g_windowMgr, &self->edit->base);
    ui_edit_ctrl_select_all(&self->edit->base);

    ok_x = wnd->width - 0x37;
    ok_y = wnd->height - 0x20;
    ok = mem_alloc(sizeof(struct UIBitmapButton));
    if (ok)
        ok = ui_bitmap_button_construct(ok);
    ui_bitmap_button_set_bitmap(ok, NPC_INPUT_BTN_DIR "btn_ok.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(ok, NPC_INPUT_BTN_DIR "btn_ok_a.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(ok, NPC_INPUT_BTN_DIR "btn_ok_b.bmp", 2, 0);
    ui_window_create(&ok->button.base, ok->bitmap_width, ok->bitmap_height);
    ui_window_move_base(&ok->button.base, ok_x, ok_y);
    ok->button.base.id = NPC_INPUT_OK_ID;
    ui_window_add_child(wnd, &ok->button.base);

    self->npc_id = -1;
    UI_FRAME_DEFAULT_ID(&self->frame) = NPC_INPUT_OK_ID;
}

struct UINpcEditDialogWnd *ui_npc_edit_dialog_wnd_construct(struct UINpcEditDialogWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_npc_edit_dialog_wnd_ops;
    return self;
}

struct UIWindow *ui_npc_edit_dialog_wnd_new(void)
{
    return (struct UIWindow *)ui_npc_edit_dialog_wnd_construct(mem_alloc(sizeof(struct UINpcEditDialogWnd)));
}

void ui_npc_edit_dialog_wnd_destruct(struct UINpcEditDialogWnd *self)
{
    self->frame.base.ops = &ui_npc_edit_dialog_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_npc_edit_dialog_wnd_destroy(struct UIWindow *window)
{
    struct UINpcEditDialogWnd *self = (struct UINpcEditDialogWnd *)window;
    ui_npc_edit_dialog_wnd_destruct(self);
    mem_free(self);
}

void ui_npc_edit_dialog_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UINpcEditDialogWnd *self = (struct UINpcEditDialogWnd *)window;
    npc_input_create(self, NPC_NUMBER_MAX_CHARS, "0", 0);
}

void ui_npc_edit_dialog_wnd_on_draw(struct UIWindow *window)
{
    struct UINpcEditDialogWnd *self = (struct UINpcEditDialogWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
    ui_window_draw_text(&self->frame.base, 0xf, 6, g_serviceType == 0xc ? "Insira o Numero" : "Input number ",
                      12, 0);
}

int ui_npc_edit_dialog_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UINpcEditDialogWnd *self = (struct UINpcEditDialogWnd *)window;
    if (g_windowMgr.rodex_compose_wnd) {
        ui_chat_line(msg_str(MSG_NPC_WHILE_MAIL_OPEN), RGB(0xff, 0xff, 0x64));
        return 0;
    }
    if (msg == NPC_INPUT_MSG_NPC_ID) {
        self->npc_id = val1;
        return 0;
    }
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);

    if (val1 == NPC_INPUT_OK_ID && ui_edit_ctrl_get_text_size(&self->edit->base)) {
        int number = atoi(ui_edit_ctrl_get_text(&self->edit->base));

        mode_send_message(client_current_mode(), MODE_MSG_NPC_NUMBER, self->npc_id, number, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, NPC_NUMBER_WND_ID);
    }
    return 0;
}

struct UINpcTextEditDialogWnd *ui_npc_text_edit_dialog_wnd_construct(struct UINpcTextEditDialogWnd *self)
{
    ui_frame_wnd_construct(&self->base.frame, 0);
    self->base.frame.base.ops = &ui_npc_edit_dialog_wnd_ops;
    self->base.frame.base.ops = &ui_npc_text_edit_dialog_wnd_ops;
    self->popup = NULL;
    return self;
}

struct UIWindow *ui_npc_text_edit_dialog_wnd_new(void)
{
    return (struct UIWindow *)ui_npc_text_edit_dialog_wnd_construct(mem_alloc(sizeof(struct UINpcTextEditDialogWnd)));
}

static void ui_npc_text_edit_dialog_wnd_destruct(struct UINpcTextEditDialogWnd *self)
{
    self->base.frame.base.ops = &ui_npc_text_edit_dialog_wnd_ops;
    if (self->popup)
        ui_item_tag_on_mega_phone_destroy(&self->popup->edit.base);
    ui_npc_edit_dialog_wnd_destruct(&self->base);
}

void ui_npc_text_edit_dialog_wnd_destroy(struct UIWindow *window)
{
    struct UINpcTextEditDialogWnd *self = (struct UINpcTextEditDialogWnd *)window;
    ui_npc_text_edit_dialog_wnd_destruct(self);
    mem_free(self);
}

void ui_npc_text_edit_dialog_wnd_on_create(struct UIWindow *window, int width_unused,
                                                int height_unused)
{
    struct UINpcTextEditDialogWnd *self = (struct UINpcTextEditDialogWnd *)window;
    npc_input_create(&self->base, NPC_TEXT_MAX_CHARS, "", NPC_TEXT_EDIT_ID);
}

void ui_npc_text_edit_dialog_wnd_on_draw(struct UIWindow *window)
{
    struct UINpcTextEditDialogWnd *self = (struct UINpcTextEditDialogWnd *)window;
    struct String text;
    const char *typed;

    ui_window_draw_sys_box(&self->base.frame.base, 0, 0, 0, 0);
    if (!self->popup)
        return;
    typed = ui_edit_ctrl_get_text(&self->base.edit->base);
    string_init(&text);
    string_assign_cstr(&text, typed);
    ui_item_tag_ctrl_set_text(self->popup, string_view(&text), self->base.edit->base.width);
    string_free(&text);
    ui_item_tag_ctrl_update(self->popup);
    if (self->popup->ordered_count == 0) {
        ui_item_tag_on_mega_phone_destroy(&self->popup->edit.base);
        self->popup = NULL;
    }
}

#define MSG_NPC_WHILE_MAIL_OPEN 0xaaa
#define MSG_NPC_TEXT_LINK 0xbe2
#define NPC_INPUT_OK_ID 0xb0
#define NPC_INPUT_MSG_NPC_ID 0x1c
#define NPC_TEXT_WND_ID 0x64
#define MODE_MSG_NPC_TEXT 0xa4

static int link_size(struct UIWindow *edit)
{
    return (int)ui_edit_ctrl_link_text((struct UIEditCtrl *)edit).size;
}

static void send_text(struct UINpcTextEditDialogWnd *self, struct Mode *mode, const char *link)
{
    mode_send_message(mode, MODE_MSG_NPC_TEXT, self->base.npc_id, (int)ui_edit_ctrl_get_text(&self->base.edit->base), (int)link,
                  0);
    ui_window_mgr_post_quit1(&g_windowMgr, NPC_TEXT_WND_ID);
}

int ui_npc_text_edit_dialog_wnd_send_msg(struct UIWindow *window,
                                              struct UIWindow *sender, int msg, int val1, int val2, int val3,
                                              int val4)
{
    struct UINpcTextEditDialogWnd *self = (struct UINpcTextEditDialogWnd *)window;
    struct UIWindow *edit = &self->base.edit->base;
    struct String text;
    struct Mode *mode;
    const char *typed;
    int blocked;

    if (msg != UI_MSG_COMMAND) {
        if (g_windowMgr.rodex_compose_wnd) {
            ui_chat_line(msg_str(MSG_NPC_WHILE_MAIL_OPEN), RGB(0xff, 0xff, 0x64));
            return 0;
        }
        if (msg == NPC_INPUT_MSG_NPC_ID) {
            self->base.npc_id = val1;
            return 0;
        }
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->base.frame), sender, msg, val1, val2, val3, val4);
    }

    if (val1 != NPC_INPUT_OK_ID || !ui_edit_ctrl_get_text_size(edit))
        return 0;

    typed = ui_edit_ctrl_get_text(edit);
    string_init(&text);
    string_assign_cstr(&text, typed);
    if (text.size) {
        const char *s = string_cstr(&text);
        const char *nl = memchr(s, '\n', text.size);

        if (nl) {
            string_truncate(&text, (size_t)(nl - s));
            ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, string_cstr(&text));
        }
    }
    mode = client_current_mode();
    blocked = navi_links_has_link(text_view_cstr(ui_edit_ctrl_get_text(edit)));
    if (!blocked)
        blocked = link_size(edit) && self->popup && self->popup->linked_text.size;
    if (blocked) {
        ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, "");
        ui_chat_line(msg_str(MSG_NPC_TEXT_LINK), 0xff);
        goto done;
    }
    if (link_size(edit)) {
        struct TextView link = ui_edit_ctrl_link_text((struct UIEditCtrl *)edit);

        send_text(self, mode, link.data);
    } else if (self->popup) {
        const struct String *popup_text = &self->popup->linked_text;

        if (!string_equal_cstr(popup_text, "")) {
            if (chat_filter_is_blocked(string_view(popup_text))) {
                ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, "");
                goto done;
            }
            send_text(self, mode, string_cstr(&self->popup->linked_text));
        } else {
            send_text(self, mode, NULL);
        }
    } else {
        send_text(self, mode, NULL);
    }
done:
    string_free(&text);
    return 0;
}

struct UIEditCtrl *ui_npc_text_dialog_edit(struct UIWindow *dialog)
{
    return ((struct UINpcEditDialogWnd *)dialog)->edit;
}

void megaphone_wnd_add_item_link(struct UIWindow *wnd, const struct ItemInfo *info)
{
    struct UINpcTextEditDialogWnd *self = (struct UINpcTextEditDialogWnd *)wnd;
    struct UIItemTagCtrl *link = self->popup;
    struct String name = {0};
    struct String out = {0};
    BYTE fits;

    if (!link) {
        struct UIItemTagOnMegaPhone *mem = mem_alloc(sizeof *mem);

        link = mem ? &ui_item_tag_on_mega_phone_construct(mem)->ctrl : NULL;
        self->popup = link;
        if (!link)
            return;
    }
    item_info_get_display_name((struct ItemInfo *)info, &name);
    fits = ui_item_tag_ctrl_can_add(link, string_view(&name), self->base.edit->base.width);
    string_free(&name);
    if (!fits)
        return;
    string_assign_cstr(&link->text, ui_edit_ctrl_get_text(&self->base.edit->base));
    item_info_get_display_name((struct ItemInfo *)info, &name);
    ui_item_tag_ctrl_append_tag(link, &out, string_view(&name));
    ui_edit_ctrl_set_text(self->base.edit, string_cstr(&out));
    string_free(&out);
    string_free(&name);
    ui_edit_ctrl_make_select(&self->base.edit->base);
    ui_item_tag_ctrl_add_item(link, info, 0x12b8a4);
}
