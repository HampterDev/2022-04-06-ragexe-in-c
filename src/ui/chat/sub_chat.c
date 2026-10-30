#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/markup_tags.h"
#include <string.h>
#include <mbstring.h>
#include "ui/chat/chat.h"
#include "ui/chat/chat_history.h"
#include "ui/chat/sub_chat.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "render/screen_draw.h"
#include "ui/ui_std.h"

struct UISubChatHisWnd;
struct UIWindow;
void ui_sub_chat_his_wnd_destroy(struct UIWindow *window);
void ui_sub_chat_his_wnd_resize(struct UIWindow *self, int width, int height);
void ui_sub_chat_his_wnd_move(struct UIWindow *self, int x, int y);
void ui_sub_chat_his_wnd_store_info(struct UIWindow *self);
void ui_sub_chat_his_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_sub_chat_his_wnd_on_draw(struct UIWindow *self);
void ui_sub_chat_his_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_sub_chat_his_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_sub_chat_his_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_sub_chat_his_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_sub_chat_his_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_sub_chat_his_wnd_draw_backdrop(struct UIWindow *window);
const struct UIWindowOps ui_sub_chat_his_wnd_ops = {
    .destroy = ui_sub_chat_his_wnd_destroy,
    .resize = ui_sub_chat_his_wnd_resize,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_sub_chat_his_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_sub_chat_his_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_sub_chat_his_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_sub_chat_his_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_sub_chat_his_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_sub_chat_his_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_sub_chat_his_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_sub_chat_his_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_sub_chat_his_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 1,
    .draw_backdrop = ui_sub_chat_his_wnd_draw_backdrop,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct UISubChatMiniWnd;
struct UIWindow;
void ui_sub_chat_mini_wnd_destroy(struct UIWindow *window);
void ui_sub_chat_mini_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_sub_chat_mini_wnd_on_draw(struct UIWindow *self);
void ui_sub_chat_mini_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_sub_chat_mini_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_sub_chat_mini_wnd_ops = {
    .destroy = ui_sub_chat_mini_wnd_destroy,
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
    .on_create = ui_sub_chat_mini_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_sub_chat_mini_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_sub_chat_mini_wnd_on_l_btn_down,
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
    .send_msg = ui_sub_chat_mini_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define BASIC_INTERFACE UI_SKIN "basic_interface\\"

#define SUBCHAT_ID_BATTLE_OPTION 0xb9
#define SUBCHAT_ID_CLOSE 0xc2
#define SUBCHAT_ID_STICK 0x15d
#define SUBCHAT_RESIZER_ID 7

#define SUBCHAT_MSG_ENTER UI_MSG_DEFAULT_BUTTON
#define SUBCHAT_MSG_PAGE_UP 3
#define SUBCHAT_MSG_PAGE_DOWN 4
#define SUBCHAT_MSG_ADD_LINE 0x25

#define SUBCHAT_MIN_WIDTH 0x118
#define SUBCHAT_MIN_HEIGHT 0x4a
#define SUBCHAT_MAX_WIDTH 0x200
#define SUBCHAT_MAX_HEIGHT 0x180
#define SUBCHAT_SIZE_STEP 32
#define SUBCHAT_TAB_MIN_WIDTH 0x19
#define SUBCHAT_TITLE_ROOM 0x32
#define MSG_SUBCHAT_TITLE_TOO_LONG 0x629
#define OFF_SCREEN (-100)
#define SUBCHAT_GREY 0x8e8e8e

static int title_width(struct UISubChatHisWnd *self, const char *text)
{
    return ui_window_text_width(&self->frame.base, text, 0, 0, 0xc, 0, 0) + 10;
}

static struct ChatTabSettings *settings(struct UISubChatHisWnd *self)
{
    return chat_tabs_get_or_add(&g_session1.subChatWnds, self->key);
}

static void tell_battle_options(struct UISubChatHisWnd *self, int what)
{
    struct UIWindow *options = ui_get_window(WND_ID_BATTLE_MSG_OPTION);

    if (options)
        ui_window_send_msg(options, &self->frame.base, UI_MSG_COMMAND, BATTLE_OPTION_FOR_TAB, self->key, 0, what);
}

struct UISubChatHisWnd *ui_sub_chat_his_wnd_construct(struct UISubChatHisWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_sub_chat_his_wnd_ops;
    string_init(&self->name);
    self->info = NULL;
    self->resizer = NULL;
    self->box = NULL;
    self->tab_height = 0x10;
    self->tab_width = 0x47;
    self->bottom = 0xf;
    switch (g_serviceType) {
    case 1:
    case 7:
    case 11:
        self->tab_width = 0x6e;
        break;
    case 10:
        self->tab_width = 0x47;
        break;
    case 18:
        self->tab_width = 0x5a;
        break;
    }
    return self;
}

struct UIWindow *ui_sub_chat_his_wnd_new(void)
{
    return (struct UIWindow *)ui_sub_chat_his_wnd_construct(mem_alloc(sizeof(struct UISubChatHisWnd)));
}

void ui_sub_chat_his_wnd_destroy(struct UIWindow *window)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    self->frame.base.ops = &ui_sub_chat_his_wnd_ops;
    string_free(&self->name);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_sub_chat_his_wnd_place_buttons(struct UISubChatHisWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *close = &self->close->button.base;
    struct UIWindow *stick = &self->stick->button.base;
    struct UIWindow *battle = &self->battle_option->button.base;
    int x;

    if (!CHAT_TABS_EDITABLE) {
        ui_window_move_base(close, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(stick, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(battle, wnd->width - battle->width - 2, 4);
        return;
    }
    x = wnd->width - close->width - 2;
    ui_window_move_base(close, x, 4);
    x += -2 - stick->width;
    ui_window_move_base(stick, x, 4);
    x -= battle->width + 2;
    ui_window_move_base(battle, x, 4);
}

static struct UIBitmapButton *add_button(const char *name)
{
    static const char *const suffix[3] = { "_a.bmp", "_b.bmp", "_b.bmp" };
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));
    struct String path;
    int state;

    if (button)
        button = ui_bitmap_button_construct(button);
    string_init(&path);
    string_assign_cstr(&path, name);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++) {
        struct String file;

        string_copy(&file, &path);
        string_append_cstr(&file, suffix[state]);
        ui_bitmap_button_set_bitmap(button, string_cstr(&file), state, 0);
        string_free(&file);
    }
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    string_free(&path);
    return button;
}

void ui_sub_chat_his_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIChatHisBox *box;
    struct UIResizer *resizer;
    struct ChatTab *node;
    int x = 0, y = 0;

    (void)width;
    (void)height;
    self->stick = add_button(BASIC_INTERFACE "stickon");
    ui_window_move_base(&self->stick->button.base, self->tab_width + 1, 4);
    self->stick->button.base.id = SUBCHAT_ID_STICK;
    ui_window_add_child(wnd, &self->stick->button.base);

    self->battle_option = add_button(BASIC_INTERFACE "battle_option" "2");
    ui_window_move_base(&self->battle_option->button.base, self->tab_width + 0xf, 5);
    self->battle_option->button.base.id = SUBCHAT_ID_BATTLE_OPTION;
    ui_window_add_child(wnd, &self->battle_option->button.base);

    self->close = add_button(BASIC_INTERFACE "wnd_mini");
    self->close->button.base.id = SUBCHAT_ID_CLOSE;
    ui_window_add_child(wnd, &self->close->button.base);

    box = mem_alloc(sizeof(struct UIChatHisBox));
    if (box)
        box = ui_chat_his_box_construct(box);
    self->box = box;
    ui_list_box_create_window(&box->list, 0, 0, wnd->width - 8, wnd->height - self->bottom - self->tab_height - 4, 0);
    ui_window_move_base(&self->box->list.base, 5, self->tab_height + 4);
    ui_window_add_child(wnd, (struct UIWindow *)self->box);

    resizer = mem_alloc(sizeof(struct UIResizer));
    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->resizer = resizer;
    ui_window_create(&resizer->base, 0xe, 0xe);
    ui_resizer_set_edge(self->resizer, SUBCHAT_RESIZER_ID);
    ui_resizer_set_bitmap(self->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(self->resizer, CHAT_COLOUR_KEY, CHAT_COLOUR_KEY);
    ui_window_add_child(wnd, &self->resizer->base);

    ui_window_to_screen(wnd, &x, &y);
    self->backdrop.x = x;
    self->backdrop.argb = CHAT_BACK_ARGB;
    self->backdrop.y = y + self->tab_height;
    self->backdrop.width = wnd->width;
    self->backdrop.edge_argb = 0;
    self->backdrop.height = wnd->height - self->tab_height;
    self->backdrop.draw_edge = 0;

    node = chat_tabs_find(&g_session1.subChatWnds, self->key);
    if (node) {
        string_assign(&self->name, owned_text_cstr(&node->value.name), node->value.name.size);
        self->tab_width = title_width(self, string_cstr(&self->name));
        memcpy(self->shown, node->value.shown, sizeof self->shown);
        if (self->tab_width < SUBCHAT_TAB_MIN_WIDTH)
            self->tab_width = SUBCHAT_TAB_MIN_WIDTH;
    }

    self->rename = mem_alloc(sizeof(struct UIEditCtrl));
    if (self->rename)
        self->rename = ui_edit_ctrl2_construct(self->rename);
    ui_window_create(self->rename, 0x64, 0x13);
    ui_window_move(self->rename, OFF_SCREEN, OFF_SCREEN);
    ui_window_add_child(wnd, self->rename);
    ui_sub_chat_his_wnd_place_buttons(self);
}

void ui_sub_chat_his_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_resize_base(wnd, width, height);
    ui_chat_his_box_resize_to(self->box, wnd->width - 8, wnd->height - self->bottom - self->tab_height - 4, 1);
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_window_move_base(&self->close->button.base, self->tab_width + 0x1a, 5);
    self->backdrop.width = wnd->width;
    self->backdrop.height = wnd->height - self->tab_height;
    {
        struct ChatTabSettings *tab = settings(self);
        if (tab) {
            tab->width = wnd->width;
            tab->height = wnd->height;
        }
    }
}

void ui_sub_chat_his_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    int sx = 0, sy = 0;

    ui_frame_wnd_move((struct UIWindow *)(&self->frame), x, y);
    ui_window_to_screen(&self->frame.base, &sx, &sy);
    self->backdrop.x = sx;
    self->backdrop.y = sy + self->tab_height;
    {
        struct ChatTabSettings *tab = settings(self);
        if (tab) {
            tab->x = sx;
            tab->y = sy;
        }
    }
}

void ui_sub_chat_his_wnd_store_info(struct UIWindow *window)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (self->info) {
        self->info->width = wnd->width;
        self->info->height = wnd->height;
        self->info->x = wnd->x;
        self->info->y = wnd->y;
    }
}

void ui_sub_chat_his_wnd_on_draw(struct UIWindow *window)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    COLORREF colour = 0xffffff;
    int tw = self->tab_width, th = self->tab_height;

    ui_window_clear_canvas(wnd, CHAT_COLOUR_KEY);
    ui_window_text_out(wnd, 5, 4, string_cstr(&self->name), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out_runs(wnd, 4, 3, string_cstr(&self->name), 0, &colour, 0, 0xc, 0);
    ui_window_fill_rect(wnd, 2, 0, tw - 5, 1, colour);
    ui_window_fill_rect(wnd, 1, 1, 1, 1, colour);
    ui_window_fill_rect(wnd, 0, 2, 1, wnd->height - 4, colour);
    ui_window_fill_rect(wnd, tw, th, wnd->width - tw - 3, 1, colour);
    ui_window_fill_rect(wnd, tw - 3, 0, 1, 1, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, tw - 2, 1, 1, 1, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, tw - 1, 2, 1, th - 1, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, wnd->width - 3, th, 1, 1, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, wnd->width - 2, th + 1, 1, 1, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, wnd->width - 1, th + 2, 1, wnd->height - th - 4, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, 1, wnd->height - 2, 1, 1, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, wnd->width - 2, wnd->height - 2, 1, 1, SUBCHAT_GREY);
    ui_window_fill_rect(wnd, 2, wnd->height - 1, wnd->width - 4, 1, SUBCHAT_GREY);
}

void ui_sub_chat_his_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    if (!CHAT_TABS_EDITABLE)
        return;
    tell_battle_options(self, 2);
    ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

void ui_sub_chat_his_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    if (!CHAT_TABS_EDITABLE || x <= 0 || x >= self->tab_width || y <= 0 || y >= self->tab_height)
        return;
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->rename, "");
    ui_window_resize(self->rename, self->tab_width - 10, self->tab_height - 2);
    ui_window_move(self->rename, 5, 2);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->rename);
}

void ui_sub_chat_his_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    if (ui_window_mgr_get_capture(&g_windowMgr) == &self->frame.base)
        ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
}

void ui_sub_chat_his_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    struct UIWindow *chat;
    int sx = 0, sy = 0, bottom;

    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
    if (!CHAT_TABS_EDITABLE)
        return;
    ui_window_to_screen(&self->frame.base, &sx, &sy);
    chat = ui_get_window(WND_ID_CHAT);
    if (!chat)
        return;
    bottom = sy + self->frame.base.height;
    if (chat->x <= sx && sx <= chat->x + chat->width && chat->y <= bottom && bottom <= chat->y + chat->height)
        ui_window_send_msg(chat, &self->frame.base, CHAT_MSG_STICK_TAB, 0, 0, 0, 0);
}

void ui_sub_chat_his_wnd_draw_backdrop(struct UIWindow *window)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;

    int x = 0, y = 0;

    ui_window_to_screen(&self->frame.base, &x, &y);
    draw_box_screen(x + 1, y + 1, self->tab_width - 2, self->tab_height, CHAT_BACK_ARGB);
    draw_box_screen(self->backdrop.x + 1, self->backdrop.y + 1, self->backdrop.width - 2, self->backdrop.height - 2,
                  self->backdrop.argb);
}

static void rename_tab(struct UISubChatHisWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct String text;
    int old_width = self->tab_width;
    RECT rect;

    string_init(&text);
    string_assign_cstr(&text, ui_edit_ctrl_get_text(self->rename));
    if (text.size && title_width(self, string_cstr(&text)) < wnd->width - SUBCHAT_TITLE_ROOM) {
        struct ChatTabSettings *tab = settings(self);

        if (!tab || !owned_text_assign(&tab->name, text_view(string_cstr(&text), text.size))) {
            string_free(&text);
            return;
        }
        string_assign(&self->name, string_cstr(&text), text.size);
        self->tab_width = title_width(self, string_cstr(&text));
        if (self->tab_width < SUBCHAT_TAB_MIN_WIDTH)
            self->tab_width = SUBCHAT_TAB_MIN_WIDTH;
    }
    if (title_width(self, string_cstr(&text)) >= wnd->width - SUBCHAT_TITLE_ROOM)
        ui_message_box(msg_str(MSG_SUBCHAT_TITLE_TOO_LONG));
    ui_window_move(self->rename, OFF_SCREEN, OFF_SCREEN);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->rename, "");
    ui_window_invalidate_base(wnd);
    ui_sub_chat_his_wnd_place_buttons(self);
    ui_window_mgr_set_focus_edit(&g_windowMgr, g_windowMgr.new_chat_wnd);
    rect.left = 0;
    rect.top = 0;
    rect.right = (old_width >= self->tab_width ? old_width : self->tab_width) + SUBCHAT_TITLE_ROOM;
    rect.bottom = self->tab_height + 10;
    ui_window_fill_canvas(wnd, CHAT_COLOUR_KEY, &rect);
    tell_battle_options(self, 2);
    string_free(&text);
}

static void resize(struct UISubChatHisWnd *self, int width, int height)
{
    struct UIWindow *wnd = &self->frame.base;

    if (width > SUBCHAT_MAX_WIDTH)
        width = SUBCHAT_MAX_WIDTH;
    if (height > SUBCHAT_MAX_HEIGHT)
        height = SUBCHAT_MAX_HEIGHT;
    if (width < SUBCHAT_MIN_WIDTH)
        width = SUBCHAT_MIN_WIDTH;
    if (height < SUBCHAT_MIN_HEIGHT)
        height = SUBCHAT_MIN_HEIGHT;
    width = (width - SUBCHAT_MIN_WIDTH) / SUBCHAT_SIZE_STEP * SUBCHAT_SIZE_STEP + SUBCHAT_MIN_WIDTH;
    height = (height - SUBCHAT_MIN_HEIGHT) / SUBCHAT_SIZE_STEP * SUBCHAT_SIZE_STEP + SUBCHAT_MIN_HEIGHT;
    if (width == wnd->width && height == wnd->height)
        return;
    ui_sub_chat_his_wnd_resize(wnd, width, height);
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_sub_chat_his_wnd_place_buttons(self);
}

static void info_set_default(struct WindowInfo *info, int id)
{
    if (id == 0)
        info->x = 0;
    else if (id == 1)
        info->x = 0x12c;
    info->width = 0x12c;
    info->y = g_renderer->height - 0x77;
    info->height = 0x5f;
}

static void close_tab(struct UISubChatHisWnd *self)
{
    struct ChatTab *where;
    struct Tree wnds;
    int key, i;

    where = chat_tabs_find(&g_session1.subChatWnds, self->key);
    if (!where)
        return;
    key = where->key;
    chat_tabs_remove(&g_session1.subChatWnds, where->key);
    if ((unsigned)key != g_session1.subChatWnds.size)
        chat_tabs_renumber(&g_session1.subChatWnds);
    ui_window_mgr_close_sub_chat(&g_windowMgr, self->key);
    tell_battle_options(self, 1);
    if ((unsigned)self->key != g_windowMgr.sub_chat_wnds.size) {
        struct TreeNode *node;

        tree_copy(&wnds, &g_windowMgr.sub_chat_wnds, sizeof(struct SubChatWnd), NULL);
        tree_clear(&g_windowMgr.sub_chat_wnds, NULL);
        i = 0;
        for (node = tree_first(&wnds); node != wnds.head; node = tree_next(node)) {
            struct UIWindow *wnd = TREE_VALUE(node, struct SubChatWnd)->wnd;
            struct SubChatWnd *sub;

            ((struct UISubChatHisWnd *)wnd)->key = i;
            sub = tree_get(&g_windowMgr.sub_chat_wnds, &i, sizeof i, sizeof *sub, tree_less_int);
            sub->wnd = wnd;
            i++;
        }
        tree_free(&wnds, NULL);
    }
}

static void add_line(struct UISubChatHisWnd *self, const char *text, COLORREF colour, const char *kept)
{
    char extra[0x19];

    memset(extra, 0, sizeof extra);
    if (kept)
        strcpy(extra, kept);
    ui_chat_his_box_log_message(self->box, text, colour, extra);
    if (_mbsstr((const unsigned char *)text, (const unsigned char *)string_cstr(&text_tag_ITEML)))
        ui_chat_his_box_add_line(self->box, text, colour, extra);
    else
        ui_chat_his_box_add_wrapped(self->box, text, colour, extra);
}

int ui_sub_chat_his_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UISubChatHisWnd *self = (struct UISubChatHisWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case SUBCHAT_MSG_ENTER:
        if (sender == self->rename && self->rename->x != OFF_SCREEN)
            rename_tab(self);
        else if (g_windowMgr.new_chat_wnd)
            ui_window_send_msg(g_windowMgr.new_chat_wnd, wnd, SUBCHAT_MSG_ENTER, 0, 0, 0, 0);
        return 0;
    case SUBCHAT_MSG_PAGE_UP:
    case SUBCHAT_MSG_PAGE_DOWN:
        if (self->box->list.has_vscroll)
            ui_chat_his_box_send_msg(&self->box->list.base, NULL, msg == SUBCHAT_MSG_PAGE_UP ? UI_MSG_PAGE_UP : UI_MSG_PAGE_DOWN, 0,
                     0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
        switch (val1) {
        case SUBCHAT_ID_BATTLE_OPTION: {
            struct UIWindow *options = ui_make_window(WND_ID_BATTLE_MSG_OPTION);

            if (options)
                ui_window_send_msg(options, wnd, UI_MSG_COMMAND, BATTLE_OPTION_FOR_TAB, self->key, 0, 0);
            break;
        }
        case SUBCHAT_ID_CLOSE:
            close_tab(self);
            break;
        case UI_ID_HISTORY_LINE:
            ui_window_mgr_set_whisper_name(&g_windowMgr, (const char *)val2);
            break;
        case SUBCHAT_ID_STICK:
            if (g_windowMgr.new_chat_wnd)
                ui_window_send_msg(g_windowMgr.new_chat_wnd, wnd, CHAT_MSG_STICK_TAB, 0, 0, 0, 0);
            break;
        }
        return 0;
    case UI_MSG_RESIZE:
        if (CHAT_TABS_EDITABLE && val1 == SUBCHAT_RESIZER_ID)
            resize(self, val2, val3);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!ui_info_on_screen(self->info))
            info_set_default(self->info, wnd->id);
        ui_sub_chat_his_wnd_move(wnd, self->info->x, self->info->y);
        ui_sub_chat_his_wnd_resize(wnd, self->info->width, self->info->height);
        return 0;
    case SUBCHAT_MSG_ADD_LINE:
        add_line(self, (const char *)val1, val2, (const char *)val3);
        return 0;
    case UI_MSG_HISTORY_DRAW:
        if (sender == (struct UIWindow *)self->box && wnd->height > self->tab_height) {
            RECT rect;

            rect.left = self->box->list.base.x;
            rect.top = self->box->list.base.y;
            rect.right = wnd->width - 3;
            rect.bottom = wnd->height - self->bottom;
            ui_window_fill_canvas(wnd, CHAT_COLOUR_KEY, &rect);
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UISubChatMiniWnd {
    struct UIFrameWnd frame;
    int count;
    int cell_width, cell_height;
};
SIZE_ASSERT(UISubChatMiniWnd, 0xa0);

#define SUBCHAT_MINI_ID_TOGGLE 0xc2
#define SUBCHAT_MINI_ID_PLACE 0xbd
#define SUBCHAT_MINI_TOGGLED 0x107

struct UISubChatMiniWnd *ui_sub_chat_mini_wnd_construct(struct UISubChatMiniWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_sub_chat_mini_wnd_ops;
    self->count = 2;
    self->cell_width = 0x20;
    self->cell_height = 0x20;
    return self;
}

struct UIWindow *ui_sub_chat_mini_wnd_new(void)
{
    return (struct UIWindow *)ui_sub_chat_mini_wnd_construct(mem_alloc(sizeof(struct UISubChatMiniWnd)));
}

void ui_sub_chat_mini_wnd_destroy(struct UIWindow *window)
{
    struct UISubChatMiniWnd *self = (struct UISubChatMiniWnd *)window;
    self->frame.base.ops = &ui_sub_chat_mini_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_sub_chat_mini_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISubChatMiniWnd *self = (struct UISubChatMiniWnd *)window;
    (void)width;
    (void)height;
    ui_window_resize_base(&self->frame.base, self->cell_width * self->count, self->cell_height);
}

void ui_sub_chat_mini_wnd_on_draw(struct UIWindow *window)
{
    struct UISubChatMiniWnd *self = (struct UISubChatMiniWnd *)window;
    struct BitmapRes *st =
        ui_skin_bitmap(g_windowMgr.on_st_sub_chat ? BASIC_INTERFACE "st_a.tga" : BASIC_INTERFACE "st_b.tga");
    struct BitmapRes *bt =
        ui_skin_bitmap(g_windowMgr.on_bt_sub_chat ? BASIC_INTERFACE "bt_a.tga" : BASIC_INTERFACE "bt_b.tga");

    ui_window_draw_bitmap(&self->frame.base, 0, 0, st, 0);
    ui_window_draw_bitmap(&self->frame.base, self->cell_width, 0, bt, 0);
}

void ui_sub_chat_mini_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISubChatMiniWnd *self = (struct UISubChatMiniWnd *)window;
    (void)y;
    if (x > 0 && x <= self->cell_width)
        g_windowMgr.on_st_sub_chat = g_windowMgr.on_st_sub_chat == 0;
    else
        g_windowMgr.on_bt_sub_chat = g_windowMgr.on_bt_sub_chat == 0;
    ui_sub_chat_mini_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, SUBCHAT_MINI_TOGGLED, 0, 0, 0);
    ui_window_invalidate_base(&self->frame.base);
}

int ui_sub_chat_mini_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UISubChatMiniWnd *self = (struct UISubChatMiniWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    if (val1 == SUBCHAT_MINI_ID_PLACE)
        ui_frame_wnd_move(&self->frame.base, val2, val3 - self->cell_height);
    else if (val1 == SUBCHAT_MINI_ID_TOGGLE)
        ui_sub_chat_mini_wnd_on_l_btn_down(&self->frame.base, self->cell_width * val2 + 1, 0);
    return 0;
}
