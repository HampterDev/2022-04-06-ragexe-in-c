#include "ui/window_access.h"
#include "ui/quests/say_dialog.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "core/fatal.h"
#include "game/context.h"
#include <string.h>
#include "ui/quests/quests.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/quests/tag_text_viewer.h"
#include "platform/clock.h"

struct UISayDialogWnd;
struct UIWindow;
void ui_say_dialog_wnd_destroy(struct UIWindow *window);
void ui_say_dialog_wnd_resize(struct UIWindow *self, int width, int height);
void ui_say_dialog_wnd_store_info(struct UIWindow *self);
void ui_say_dialog_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_say_dialog_wnd_on_draw(struct UIWindow *self);
int ui_say_dialog_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_say_dialog_wnd_ops = {
    .destroy = ui_say_dialog_wnd_destroy,
    .resize = ui_say_dialog_wnd_resize,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_say_dialog_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_say_dialog_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_say_dialog_wnd_on_draw,
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
    .send_msg = ui_say_dialog_wnd_send_msg,
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

struct UISayDialogWnd *ui_say_dialog_wnd_construct(struct UISayDialogWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_say_dialog_wnd_ops;
    list_init(&self->buttons);
    list_init(&self->button_ids);
    self->text = NULL;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_say_dialog_wnd_new(void)
{
    return (struct UIWindow *)ui_say_dialog_wnd_construct(mem_alloc(sizeof(struct UISayDialogWnd)));
}

static void ui_say_dialog_wnd_destruct(struct UISayDialogWnd *self)
{
    self->frame.base.ops = &ui_say_dialog_wnd_ops;
    game_mark_npc_dialog_closed();
    list_free(&self->button_ids, NULL);
    list_free(&self->buttons, NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_say_dialog_wnd_destroy(struct UIWindow *window)
{
    struct UISayDialogWnd *self = (struct UISayDialogWnd *)window;
    ui_say_dialog_wnd_destruct(self);
    mem_free(self);
}

void ui_say_dialog_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UISayDialogWnd *self = (struct UISayDialogWnd *)window;
    ui_window_resize_base(&self->frame.base, width, height);
    ui_window_resize_base(&self->text->base, width - 0x14, height - 0x28);
}

void ui_say_dialog_wnd_store_info(struct UIWindow *window)
{
    struct UISayDialogWnd *self = (struct UISayDialogWnd *)window;
    if (!self->info)
        return;
    self->info->width = self->frame.base.width;
    self->info->height = self->frame.base.height;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
}

void ui_say_dialog_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISayDialogWnd *self = (struct UISayDialogWnd *)window;
    struct UITagTextViewer *text = ui_tag_text_viewer_create(width - 0x14, height - 0x28);

    self->text = text;
    self->text->bg_argb = 0xfff2f2f2;
    ui_window_invalidate_base(&self->text->base);
    self->text->wrap_width = width - 0x41;
    ui_window_move_base(&self->text->base, 10, 10);
    ui_window_add_child(&self->frame.base, &self->text->base);
    self->npc_id = 0;
    UI_FRAME_DEFAULT_ID(&self->frame) = UI_ID_NONE;
    self->clear_on_text = 0;
}

void ui_say_dialog_wnd_on_draw(struct UIWindow *window)
{
    struct UISayDialogWnd *self = (struct UISayDialogWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
}

#define SAY_MSG_SHOW_NEXT 0x19
#define SAY_MSG_TEXT 0x1a
#define SAY_MSG_NO_BUTTON 0x20
#define SAY_MSG_SHOW_CLOSE 0x3f
#define SAY_MSG_CLEAR 0x4b
#define SAY_MSG_SET_CMD 0x50
#define SAY_MSG_ADD_BUTTON 0x55
#define SAY_MSG_REMOVE_BUTTON 0x56

#define SAY_ID_NEXT 0xd2
#define SAY_CMD_NEXT 0x16
#define SAY_CMD_CLOSE 0x59
#define SAY_NEXT_GAP_MS 300
#define SAY_BUTTON_MARGIN 4
#define MSG_SAY_BUSY 0xaaa

static void say_button_bitmap(struct UIBitmapButton *button, const struct String *dir, const char *name,
                              const char *suffix, int state)
{
    struct String path;

    string_copy(&path, dir);
    string_append_cstr(&path, name);
    string_append_cstr(&path, suffix);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

static void say_add_button(struct UISayDialogWnd *self, int id, const char *name)
{
    struct UIWindow *wnd = &self->frame.base;
    struct String dir;
    struct UIBitmapButton *button;
    struct UIWindow *window;
    struct ListNode *node;
    int x;

    string_init(&dir);
    string_assign(&dir, UI_SKIN, sizeof UI_SKIN - 1);
    button = mem_alloc(sizeof *button);
    if (button)
        button = ui_bitmap_button_construct(button);
    say_button_bitmap(button, &dir, name, ".bmp", 0);
    say_button_bitmap(button, &dir, name, "_a.bmp", 1);
    say_button_bitmap(button, &dir, name, "_b.bmp", 2);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    x = wnd->width - SAY_BUTTON_MARGIN;
    for (node = self->buttons.head->next; node != self->buttons.head; node = node->next)
        x += -SAY_BUTTON_MARGIN - ((struct UIBitmapButton *)*LIST_VALUE(node, struct UIWindow *))->bitmap_width;
    x -= button->bitmap_width;
    ui_window_move_base(&button->button.base, x, wnd->height - 0x18);
    button->button.base.id = id;
    ui_window_add_child(wnd, &button->button.base);
    window = &button->button.base;
    list_push_back(&self->buttons, &window, sizeof window);
    string_free(&dir);
}

static void say_remove_button(struct UISayDialogWnd *self, int id)
{
    struct ListNode *node;

    for (node = self->buttons.head->next; node != self->buttons.head; node = node->next) {
        struct UIWindow *button = *LIST_VALUE(node, struct UIWindow *);

        if (button->id != id)
            continue;
        ui_window_move(button, self->frame.base.width, self->frame.base.height);
        ui_window_mgr_post_quit2(&g_windowMgr, button);
        list_erase(&self->buttons, node, NULL);
        return;
    }
}

static void say_show_button(struct UISayDialogWnd *self, int default_id, int cmd_id, const char *name)
{
    UI_FRAME_DEFAULT_ID(&self->frame) = default_id;
    self->cmd_id = cmd_id;
    ui_say_dialog_wnd_send_msg(&self->frame.base, NULL, SAY_MSG_ADD_BUTTON, SAY_ID_NEXT, (int)name, 0, 0);
    ui_window_invalidate_base(&self->frame.base);
}

int ui_say_dialog_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UISayDialogWnd *self = (struct UISayDialogWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (g_windowMgr.rodex_compose_wnd) {
        ui_chat_line(msg_str(MSG_SAY_BUSY), 0x64ffff);
        return 0;
    }
    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == SAY_ID_NEXT) {
            DWORD now = platform_coarse_ticks_ms();

            if (now - g_session1.npcDialogTick < SAY_NEXT_GAP_MS)
                return 0;
            g_session1.npcDialogTick = now;
            mode_send_message(client_current_mode(), self->cmd_id, self->npc_id, SAY_ID_NEXT, 0, 0);
            self->clear_on_text = 1;
            ui_say_dialog_wnd_send_msg(wnd, NULL, SAY_MSG_NO_BUTTON, 0, 0, 0, 0);
            return 0;
        }
        if (val1 == UI_ID_NONE)
            return 0;
        mode_send_message(client_current_mode(), self->cmd_id, self->npc_id, val1, 0, 0);
        return 0;

    case UI_MSG_BAR_PAGE_UP:
    case UI_MSG_BAR_PAGE_DOWN:
        ui_tag_text_viewer_send_msg(&self->text->base, NULL, msg, 0, 0, 0, 0);
        return 0;

    case SAY_MSG_SHOW_NEXT:
        say_show_button(self, SAY_ID_NEXT, SAY_CMD_NEXT, "btn_next");
        return 0;

    case SAY_MSG_SHOW_CLOSE:
        say_show_button(self, UI_ID_CLOSE, SAY_CMD_CLOSE, "btn_close");
        return 0;

    case SAY_MSG_TEXT:
        if (val1) {
            if (self->clear_on_text) {
                ui_tag_text_viewer_remove_all(self->text);
                self->clear_on_text = 0;
            }
            ui_tag_text_viewer_append_text(self->text, (const char *)val1);
        }
        self->npc_id = val2;
        return 0;

    case SAY_MSG_NO_BUTTON:
        UI_FRAME_DEFAULT_ID(&self->frame) = UI_ID_NONE;
        ui_say_dialog_wnd_send_msg(wnd, NULL, SAY_MSG_REMOVE_BUTTON, SAY_ID_NEXT, 0, 0, 0);
        ui_window_invalidate_base(wnd);
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!say_dialog_info_is_valid(self->info))
            say_dialog_info_set_default(self->info);
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        return 0;

    case SAY_MSG_CLEAR:
        ui_tag_text_viewer_remove_all(self->text);
        return 0;

    case SAY_MSG_SET_CMD:
        self->cmd_id = val1;
        return 0;

    case SAY_MSG_ADD_BUTTON:
        say_add_button(self, val1, (const char *)val2);
        return 0;

    case SAY_MSG_REMOVE_BUTTON:
        say_remove_button(self, val1);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_say_dialog_wnd_set_text_align(struct UISayDialogWnd *self, unsigned char align)
{
    if (self && self->text) ui_tag_text_viewer_set_align(self->text, align);
}
