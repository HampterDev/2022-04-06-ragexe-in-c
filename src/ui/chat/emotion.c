#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "session/chat_macros.h"
#include "session/hotkey_bindings.h"
#include "contents/client_lua_calls.h"
#include "contents/account_settings.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/chat/chat.h"
#include "ui/chat/chat_window_parts.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct Vec;
struct UIEmotionWnd;
struct UIWindow;
void ui_emotion_wnd_destroy(struct UIWindow *window);
void ui_emotion_wnd_store_info(struct UIWindow *self);
void ui_emotion_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_emotion_wnd_on_draw(struct UIWindow *self);
void ui_emotion_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_emotion_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_emotion_wnd_ops = {
    .destroy = ui_emotion_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_emotion_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_emotion_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_emotion_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_emotion_wnd_on_l_btn_down,
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
    .send_msg = ui_emotion_wnd_send_msg,
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
struct UIEmotionListWnd;
void ui_emotion_list_wnd_destroy(struct UIWindow *window);
void ui_emotion_list_wnd_store_info(struct UIWindow *self);
void ui_emotion_list_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_emotion_list_wnd_on_process(struct UIWindow *self);
void ui_emotion_list_wnd_on_draw(struct UIWindow *self);
void ui_emotion_list_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_emotion_list_wnd_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused);
void ui_emotion_list_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
intptr_t ui_emotion_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_emotion_list_wnd_ops = {
    .destroy = ui_emotion_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_emotion_list_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_emotion_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_emotion_list_wnd_on_process,
    .on_draw = ui_emotion_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_emotion_list_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_emotion_list_wnd_on_l_btn_up,
    .on_mouse_move = ui_emotion_list_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_emotion_list_wnd_send_msg,
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

#define HOTKEY_CATEGORY_CHAT_MACRO 2

#define MSG_CHAT_MACRO_TITLE 0x23e
#define CHAT_MACRO_ID_VIEW 0x113

struct UIEmotionWnd {
    struct UIFrameWnd frame;
    int focus_index;
    struct UIWindow *key_label[CHAT_MACRO_COUNT];
    struct UIWindow *macro_edit[CHAT_MACRO_COUNT];
    struct WindowInfo *info;
    struct UIWindow *view_button;
};

void ui_emotion_wnd_save_macros(struct UIEmotionWnd *self);



struct UIWindow *ui_emotion_wnd_new(void)
{
    struct UIEmotionWnd *self = mem_alloc(sizeof *self);

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_emotion_wnd_ops;
    return &self->frame.base;
}

void ui_emotion_wnd_destroy(struct UIWindow *window)
{
    struct UIEmotionWnd *self = (struct UIEmotionWnd *)window;
    self->frame.base.ops = &ui_emotion_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_emotion_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIEmotionWnd *self = (struct UIEmotionWnd *)window;
    static const char *const view_bitmaps[3] = {
        UI_SKIN "btn_view.bmp", UI_SKIN "btn_view_a.bmp", UI_SKIN "btn_view_b.bmp",
    };
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *view;
    char key[0x10];
    int i, y, state;

    for (i = 0, y = 0x17; y < 0xe9; i++, y += 0x15) {
        struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));
        struct UIWindow *label;

        if (edit)
            edit = ui_edit_ctrl2_construct(edit);
        self->macro_edit[i] = edit;
        ui_window_create(edit, 0x10e, 0x10);
        ui_window_move(self->macro_edit[i], 0x53, y - 3);
        UI_EDIT_MAX_CHARS(self->macro_edit[i]) = 0x32;
        ui_edit_ctrl_set_back_colour(self->macro_edit[i], 0xe8, 0xe8, 0xe8);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->macro_edit[i], session_chat_macro(&g_session1, i).data);
        ui_window_add_child(wnd, self->macro_edit[i]);

        sprintf(key, "Alt + %d", i + 1);
        label = mem_alloc(sizeof(struct UIStaticText));
        if (label)
            label = ui_static_text_construct(label);
        self->key_label[i] = label;
        ui_window_create(label, 0x46, 0x10);
        ui_window_move(self->key_label[i], 0x14, y);
        ui_static_text_set_text(self->key_label[i], key, 0);
        ui_window_add_child(wnd, self->key_label[i]);
    }
    ui_static_text_set_text(self->key_label[9], "Alt + 0", 0);
    ui_window_add_close_button(wnd, "Alt+M", 0);

    view = mem_alloc(sizeof(struct UIBitmapButton));
    if (view)
        view = ui_bitmap_button_construct(view);
    self->view_button = &view->button.base;
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
        ui_bitmap_button_set_bitmap((struct UIBitmapButton *)self->view_button, view_bitmaps[state], state, 0);
    view = (struct UIBitmapButton *)self->view_button;
    ui_window_create(&view->button.base, view->bitmap_width, view->bitmap_height);
    ui_window_move_base(self->view_button, wnd->width - 0x32, wnd->height - 0x19);
    self->view_button->id = CHAT_MACRO_ID_VIEW;
    ui_window_add_child(wnd, self->view_button);

    ui_window_mgr_set_focus_edit(&g_windowMgr, self->macro_edit[0]);
    ui_edit_ctrl_set_caret(self->macro_edit[0], 0xff);
    self->focus_index = 0;

    for (i = 0; i < CHAT_MACRO_COUNT; i++) {
        struct HotKey hotkey;

        session_get_hot_key(&g_session1, &hotkey, HOTKEY_CATEGORY_CHAT_MACRO, i);
        if (hotkey.action.size != 0)
            ui_static_text_set_text(self->key_label[i], string_cstr(&hotkey.label), 0);
        string_free(&hotkey.action);
        string_free(&hotkey.label);
    }
}

void ui_emotion_wnd_on_draw(struct UIWindow *window)
{
    struct UIEmotionWnd *self = (struct UIEmotionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int top = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_CHAT_MACRO_TITLE), 0);

    ui_window_draw_body(wnd, top, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

void ui_emotion_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIEmotionWnd *self = (struct UIEmotionWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

intptr_t ui_emotion_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                    intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIEmotionWnd *self = (struct UIEmotionWnd *)window;
    switch (msg) {
    case 0:
        ui_emotion_wnd_save_macros(self);
        return 0;
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE) {
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_CHAT_MACRO);
        } else if (val1 == CHAT_MACRO_ID_VIEW) {
            if (((struct UIEmotionListWnd *)g_windowMgr.emotion_list_wnd) == NULL)
                ui_window_open(WND_ID_EMOTION_LIST);
            else
                ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_EMOTION_LIST);
        }
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!ui_info_on_screen(self->info)) {
            self->info->x = 200;
            self->info->y = 0x96;
            self->info->width = 0x177;
            self->info->height = 0x118;
        }
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_emotion_wnd_store_info(struct UIWindow *window)
{
    struct UIEmotionWnd *self = (struct UIEmotionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (self->info) {
        self->info->x = wnd->x;
        self->info->y = wnd->y;
        self->info->width = wnd->width;
        self->info->height = wnd->height;
    }
    ui_emotion_wnd_save_macros(self);
}

void ui_emotion_wnd_set_macro_text(struct UIEmotionWnd *self, const char *text, int index)
{
    if (index == -1) {
        struct UIWindow *focus = ui_window_mgr_get_focus_edit(&g_windowMgr);
        int i;

        for (i = 0; i < CHAT_MACRO_COUNT; i++)
            if (focus == self->macro_edit[i])
                index = i;
        if (index == -1)
            return;
    }
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->macro_edit[index], text);
}

void ui_emotion_wnd_save_macros(struct UIEmotionWnd *self)
{
    int i;
    for (i = 0; i < CHAT_MACRO_COUNT; i++) {
        const char *text = ui_edit_ctrl_get_text(self->macro_edit[i]);
        if (!session_set_chat_macro(&g_session1, i, text_view_cstr(text)))
            return;
    }
    account_user_data_save(g_accountUserDataMgr);
}

#define MSG_EMOTION_LIST_TITLE 0x21e
#define MSG_EMOTION_PREV 0x41d
#define MSG_EMOTION_NEXT 0x41e
#define EMOTION_ID_ICON 0x113
#define EMOTION_ID_PREV 0x134
#define EMOTION_ID_NEXT 0x135
#define EMOTION_ICON_SIZE 0x28
#define EMOTION_CELL 0x2f
#define EMOTION_LEFT 0xb
#define EMOTION_TOP 0x1c
#define EMOTION_OFFSCREEN (-100)

struct UIEmotionListWnd {
    struct UIFrameWnd frame;
    int hover_drawn;
    int hover;
    struct WindowInfo *info;
    struct Vec names;
    int count;
    int per_page;
    struct UIWindow **icons;
    struct UIWindow *prev_button;
    struct UIWindow *next_button;
    int page;
    int pages;
};

struct UIEmotionListWnd *ui_emotion_list_wnd_construct(struct UIEmotionListWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_emotion_list_wnd_ops;
    self->names.first = self->names.last = self->names.end = NULL;
    self->page = 1;
    client_lua_call(g_session1.luaState, "GetEmotionOrderListSize", ">d", &self->count);
    self->per_page = 30;
    self->pages = self->count / 30;
    if (self->count % 30 != 0)
        self->pages++;
    self->icons = mem_alloc((unsigned)self->count > 0x3fffffff ? 0xffffffffu : (unsigned)self->count * 4);
    return self;
}

struct UIWindow *ui_emotion_list_wnd_new(void)
{
    return (struct UIWindow *)ui_emotion_list_wnd_construct(mem_alloc(sizeof(struct UIEmotionListWnd)));
}

void ui_emotion_list_wnd_destroy(struct UIWindow *window)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    self->frame.base.ops = &ui_emotion_list_wnd_ops;
    vec_free(&self->names);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static void emotion_list_layout(struct UIEmotionListWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    char line[0x10];
    int i, x = EMOTION_LEFT, y = EMOTION_TOP, text_x;

    for (i = 0; i < self->count; i++) {
        if ((self->page - 1) * self->per_page <= i && i < self->page * self->per_page) {
            ui_window_move(self->icons[i], x, y);
            x += EMOTION_CELL;
            if (x + EMOTION_ICON_SIZE >= wnd->width) {
                x = EMOTION_LEFT;
                y += EMOTION_CELL;
            }
        } else {
            ui_window_move(self->icons[i], EMOTION_OFFSCREEN, EMOTION_OFFSCREEN);
        }
    }
    sprintf(line, "%d / %d", self->page, self->pages);
    text_x = (wnd->width - ui_window_text_width(wnd, line, 0, 0, 0xc, 0, 0)) / 2;
    if (self->page > 1)
        ui_window_move_base(self->prev_button, text_x - self->prev_button->width - 10, wnd->height - 0x16);
    else
        ui_window_move_base(self->prev_button, EMOTION_OFFSCREEN, EMOTION_OFFSCREEN);
    if (self->page < self->pages)
        ui_window_move_base(self->next_button, ui_window_text_width(wnd, line, 0, 0, 0xc, 0, 0) + 10 + text_x,
                 wnd->height - 0x16);
    else
        ui_window_move_base(self->next_button, EMOTION_OFFSCREEN, EMOTION_OFFSCREEN);
    self->hover = -1;
    ui_window_invalidate_base(&self->frame.base);
}

static struct UIWindow *add_page_button(struct UIEmotionListWnd *self, int text_id, int id)
{
    struct UIWindow *button = (struct UIWindow *)ui_text_button_create();
    ui_text_button_set_text(button, msg_str(text_id), 0x9fb3e1, 0, 0xc, 0, 0);
    ((struct UITextButton *)(button))->hover_colour = 0x9a9aff;
    ui_window_move_base(button, EMOTION_OFFSCREEN, EMOTION_OFFSCREEN);
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)button, msg_str(text_id));
    button->id = id;
    ui_window_add_child(&self->frame.base, button);
    return button;
}

void ui_emotion_list_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int i;

    self->hover = -1;
    self->hover_drawn = -1;
    ui_window_add_close_button(wnd, "Alt+L", 0);
    for (i = 0; i < self->count; i++) {
        struct UIWindow *icon = mem_alloc(sizeof(struct UISpriteButton));
        int action = 0;

        if (icon)
            icon = ui_sprite_button_construct(icon);
        self->icons[i] = icon;
        ui_window_create(self->icons[i], EMOTION_ICON_SIZE, EMOTION_ICON_SIZE);
        ui_sprite_button_set_sprite(self->icons[i], "이팩트\\emotion.spr");
        ui_sprite_button_set_act(self->icons[i], "이팩트\\emotion.act");
        ui_window_move(self->icons[i], EMOTION_OFFSCREEN, EMOTION_OFFSCREEN);
        ((struct UISpriteButton *)(self->icons[i]))->motion = 5;
        if (client_lua_call(g_session1.luaState, "GetEmotionActionByWndIdx", "d>d", i, &action) && action >= 0)
            ((struct UISpriteButton *)(self->icons[i]))->action = action;
        self->icons[i]->id = EMOTION_ID_ICON;
        ui_window_add_child(wnd, self->icons[i]);
    }
    self->prev_button = add_page_button(self, MSG_EMOTION_PREV, EMOTION_ID_PREV);
    self->next_button = add_page_button(self, MSG_EMOTION_NEXT, EMOTION_ID_NEXT);
    emotion_list_layout(self);
}

static int emotion_columns(struct UIEmotionListWnd *self)
{
    return (self->frame.base.width - EMOTION_LEFT) / EMOTION_CELL;
}

void ui_emotion_list_wnd_on_draw(struct UIWindow *window)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char line[0x10];
    int top = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_EMOTION_LIST_TITLE), 0);
    int hover = self->hover;

    ui_window_draw_body(wnd, top, 0, "");
    if (hover >= 0 && hover < self->count && hover < self->per_page) {
        int columns = emotion_columns(self);
        int row = hover / columns;
        int r, g, b;

        ui_skin_color_chip(&g_skinMgr, 0xe, 6, &r, &g, &b);
        ui_window_draw_round_frame(wnd, (hover - columns * row) * EMOTION_CELL + EMOTION_LEFT,
                                row * EMOTION_CELL + EMOTION_TOP, EMOTION_ICON_SIZE, EMOTION_ICON_SIZE,
                                RGB(r, g, b));
    }
    sprintf(line, "%d / %d", self->page, self->pages);
    ui_window_text_out(wnd, (wnd->width - ui_window_text_width(wnd, line, 0, 0, 0xc, 0, 0)) / 2,
                     wnd->height - 0x14, line, 0, 0, 0xc, 0, 0, 0);
}

static int emotion_index(struct UIEmotionListWnd *self)
{
    return (self->page - 1) * self->per_page + self->hover;
}

void ui_emotion_list_wnd_on_l_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    if (self->hover < self->count && self->hover >= 0)
        client_lua_call(g_session1.luaState, "ReqEmotionByIndex", "d", emotion_index(self));
}

void ui_emotion_list_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    struct UIEmotionWnd *macros;
    struct String text;
    const char *msg = NULL;
    int index;

    if (y < 0x11) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    index = emotion_index(self);
    macros = ((struct UIEmotionWnd *)g_windowMgr.emotion_wnd);
    if (self->hover >= self->count || self->hover < 0)
        return;
    string_init(&text);
    if (client_lua_call(g_session1.luaState, "GetEmotionMsg", "d>s", index, &msg))
        string_assign_cstr(&text, msg);
    if (macros == NULL) {
        ui_edit_ctrl_set_text_default((struct UIEditCtrl *)(ui_chat_window_edit(g_windowMgr.new_chat_wnd)), string_cstr(&text));
        ui_window_invalidate_base(ui_chat_window_edit(g_windowMgr.new_chat_wnd));
    } else {
        ui_emotion_wnd_set_macro_text(macros, string_cstr(&text), -1);
    }
    string_free(&text);
}

void ui_emotion_list_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    int hover;

    ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
    hover = emotion_columns(self) * ((y - 0x14) / EMOTION_CELL) + (x - EMOTION_LEFT) / EMOTION_CELL;
    self->hover = hover;
    if ((self->page - 1) * self->per_page + hover >= self->count) {
        self->hover = -1;
        hover = -1;
    }
    if (hover != self->hover_drawn) {
        ui_window_invalidate_base(&self->frame.base);
        self->hover_drawn = self->hover;
    }
}

void ui_emotion_list_wnd_on_process(struct UIWindow *window)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    int i;

    for (i = 0; i < self->count; i++)
        if (((struct UISpriteButton *)(self->icons[i]))->animating)
            ui_window_invalidate(self->icons[i]);
}

intptr_t ui_emotion_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                        intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_EMOTION_LIST);
        return 0;
    case UI_MSG_TEXT_BUTTON:
        if (val1 == EMOTION_ID_PREV) {
            if (self->page > 1)
                self->page--;
        } else if (val1 == EMOTION_ID_NEXT) {
            if (self->page < self->pages)
                self->page++;
        } else {
            return 0;
        }
        emotion_list_layout(self);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!ui_info_on_screen(self->info)) {
            self->info->x = 200;
            self->info->y = 200;
            self->info->width = 0xf6;
            self->info->height = 0x14a;
        }
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_emotion_list_wnd_store_info(struct UIWindow *window)
{
    struct UIEmotionListWnd *self = (struct UIEmotionListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
}
