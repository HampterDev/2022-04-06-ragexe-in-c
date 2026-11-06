#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "contents/manner_filter.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <mbstring.h>
#include "ui/chat/chat.h"
#include "ui/chat/chat_history.h"
#include "ui/chat/whisper.h"
#include "core/app.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "render/screen_draw.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "ui/chat/chat_window.h"
#include "ui/wndmgr/wndmgr.h"

struct UIWhisperWnd;
struct UIWindow;
void ui_whisper_wnd_destroy(struct UIWindow *window);
void ui_whisper_wnd_move(struct UIWindow *self, int x, int y);
void ui_whisper_wnd_store_info(struct UIWindow *self);
void ui_whisper_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_whisper_wnd_on_draw(struct UIWindow *self);
void ui_whisper_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_whisper_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
int ui_whisper_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_whisper_wnd_draw_backdrop(struct UIWindow *window);
const struct UIWindowOps ui_whisper_wnd_ops = {
    .destroy = ui_whisper_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_whisper_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_whisper_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_whisper_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_whisper_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_whisper_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_whisper_wnd_on_r_btn_up,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_whisper_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 1,
    .draw_backdrop = ui_whisper_wnd_draw_backdrop,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define WHISPER_RESIZER_ID 7
#define WHISPER_TITLE_HEIGHT 0x11
#define WHISPER_MIN_WIDTH 0x118
#define WHISPER_MIN_HEIGHT 0x78
#define WHISPER_MAX_WIDTH 0x200
#define WHISPER_MAX_HEIGHT 0x180
#define WHISPER_SIZE_STEP 32
#define WHISPER_DEFAULT_X 0
#define WHISPER_DEFAULT_Y 0x15b

#define WHISPER_FRAME_COLOUR 0xc0c0c0
#define WHISPER_BACK_ARGB 0x66000000
#define WHISPER_TITLE_ARGB 0x449eb6e4
#define WHISPER_OWN_LINE 0x7800
#define WHISPER_OWN_COLOUR 0xffff
#define WHISPER_OTHER_COLOUR 0xe8ddb6
#define WHISPER_COPIED_COLOUR 0x64ffff

#define WHISPER_MSG_PAGE_UP 3
#define WHISPER_MSG_PAGE_DOWN 4
#define WHISPER_MSG_ADD_LINE 0x25
#define WHISPER_MSG_OPEN_MENU 0x31
#define WHISPER_MSG_RESOLVE_PENDING_NAME 0x9a

#define WHISPER_ID_SEND 0xb0

#define WND_ID_MENU 0x12
#define MENU_MSG_FIT 0x1e
#define MENU_MSG_ADD_ITEM 0x1f
#define MENU_MSG_CHOICE 0x30
#define WHISPER_MENU_SAVE 0xb
#define WHISPER_MENU_ADD_FRIEND 0x15
#define MSG_WHISPER_MENU_SAVE 0x138
#define MSG_WHISPER_MENU_ADD_FRIEND 0x166

#define MSG_WHISPER_ITEM_LINK 0xbe2
#define MSG_TEXT_NOT_ASCII 0xbe
#define MODE_REQ_WHISPER 0xb
#define MODE_REQ_TALK 0x2a
#define MODE_REQ_MENU_NAME 0x65
#define TALK_TYPE_CLOSE 0x18
#define TALK_TYPE_SAVE 0x33
#define UIWM_MSG_SHOW_CHAT 3

struct UIWhisperWnd *ui_whisper_wnd_construct(struct UIWhisperWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_whisper_wnd_ops;
    string_init(&self->name);
    string_init(&self->title_name);
    memset(&self->menu, 0, sizeof self->menu);
    string_init(&self->code);
    self->info = NULL;
    return self;
}

void ui_whisper_wnd_destruct(struct UIWhisperWnd *self)
{
    self->frame.base.ops = &ui_whisper_wnd_ops;
    string_free(&self->code);
    ui_menu_entry_vec_free(&self->menu);
    string_free(&self->title_name);
    string_free(&self->name);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_whisper_wnd_destroy(struct UIWindow *window)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    ui_whisper_wnd_destruct(self);
    mem_free(self);
}

void ui_whisper_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    ui_frame_wnd_move((struct UIWindow *)(&self->frame), x, y);
}

void ui_whisper_wnd_store_info(struct UIWindow *window)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String name;

    if (self->info) {
        self->info->width = wnd->width;
        self->info->height = wnd->height;
        self->info->x = wnd->x;
        self->info->y = wnd->y;
    }
    string_copy(&name, &self->name);
    ui_window_mgr_forget_whisper_wnd(&g_windowMgr, name);
}

void ui_whisper_wnd_on_draw(struct UIWindow *window)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char title[0x80];

    if (self->code.size)
        sprintf(title, "With  %s [%s]", string_cstr(&self->title_name), string_cstr(&self->code));
    else
        sprintf(title, "With  %s", string_cstr(&self->title_name));
    ui_window_draw_round_frame(wnd, 0, 0, wnd->width, wnd->height, WHISPER_FRAME_COLOUR);
    ui_window_fill_rect(wnd, 1, wnd->height - 0x14, wnd->width - 2, 1, WHISPER_FRAME_COLOUR);
    ui_window_text_out(wnd, 10, 3, title, 0, 0, 0xc, 0xffffff, 0, 0);
}

void ui_whisper_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    HGLOBAL mem;

    (void)x;
    if (y >= WHISPER_TITLE_HEIGHT || !self->code.size)
        return;
    mem = GlobalAlloc(GHND, self->code.size + 1);
    memcpy(GlobalLock(mem), string_cstr(&self->code), self->code.size);
    GlobalUnlock(mem);
    if (!OpenClipboard(g_hMainWnd))
        return;
    EmptyClipboard();
    SetClipboardData(CF_TEXT, mem);
    CloseClipboard();
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)"C-Code copied to clipboard",
                        WHISPER_COPIED_COLOUR, 0, 0);
}

void ui_whisper_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    (void)x;
    (void)y;
    ui_whisper_wnd_send_msg(&self->frame.base, NULL, WHISPER_MSG_OPEN_MENU, 0, 0, 0, 0);
}

void ui_whisper_wnd_draw_backdrop(struct UIWindow *window)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;

    struct UIWindow *wnd = &self->frame.base;
    int x = 0, y = 0;

    ui_window_to_screen(wnd, &x, &y);
    draw_box_screen(x + 1, y + 1, wnd->width - 2, wnd->height, WHISPER_BACK_ARGB);
    draw_box_screen(x + 1, y + 1, wnd->width - 2, WHISPER_TITLE_HEIGHT, WHISPER_TITLE_ARGB);
}

static int info_is_usable(const struct WindowInfo *info)
{
    if (info->width < WHISPER_MIN_WIDTH || info->height < WHISPER_MIN_HEIGHT)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - info->width)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - info->height)
        return 0;
    return 1;
}

static void info_set_default(struct WindowInfo *info)
{
    info->width = WHISPER_MIN_WIDTH;
    info->height = WHISPER_MIN_HEIGHT;
    info->x = WHISPER_DEFAULT_X;
    info->y = WHISPER_DEFAULT_Y;
}

static void save_conversation(struct UIWhisperWnd *self)
{
    char path[0x800];
    struct String name;
    unsigned i;
    int n = 0;

    sprintf(path, "%s\\Whisper", g_clientDir);
    if (_access(path, 0) && _mkdir(path))
        return;
    string_copy(&name, &self->name);
    for (i = 0; i < name.size; i++) {
        char *c = name.data + i;

        if (strchr("\\/:*?\"<>|", *c) && *c)
            *c = '_';
    }
    sprintf(path, "%s\\Whisper\\%s.txt", g_clientDir, string_cstr(&name));
    if (!_access(path, 0)) {
        do {
            n++;
            sprintf(path, "%s\\Whisper\\%s%03d.txt", g_clientDir, string_cstr(&name), n);
        } while (!_access(path, 0));
    }
    if (ui_chat_his_box_save_to_file(&self->box->box, path)) {
        struct String said;

        string_init(&said);
        string_assign_cstr(&said, path);
        string_append_cstr(&said, " is Saved.");
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)string_cstr(&said), 0xff00, 0, 0);
        string_free(&said);
    }
    string_free(&name);
}

static void add_menu_entry(struct UIWhisperWnd *self, int msg, int id)
{
    struct String text;
    struct UIMenuEntry entry;

    string_init(&text);
    string_assign_cstr(&text, msg_str(msg));
    ui_menu_entry_construct(&entry, string_cstr(&text), id);
    ui_menu_entry_vec_push_move(&self->menu, &entry);
    string_free(&entry.text);
    string_free(&text);
}

static void open_menu(struct UIWhisperWnd *self)
{
    struct UIMenuEntry *entry;
    struct UIWindow *menu;
    unsigned i;
    int x, y;

    for (entry = MENU_ENTRIES(&self->menu); entry != (struct UIMenuEntry *)self->menu.last; entry++)
        string_free(&entry->text);
    self->menu.last = self->menu.first;
    if (g_windowMgr.menu_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_MENU);
    add_menu_entry(self, MSG_WHISPER_MENU_SAVE, WHISPER_MENU_SAVE);
    if (!ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&self->name)) && g_serverType != 3)
        add_menu_entry(self, MSG_WHISPER_MENU_ADD_FRIEND, WHISPER_MENU_ADD_FRIEND);
    mode_send_message(client_current_mode(), MODE_REQ_MENU_NAME, (int)string_cstr(&self->name), 0, 0, 0);
    if (self->menu.first == self->menu.last)
        return;

    menu = ui_window_open(WND_ID_MENU);
    for (i = 0; i < MENU_ENTRY_COUNT(&self->menu); i++)
        ui_window_send_msg(menu, NULL, MENU_MSG_ADD_ITEM, (int)string_cstr(&MENU_ENTRIES(&self->menu)[i].text),
                 0, 0, 0);
    ui_window_send_msg(menu, NULL, MENU_MSG_FIT, 0, 0, 0, 0);
    x = g_mouse.x + 1;
    if (x < 1)
        x = 1;
    if (x > g_renderer->width - menu->width)
        x = g_renderer->width - menu->width;
    y = g_mouse.y + 1;
    if (y < 1)
        y = 1;
    if (y > g_renderer->height - menu->height)
        y = g_renderer->height - menu->height;
    ui_window_move(menu, x, y);
}

static void send_line(struct UIWhisperWnd *self)
{
    struct UIWindow *edit = &self->edit->edit.base;
    char line[0x100];
    struct String text;
    int has_link;

    string_init(&text);
    string_assign_cstr(&text, ui_edit_ctrl_get_text(edit));
    has_link = ui_chat_edit_ctrl_has_item_link(self->edit, &text);
    string_free(&text);
    if (has_link == 1) {
        ui_edit_ctrl_set_text_default((struct UIEditCtrl *)edit, "");
        ui_chat_line(msg_str(MSG_WHISPER_ITEM_LINK), 0xff);
        return;
    }
    strcpy(line, ui_edit_ctrl_get_text(edit));
    ui_chat_edit_ctrl_commit(self->edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, edit);
    if (!strlen(line))
        return;
    if (g_serviceType == 1 && !session_is_ascii_name(&g_session1, line)) {
        ui_chat_line(msg_str(MSG_TEXT_NOT_ASCII), 0xff);
        return;
    }

    if (self->name.size && line[0] != '/') {
        if (((struct UIEditCtrl *)edit)->navigation_link) {
            struct TextView link = ui_edit_ctrl_link_text((struct UIEditCtrl *)edit);

            ui_window_mgr_set_chat_line(&g_windowMgr, link.data);
        } else if (self->edit->popup) {
            ui_window_mgr_set_chat_line(&g_windowMgr, string_cstr(&self->edit->popup->linked_text));
        } else {
            string_assign_cstr(&g_windowMgr.chat_line, line);
        }
        mode_send_message(client_current_mode(), MODE_REQ_WHISPER, (int)string_cstr(&self->name), 0, 0, 0);
        return;
    }

    if (!session_is_emotion_text(&g_session1, line)) {
        struct String parts[3];
        int type, at, i;

        for (i = 0; i < 3; i++)
            string_init(&parts[i]);
        at = session_get_talk_type(&g_session1, line, &type, parts);
        if (at != -1)
            ui_window_mgr_set_chat_line(&g_windowMgr, line + at);
        if (type == TALK_TYPE_CLOSE)
            ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
        else if (type == TALK_TYPE_SAVE)
            save_conversation(self);
        else
            mode_send_message(client_current_mode(), MODE_REQ_TALK, type, (int)parts, 0, 0);
        for (i = 3; i-- > 0;)
            string_free(&parts[i]);
    }
}

static void add_line(struct UIWhisperWnd *self, char *text, int own)
{
    COLORREF colour;

    if (word_filter_contains(&g_nameFilter, text) == 1) {
        word_filter_censor(&g_nameFilter, text);
    }
    colour = own == WHISPER_OWN_LINE ? WHISPER_OWN_COLOUR : WHISPER_OTHER_COLOUR;
    if (_mbsstr((const unsigned char *)text, (const unsigned char *)string_cstr(&text_tag_ITEML)))
        ui_chat_his_box_add_line(&self->box->box, text, colour, NULL);
    else
        ui_chat_his_box_add_wrapped(&self->box->box, text, colour, NULL);
    ui_chat_his_box_log_message(&self->box->box, text, colour, "");
}

static void resize(struct UIWhisperWnd *self, int width, int height)
{
    struct UIWindow *wnd = &self->frame.base;

    if (width > WHISPER_MAX_WIDTH)
        width = WHISPER_MAX_WIDTH;
    if (height > WHISPER_MAX_HEIGHT)
        height = WHISPER_MAX_HEIGHT;
    if (width < WHISPER_MIN_WIDTH)
        width = WHISPER_MIN_WIDTH;
    if (height < WHISPER_MIN_HEIGHT)
        height = WHISPER_MIN_HEIGHT;
    width = (width - WHISPER_MIN_WIDTH) / WHISPER_SIZE_STEP * WHISPER_SIZE_STEP + WHISPER_MIN_WIDTH;
    height = (height - WHISPER_MIN_HEIGHT) / WHISPER_SIZE_STEP * WHISPER_SIZE_STEP + WHISPER_MIN_HEIGHT;
    if (width != wnd->width || height != wnd->height) {
        ui_window_resize_base(wnd, width, height);
        ui_window_move_base(self->resizer, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_resize_base(&self->edit->edit.base, wnd->width - 0x1e, 0x10);
        ui_window_move_base(&self->edit->edit.base, 3, wnd->height - 0x12);
        ui_chat_his_box_resize_to(&self->box->box, wnd->width - 2, wnd->height - 0x26, 1);
        ui_window_move_base((struct UIWindow *)self->box, 1, WHISPER_TITLE_HEIGHT);
        ui_window_move_base(self->close, wnd->width - 0xe, 3);
    }
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

int ui_whisper_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case WHISPER_MSG_PAGE_UP:
    case WHISPER_MSG_PAGE_DOWN:
        if (self->box->box.list.has_vscroll)
            ui_chat_his_box_send_msg((struct UIWindow *)self->box, NULL,
                     msg == WHISPER_MSG_PAGE_UP ? UI_MSG_PAGE_UP : UI_MSG_PAGE_DOWN, 0, 0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
        if (val1 == WHISPER_ID_SEND)
            send_line(self);
        else if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
        return 0;
    case UI_MSG_RESIZE:
        if (val1 == WHISPER_RESIZER_ID)
            resize(self, val2, val3);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!info_is_usable(self->info))
            info_set_default(self->info);
        ui_whisper_wnd_send_msg(wnd, NULL, UI_MSG_RESIZE, WHISPER_RESIZER_ID, self->info->width, self->info->height, 0);
        ui_whisper_wnd_move(wnd, self->info->x, self->info->y);
        return 0;
    case WHISPER_MSG_ADD_LINE:
        add_line(self, (char *)val1, val2);
        return 0;
    case MENU_MSG_CHOICE:
        if (MENU_ENTRIES(&self->menu)[val1].id == WHISPER_MENU_SAVE)
            save_conversation(self);
        return 0;
    case WHISPER_MSG_OPEN_MENU:
        open_menu(self);
        return 0;
    case WHISPER_MSG_RESOLVE_PENDING_NAME: {
        ui_chat_his_box_resolve_pending_name(&self->box->box, (const char *)val1);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define WHISPER_WND_ID 0xb0
#define WHISPER_EDIT_ID 0x1e2
#define WHISPER_GRIP_EDGE 7

void ui_whisper_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIWhisperWnd *self = (struct UIWhisperWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIResizer *grip;
    struct UIChatEditCtrl *edit;
    struct UIChatHisBox2 *box;
    void *mem;

    (void)width;
    (void)height;
    self->frame.default_id = WHISPER_WND_ID;

    mem = mem_alloc(sizeof(struct UIResizer));
    grip = mem ? ui_resizer_construct(mem) : NULL;
    self->resizer = &grip->base;
    ui_window_create(&grip->base, 0x10, 0x10);
    ui_resizer_set_edge(grip, WHISPER_GRIP_EDGE);
    ui_resizer_set_bitmap(grip, "유저인터페이스\\btn_resize.bmp");
    ui_window_move_base(&grip->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(grip, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, &grip->base);

    mem = mem_alloc(sizeof(struct UIChatEditCtrl));
    edit = mem ? ui_chat_edit_ctrl_construct(mem) : NULL;
    self->edit = edit;
    ui_window_create((struct UIWindow *)edit, wnd->width - 0x1e, 0x10);
    ui_window_move_base((struct UIWindow *)edit, 3, wnd->height - 0x12);
    edit->edit.max_chars = 0xff;
    ui_edit_ctrl_set_back_colour((struct UIWindow *)edit, 0xff, 0, 0xff);
    ui_edit_ctrl_set_text_colour((struct UIWindow *)edit, 0xff, 0xff, 0xff, -1);
    ui_window_add_child(wnd, (struct UIWindow *)edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, (struct UIWindow *)edit);
    ((struct UIWindow *)edit)->id = WHISPER_EDIT_ID;

    mem = mem_alloc(sizeof(struct UIChatHisBox2));
    box = mem ? ui_chat_his_box2_construct(mem) : NULL;
    self->box = box;
    ui_chat_his_box2_create(&box->box.list, 0, 0, wnd->width - 2, wnd->height - 0x26, 1);
    ui_window_move_base((struct UIWindow *)box, 1, 0x11);
    ((struct UIWindow *)box)->id = WHISPER_WND_ID;
    box->back_r = 0xff;
    box->back_g = 0;
    box->back_b = 0xff;
    ui_window_add_child(wnd, (struct UIWindow *)box);

    self->close = ui_window_add_close_button(wnd, "/q", 0);
}
