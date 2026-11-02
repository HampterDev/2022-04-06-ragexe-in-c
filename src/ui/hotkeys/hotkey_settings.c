#include "contents/account_settings.h"
#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include "ui/chat/chat.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <stdio.h>
#include "ui/hotkeys/hotkeys.h"
#include "ui/chat/chat_window_parts.h"
#include "ui/chat/balloon.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_options.h"
#include "ui/chat/chat_window.h"
#include "ui/hotkeys/hotkeys_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "std/tree.h"

#define HOTKEY_PAGE_ROWS 36
#define HOTKEY_TEXT_WIDTH 0x82
#define HOTKEY_NOTE_CHARS 0x3c
#define HOTKEY_FONT_HEIGHT 0xc

#define HOTKEY_ID_RESET 0x163
#define HOTKEY_ID_PREV 0x134
#define HOTKEY_ID_NEXT 0x135
#define WND_TAB_RELAY 0x12f

#define KEY_SHIFT 0x10
#define KEY_CONTROL 0x11
#define KEY_ALT 0x12
#define KEY_ESCAPE 0x1b
#define KEY_SPACE 0x20
#define KEY_NUMLOCK 0x90
#define HOTKEY_INDEX_CHAT 0x1f

#define MSG_HOTKEY_TITLE 0x5d6
#define MSG_HOTKEY_ASK_KEY 0x5ce
#define MSG_HOTKEY_KEY_REFUSED 0x5cf
#define MSG_HOTKEY_KEY_INVALID 0x5d0
#define MSG_HOTKEY_ASK_REPLACE 0x5d1
#define MSG_HOTKEY_NO_KEY 0x5ee
#define MSG_HOTKEY_UNNAMED_KEY 0x6a4
#define MSG_HOTKEY_SAVED 0xdf4
#define MSG_HOTKEY_CHAT_MODE_TOGGLE 0x6ef
#define MSG_HOTKEY_PREV 0x41d
#define MSG_HOTKEY_NEXT 0x41e
static const int hotkey_tab_names[HOTKEY_GROUPS] = { 0x5d3, 0xe0b, 0x5d4, 0x5d5 };

#define HOTKEY_KEY_COLOUR_0 0xffefd6
#define HOTKEY_KEY_COLOUR_3 0xefd6ff
#define HOTKEY_KEY_COLOUR 0xdffffe
#define HOTKEY_SELECTED_COLOUR 0xe1e1ff
#define HOTKEY_LINE_COLOUR 0xc0c0c0
#define HOTKEY_PAGE_COLOUR 0x7f3cff
#define HOTKEY_PAGES_COLOUR 0x666666
#define HOTKEY_OUTLINE_COLOUR 0xffffff
#define HOTKEY_NOTE_COLOUR 0xffffff
#define HOTKEY_WARN_COLOUR 0xffff

struct UIHotKeyWnd {
    struct UIFrameWnd frame;
    int selected;
    int rows;
    int title_height;
    int bar_height;
    int tab_height;
    int margin_x;
    int margin_y;
    int row_height;
    int key_width;
    int action_width;
    struct Tree changes[HOTKEY_GROUPS];
    struct UIWindow *balloon;
    struct UIWindow *tabs;
    int group;
    struct UIWindow *chat_bar;
    int page;
    int pages;
    BYTE paged;
    struct UIBitmapButton *prev;
    struct UIBitmapButton *next;
};

#define CHAT_LINE_TAGS(line) (((struct UIChatEditCtrl *)(line))->popup)

static int hotkey_count(int group)
{
    return session_get_hot_key_list_size(&g_session1, group);
}

static int hotkey_page_first(const struct UIHotKeyWnd *self)
{
    return self->page * HOTKEY_PAGE_ROWS - HOTKEY_PAGE_ROWS;
}

static void hotkey_init(struct HotKey *key)
{
    key->code = 0;
    key->modifier = 0;
    string_init(&key->label);
    string_init(&key->action);
}

static void hotkey_destroy(struct HotKey *key)
{
    string_free(&key->action);
    string_free(&key->label);
}

static void destroy_change(void *value)
{
    hotkey_destroy(&((struct HotKeyEntry *)value)->key);
}

static struct HotKey *change_at(struct UIHotKeyWnd *self, int group, int index)
{
    struct HotKeyEntry *change = tree_get(&self->changes[group], &index, sizeof index, sizeof *change,
                                          tree_less_int);

    return &change->key;
}

static void hotkey_copy_assign(struct HotKey *self, const struct HotKey *other)
{
    self->code = other->code;
    self->modifier = other->modifier;
    string_assign(&self->label, string_cstr(&other->label), other->label.size);
    string_assign(&self->action, string_cstr(&other->action), other->action.size);
}

static void hotkey_copy(struct HotKey *self, const struct HotKey *other)
{
    self->code = other->code;
    self->modifier = other->modifier;
    string_copy(&self->label, &other->label);
    string_copy(&self->action, &other->action);
}

struct HotKey *hotkey_move_assign(struct HotKey *self, struct HotKey *other)
{
    self->code = other->code;
    self->modifier = other->modifier;
    if (&self->label != &other->label) {
        string_free(&self->label);
        self->label = other->label;
        string_init(&other->label);
    }
    if (&self->action != &other->action) {
        string_free(&self->action);
        self->action = other->action;
        string_init(&other->action);
    }
    return self;
}

static struct HotKeyEntry *hotkey_wnd_entry(struct UIHotKeyWnd *self, struct HotKeyEntry *out, int group, int n)
{
    struct HotKey key;
    int index;

    hotkey_init(&key);
    index = session_get_original_hot_key_index(&g_session1, group, n);
    if (index != -1) {
        struct HotKeyEntry *change = tree_find(&self->changes[group], &index, tree_less_int);

        if (change)
            hotkey_copy_assign(&key, &change->key);
        if (!change || key.action.size == 0) {
            struct HotKey session;

            session_get_hot_key(&g_session1, &session, group, index);
            hotkey_move_assign(&key, &session);
            hotkey_destroy(&session);
        }
    }
    out->index = index;
    hotkey_copy(&out->key, &key);
    hotkey_destroy(&key);
    return out;
}

static void hotkey_wnd_set(struct UIHotKeyWnd *self, int group, int index, const struct HotKey *key)
{
    struct HotKey *value = change_at(self, group, index);

    value->code = key->code;
    value->modifier = key->modifier;
    string_assign(&value->label, string_cstr(&key->label), key->label.size);
    string_assign(&value->action, string_cstr(&key->action), key->action.size);
}

static void hotkey_wnd_clear_changes(struct UIHotKeyWnd *self)
{
    int i;

    for (i = 0; i < HOTKEY_GROUPS; i++)
        tree_clear(&self->changes[i], destroy_change);
}

void ui_hot_key_wnd_destruct(struct UIHotKeyWnd *self)
{
    int i;

    self->frame.base.ops = &ui_hot_key_wnd_ops;
    hotkey_wnd_clear_changes(self);
    ui_window_mgr_post_quit2(&g_windowMgr, self->balloon);
    for (i = HOTKEY_GROUPS - 1; i >= 0; i--)
        tree_free(&self->changes[i], destroy_change);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_hot_key_wnd_destroy(struct UIWindow *window)
{
    struct UIHotKeyWnd *self = (struct UIHotKeyWnd *)window;
    ui_hot_key_wnd_destruct(self);
    mem_free(self);
}

static struct UIBitmapButton *hotkey_button(const char *name)
{
    char normal[MAX_PATH], hover[MAX_PATH], pressed[MAX_PATH];

    sprintf(normal, "%s%s.bmp", UI_SKIN, name);
    sprintf(hover, "%s%s_a.bmp", UI_SKIN, name);
    sprintf(pressed, "%s%s_b.bmp", UI_SKIN, name);
    return ui_bitmap_button(normal, hover, pressed);
}

static struct UIBitmapButton *hotkey_arrow(struct UIHotKeyWnd *self, const char *bitmap, int x, int tip, int id)
{
    struct UIBitmapButton *arrow = ui_bitmap_button(bitmap, bitmap, bitmap);

    ui_window_move_base(&arrow->button.base, x, 0x19);
    ui_button_show((struct UIButton *)(arrow), 0);
    ui_bitmap_button_set_tool_tip(arrow, msg_str(tip));
    arrow->button.base.id = id;
    ui_window_add_child(&self->frame.base, &arrow->button.base);
    return arrow;
}

void ui_hot_key_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIHotKeyWnd *self = (struct UIHotKeyWnd *)window;
    static const char *const names[4] = { "btn_ok", "btn_cancel", "btn_reset", "btn_close" };
    static const int ids[4] = { UI_ID_OK, UI_ID_CANCEL, HOTKEY_ID_RESET, UI_ID_CLOSE };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *balloon, *tabs, *check;
    const char *tab_names[HOTKEY_GROUPS];
    int i, x = 0;

    (void)width;
    (void)height;
    session_reset_option2(&g_session1);
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);

    balloon = mem_alloc(sizeof(struct UITransBalloonText));
    if (balloon)
        balloon = ui_trans_balloon_text_construct(balloon);
    self->balloon = balloon;
    ui_window_create(balloon, wnd->width, 0x14);
    ui_balloon_text_set_text((struct UIBalloonText *)self->balloon, msg_str(MSG_HOTKEY_ASK_KEY), HOTKEY_NOTE_CHARS);
    ui_window_set_show(self->balloon, 0);
    ui_window_mgr_add_window(&g_windowMgr, self->balloon);

    tabs = mem_alloc(sizeof(struct UITabControl));
    if (tabs)
        tabs = ui_tab_control_construct(tabs);
    self->tabs = tabs;
    ui_tab_control_setup(tabs, 1, self->tab_height - 3, 0x14, 1, 1, 0, 0, 0, 0, 0, 0, 0);
    ui_window_move(self->tabs, self->margin_x, self->margin_y + self->title_height);
    ui_window_add_child(wnd, self->tabs);
    for (i = 0; i < HOTKEY_GROUPS; i++)
        tab_names[i] = msg_str(hotkey_tab_names[i]);
    for (i = 0; i < HOTKEY_GROUPS; i++)
        ui_tab_control_add_tab(self->tabs, tab_names[i]);

    for (i = 0; i < 4; i++) {
        struct UIBitmapButton *button = hotkey_button(names[i]);

        button->button.base.id = ids[i];
        ui_window_add_child(wnd, &button->button.base);
        if (ids[i] == UI_ID_OK)
            x = wnd->width - 0x8b;
        else if (ids[i] == UI_ID_CANCEL)
            x = wnd->width - 0x5c;
        else if (ids[i] == HOTKEY_ID_RESET)
            x = 5;
        else if (ids[i] == UI_ID_CLOSE)
            x = wnd->width - 0x2e;
        ui_window_move_base(&button->button.base, x, wnd->height - 0x19);
    }

    check = mem_alloc(sizeof(struct UICheckBox));
    if (check)
        check = (struct UIWindow *)ui_check_box_construct(check);
    self->chat_bar = check;
    ui_window_create(check, 0x96, 0xc);
    ui_check_box_set_bitmaps(self->chat_bar, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp",
                          msg_str(MSG_HOTKEY_CHAT_MODE_TOGGLE));
    ui_window_move_base(self->chat_bar, 0x50, wnd->height - 0x16);
    ui_check_box_set_check(self->chat_bar, g_windowMgr.chat_bar_toggleable);
    ui_window_add_child(wnd, self->chat_bar);

    self->prev = hotkey_arrow(self, UI_SKIN "basic_interface\\arw_left.bmp", wnd->width - 0x50, MSG_HOTKEY_PREV,
                              HOTKEY_ID_PREV);
    self->next = hotkey_arrow(self, UI_SKIN "basic_interface\\arw_right.bmp", wnd->width - 0x14,
                              MSG_HOTKEY_NEXT, HOTKEY_ID_NEXT);
}

static int hotkey_text_width(struct UIHotKeyWnd *self, const char *text)
{
    return ui_window_text_width(&self->frame.base, text, 0, 0, HOTKEY_FONT_HEIGHT, 0, 0);
}

void ui_hot_key_wnd_on_draw(struct UIWindow *window)
{
    struct UIHotKeyWnd *self = (struct UIHotKeyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int i, count, row = 0, top, frame_width, frame_height;

    if (wnd->height == self->title_height)
        return;
    self->title_height = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_HOTKEY_TITLE), 0);
    self->bar_height = ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_draw_body(wnd, self->title_height, 0, "");

    count = hotkey_count(self->group);
    for (i = 0; i < count; i++) {
        struct HotKeyEntry entry;
        struct String label, action;
        struct String fitted = {0};
        int key_code, label_x, column, y, left, right, key_x, text_width;
        COLORREF colour;

        if (self->paged && i < hotkey_page_first(self))
            continue;
        if (i >= self->page * HOTKEY_PAGE_ROWS)
            continue;
        hotkey_wnd_entry(self, &entry, self->group, i);
        key_code = entry.key.code;
        label = entry.key.label;
        action = entry.key.action;
        string_init(&entry.key.label);
        string_init(&entry.key.action);
        hotkey_destroy(&entry.key);

        if (action.size != 0) {
            if (label.size == 0) {
                const char *none = msg_str(key_code == 0 ? MSG_HOTKEY_NO_KEY : MSG_HOTKEY_UNNAMED_KEY);

                string_assign_cstr(&label, none);
            }
            label_x = (self->key_width - hotkey_text_width(self, string_cstr(&label))) / 2;
            column = row / self->rows;
            y = row % self->rows * self->row_height + self->margin_y + self->tab_height + self->title_height;
            if (column != 0) {
                left = wnd->width / 2;
                right = wnd->width - self->margin_x;
            } else {
                left = self->margin_x;
                right = wnd->width / 2;
            }
            key_x = right - self->key_width;
            if (self->group == 0)
                colour = HOTKEY_KEY_COLOUR_0;
            else if (self->group == 3)
                colour = HOTKEY_KEY_COLOUR_3;
            else
                colour = HOTKEY_KEY_COLOUR;
            ui_window_fill_rect(wnd, key_x, y, self->key_width, self->row_height - 1, colour);
            if (row == self->selected)
                ui_window_fill_rect(wnd, left + 1, y, wnd->width / 2 - self->margin_x - 1, 0x13,
                                  HOTKEY_SELECTED_COLOUR);
            ui_window_fill_rect(wnd, left, y + self->row_height - 1, self->action_width + self->key_width, 1,
                              HOTKEY_LINE_COLOUR);
            ui_window_fill_rect(wnd, key_x, y, 1, self->row_height, HOTKEY_LINE_COLOUR);

            ui_window_fit_text(wnd, &fitted, HOTKEY_TEXT_WIDTH, string_cstr(&action), 0,
                             HOTKEY_FONT_HEIGHT, 0);
            text_width = hotkey_text_width(self, string_cstr(&fitted));
            ui_window_text_out(wnd, (wnd->width / 2 - self->margin_x - self->key_width - text_width) / 2 + left,
                             y + 5, string_cstr(&fitted), 0, 0, HOTKEY_FONT_HEIGHT, 0, 0, 0);
            ui_window_text_out(wnd, key_x + label_x, y + 5, string_cstr(&label), 0, 0,
                             HOTKEY_FONT_HEIGHT, 0, 0, 0);
            row++;
            string_free(&fitted);
        }
        string_free(&action);
        string_free(&label);
    }

    if (self->paged) {
        struct String text, part;
        int arrow_x = self->prev->button.base.x, x;

        string_init(&text);
        string_format(&text, "%d / %d", self->page, self->pages);
        x = (self->prev->button.base.width - hotkey_text_width(self, string_cstr(&text)) + 0x3c) / 2 + arrow_x;
        string_init(&part);
        string_format(&part, "%d", self->page);
        string_assign(&text, string_cstr(&part), part.size);
        string_free(&part);
        ui_window_text_out_outlined(wnd, x, 0x19, string_cstr(&text), 0, HOTKEY_PAGE_COLOUR,
                                 HOTKEY_OUTLINE_COLOUR, 0, 0xb, 1, 0);
        x += hotkey_text_width(self, string_cstr(&text));
        string_format(&part, " / %d", self->pages);
        string_assign(&text, string_cstr(&part), part.size);
        string_free(&part);
        ui_window_text_out_outlined(wnd, x, 0x19, string_cstr(&text), 0, HOTKEY_PAGES_COLOUR,
                                 HOTKEY_OUTLINE_COLOUR, 0, 0xb, 1, 0);
        string_free(&text);
    }

    top = self->margin_y + self->tab_height + self->title_height;
    frame_width = wnd->width - 2 * self->margin_x;
    frame_height = wnd->height - 2 * self->margin_y - self->bar_height - self->tab_height - self->title_height;
    ui_window_draw_outline(wnd, self->margin_x, top, frame_width, frame_height, HOTKEY_LINE_COLOUR);
    ui_window_fill_rect(wnd, wnd->width / 2, top, 1, frame_height, HOTKEY_LINE_COLOUR);
    ui_window_fill_rect(wnd, self->margin_x, top - 3, 1, 3, HOTKEY_LINE_COLOUR);
    ui_window_fill_rect(wnd, wnd->width - self->margin_x - 1, top - 3, 1, 3, HOTKEY_LINE_COLOUR);
    ui_window_fill_rect(wnd, self->margin_x + self->tabs->width, top - 3, frame_width - self->tabs->width, 1,
                      HOTKEY_LINE_COLOUR);
}

static int hotkey_wnd_hit(struct UIHotKeyWnd *self, int x, int y)
{
    struct UIWindow *wnd = &self->frame.base;
    int row;

    if (x < self->margin_x || x > wnd->width - self->margin_x)
        return -1;
    if (y < self->tab_height + self->title_height + self->margin_y
        || y > wnd->height - self->bar_height - self->margin_y)
        return -1;
    row = (y - self->title_height - self->tab_height - self->margin_y) / self->row_height;
    if (x > wnd->width / 2)
        row += self->rows;
    if (row >= hotkey_count(self->group))
        return -1;
    return row;
}

static void hotkey_wnd_place_balloon(struct UIHotKeyWnd *self, int screen_x, int screen_y)
{
    struct UIWindow *wnd = &self->frame.base;
    int y = (self->selected % self->rows - 1) * self->row_height + self->margin_y + self->tab_height
            + screen_y + self->title_height;

    ui_window_move(self->balloon, (wnd->width - self->balloon->width) / 2 + screen_x, y);
}

void ui_hot_key_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIHotKeyWnd *self = (struct UIHotKeyWnd *)window;
    int old;

    if (y < self->title_height)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    old = self->selected;
    self->selected = hotkey_wnd_hit(self, x, y);
    if (self->selected + hotkey_page_first(self) < hotkey_count(self->group)) {
        if (self->selected > -1) {
            int sx = 0, sy = 0;

            ui_window_to_screen(&self->frame.base, &sx, &sy);
            ui_balloon_text_set_text((struct UIBalloonText *)self->balloon, msg_str(MSG_HOTKEY_ASK_KEY), HOTKEY_NOTE_CHARS);
            ((struct UIBalloonText *)self->balloon)->colour = HOTKEY_NOTE_COLOUR;
            ((struct UIBalloonText *)self->balloon)->outline = 0;
            hotkey_wnd_place_balloon(self, sx, sy);
            ui_window_set_show(self->balloon, 1);
            ui_window_mgr_raise_window(&g_windowMgr, self->balloon);
        } else {
            ui_window_set_show(self->balloon, 0);
        }
        if (old == self->selected)
            return;
    }
    ui_window_invalidate_base(&self->frame.base);
}

void ui_hot_key_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIHotKeyWnd *self = (struct UIHotKeyWnd *)window;
    struct HotKeyEntry entry;
    int row = hotkey_wnd_hit(self, x, y), tip_x, tip_y, sx = 0, sy = 0;

    if (row <= -1)
        return;
    tip_y = (row % self->rows - 1) * self->row_height + self->margin_y + self->tab_height + self->title_height;
    tip_x = self->frame.base.width / 2 * (row / self->rows) + 0x14;
    ui_window_to_screen(&self->frame.base, &sx, &sy);
    hotkey_wnd_entry(self, &entry, self->group, row + hotkey_page_first(self));
    if (entry.key.action.size > 0)
        mode_show_tooltip(client_current_mode(), string_cstr(&entry.key.action), sx + tip_x, sy + tip_y,
                          0xffffffff, 0);
    hotkey_destroy(&entry.key);
}

void ui_hot_key_wnd_on_r_btn_down(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

static int hotkey_wnd_take_key(struct UIHotKeyWnd *self)
{
    char text[100];
    int group, n, count;

    memset(text, 0, sizeof text);
    for (group = 0; group < HOTKEY_GROUPS; group++) {
        if (self->group == 0 ? group == 3 : self->group == 3 && group == 0)
            continue;
        count = hotkey_count(group);
        for (n = 0; n < count; n++) {
            struct HotKeyEntry entry;
            struct HotKey key;
            int stays;

            hotkey_wnd_entry(self, &entry, group, n);
            hotkey_copy(&key, &entry.key);
            if (key.code != g_windowMgr.last_key || key.modifier != g_windowMgr.key_modifier) {
                hotkey_destroy(&key);
                hotkey_destroy(&entry.key);
                continue;
            }
            stays = 1;
            if (n != self->selected || group != self->group) {
                snprintf(text, sizeof text, msg_str(MSG_HOTKEY_ASK_REPLACE),
                            string_cstr(&key.action));
                if (ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_QUESTION, 1, 0, 0, UI_MSGBOX_TITLE,
                                           0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES) {
                    key.code = 0;
                    key.modifier = 0;
                    string_clear(&key.label);
                    hotkey_wnd_set(self, group, entry.index, &key);
                    if (group == 0 && self->group != 0) {
                        int i, count3 = hotkey_count(3);

                        for (i = 0; i < count3; i++) {
                            struct HotKeyEntry other;
                            struct HotKey cleared;

                            hotkey_wnd_entry(self, &other, 3, i);
                            hotkey_copy(&cleared, &other.key);
                            if (cleared.code == g_windowMgr.last_key && cleared.modifier == g_windowMgr.key_modifier) {
                                cleared.code = 0;
                                cleared.modifier = 0;
                                string_clear(&cleared.label);
                                hotkey_wnd_set(self, 3, other.index, &cleared);
                            }
                            hotkey_destroy(&cleared);
                            hotkey_destroy(&other.key);
                        }
                    }
                    stays = 0;
                }
            }
            hotkey_destroy(&key);
            hotkey_destroy(&entry.key);
            return stays;
        }
    }
    return 0;
}

static void hotkey_wnd_note(struct UIHotKeyWnd *self, int sx, int sy)
{
    hotkey_wnd_place_balloon(self, sx, sy);
    ui_window_invalidate_base(&self->frame.base);
}

BYTE ui_hot_key_wnd_on_key(struct UIHotKeyWnd *self, int state)
{
    struct HotKeyEntry entry;
    struct HotKey key;
    int sx = 0, sy = 0, result;

    ui_window_to_screen(&self->frame.base, &sx, &sy);
    ui_balloon_text_set_text((struct UIBalloonText *)self->balloon, msg_str(MSG_HOTKEY_ASK_KEY), HOTKEY_NOTE_CHARS);
    ((struct UIBalloonText *)self->balloon)->colour = HOTKEY_NOTE_COLOUR;
    ((struct UIBalloonText *)self->balloon)->outline = 0;
    hotkey_wnd_note(self, sx, sy);
    if (self->selected < 0)
        return 0;

    result = state;
    if (self->group == 1
        && session_get_original_hot_key_index(&g_session1, 1, self->selected) == HOTKEY_INDEX_CHAT) {
        int code = g_windowMgr.last_key, mod = g_windowMgr.key_modifier;

        if (code == KEY_ALT || mod == KEY_ALT) {
            result = 3;
        } else if (code == KEY_CONTROL || mod == KEY_CONTROL || code == KEY_SHIFT || mod == KEY_SHIFT) {
            result = 3;
        } else if (code == KEY_SPACE || code == KEY_NUMLOCK) {
            result = mod != 0 ? hotkey_wnd_take_key(self) : 2;
        } else if (state == 4) {
            if (mod == 0 && !ui_window_mgr_is_reserved_key(&g_windowMgr, code))
                result = 3;
            else
                result = hotkey_wnd_take_key(self);
        }
    } else if (state == 4) {
        result = hotkey_wnd_take_key(self);
    }
    if (result == 1)
        return 0;
    if (result == 2 || result == 3) {
        ui_balloon_text_set_text((struct UIBalloonText *)self->balloon,
                              msg_str(result == 2 ? MSG_HOTKEY_KEY_REFUSED : MSG_HOTKEY_KEY_INVALID),
                              HOTKEY_NOTE_CHARS);
        ((struct UIBalloonText *)self->balloon)->colour = HOTKEY_WARN_COLOUR;
        ((struct UIBalloonText *)self->balloon)->outline = 0;
        hotkey_wnd_note(self, sx, sy);
        return 0;
    }

    hotkey_wnd_entry(self, &entry, self->group, self->selected + hotkey_page_first(self));
    hotkey_copy(&key, &entry.key);
    if (g_windowMgr.last_key == KEY_ESCAPE) {
        key.code = 0;
        key.modifier = 0;
        string_clear(&key.label);
    } else {
        const char *text = NULL;

        key.code = g_windowMgr.last_key;
        key.modifier = g_windowMgr.key_modifier;

        if (client_lua_call(g_session1.luaState, "GetKeyDes", "dd>s", g_windowMgr.last_key, g_windowMgr.key_modifier, &text) && text)
            string_assign_cstr(&key.label, text);
    }
    hotkey_wnd_set(self, self->group, entry.index, &key);
    ui_window_invalidate_base(&self->frame.base);
    hotkey_destroy(&key);
    hotkey_destroy(&entry.key);
    return 1;
}

static void hotkey_wnd_show_arrows(struct UIHotKeyWnd *self)
{
    ui_button_show((struct UIButton *)(self->prev), self->page > 1 ? 1 : 0);
    ui_button_show((struct UIButton *)(self->next), self->page < self->pages ? 1 : 0);
    ui_window_invalidate_base(&self->frame.base);
}

static void hotkey_wnd_apply(struct UIHotKeyWnd *self)
{
    char text[500];
    int group;

    for (group = 0; group < HOTKEY_GROUPS; group++) {
        struct TreeNode *node;

        for (node = tree_first(&self->changes[group]); node != self->changes[group].head; node = tree_next(node)) {
            struct HotKeyEntry *change = TREE_VALUE(node, struct HotKeyEntry);

            user_hotkey_mgr_change(g_userHotkeyMgr, group, change->index, change->key.code,
                                   change->key.modifier, string_cstr(&change->key.action));
        }
    }
    memset(text, 0, sizeof text);
    snprintf(text, sizeof text, "%s", account_user_data_save(g_accountUserDataMgr)
        ? msg_str(MSG_HOTKEY_SAVED) : "Could not save local account settings.");
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)text, HOTKEY_WARN_COLOUR, 1, 0);
}

static void hotkey_wnd_reset(struct UIHotKeyWnd *self)
{
    int tab;

    self->selected = -1;
    hotkey_wnd_clear_changes(self);
    for (tab = 0; tab < HOTKEY_GROUPS; tab++) {
        int group = user_hotkey_tab_group(g_userHotkeyMgr, tab);
        int i, count = hotkey_count(group);

        for (i = 0; i < count; i++) {
            int index = session_get_original_hot_key_index(&g_session1, group, i);
            int code = 0, mod = 0;
            const char *label = NULL, *action = NULL;
            struct String label_text, action_text;

            if (index == -1)
                continue;
            string_init(&label_text);
            string_init(&action_text);
            if (client_lua_call(g_session1.luaState, "GetUserHotKeyInfo", "dd>ddss", group + 1, index, &code, &mod,
                        &label, &action)) {
                string_assign_cstr(&action_text, action);
                if (action_text.size != 0
                    && client_lua_call(g_session1.luaState, "GetOriginalHotKeyInfo", "dd>ddss", group + 1, index, &code,
                               &mod, &label, &action)) {
                    string_assign_cstr(&label_text, label);
                    string_assign_cstr(&action_text, action);
                    if (action_text.size != 0) {
                        struct HotKey *value = change_at(self, group, index);

                        value->code = code;
                        value->modifier = mod;
                        string_assign(&value->label, string_cstr(&label_text), label_text.size);
                        string_assign(&value->action, string_cstr(&action_text), action_text.size);
                    }
                }
            }
            string_free(&action_text);
            string_free(&label_text);
        }
    }
    ui_window_invalidate_base(&self->frame.base);
}

intptr_t ui_hot_key_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                   intptr_t val3, intptr_t val4)
{
    struct UIHotKeyWnd *self = (struct UIHotKeyWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case UI_ID_OK:
            hotkey_wnd_apply(self);
            ui_close_window(WND_HOTKEY);
            break;
        case UI_ID_CLOSE:
            ui_close_window(WND_HOTKEY);
            break;
        case UI_ID_CANCEL:
            self->selected = -1;
            hotkey_wnd_clear_changes(self);
            ui_window_invalidate_base(&self->frame.base);
            break;
        case UI_ID_CHECKBOX: {
            struct UIWindow *chat;

            if (sender != self->chat_bar)
                break;
            chat = g_windowMgr.new_chat_wnd;
            g_windowMgr.chat_bar_toggleable = val2 != 0;
            if (!chat)
                break;
            if (val2) {
                ui_window_invalidate(chat);
                break;
            }
            g_windowMgr.chat_bar_shown = 1;
            ui_new_chat_wnd_place_bar((struct UINewChatWnd *)chat);
            ui_window_mgr_set_focus_edit(&g_windowMgr, ui_chat_window_focus_edit(chat));
            ui_edit_ctrl_set_text_default((struct UIEditCtrl *)(ui_chat_window_focus_edit(chat)), "");
            if (!g_windowMgr.chat_bar_toggleable)
                ui_new_chat_wnd_set_size_step((struct UINewChatWnd *)chat, chat->state);
            ui_window_mgr_set_active_top_window(&g_windowMgr, ui_chat_window_focus_edit(chat));
            if (CHAT_LINE_TAGS(ui_chat_window_edit(chat)))
                ui_item_tag_ctrl_clear_tags(CHAT_LINE_TAGS(ui_chat_window_edit(chat)));
            break;
        }
        case HOTKEY_ID_PREV:
            if (self->page > 1)
                self->page--;
            hotkey_wnd_show_arrows(self);
            break;
        case HOTKEY_ID_NEXT:
            if (self->page < self->pages)
                self->page++;
            hotkey_wnd_show_arrows(self);
            break;
        case HOTKEY_ID_RESET:
            hotkey_wnd_reset(self);
            break;
        }
        return 0;
    }
    if (msg == UI_MSG_TAB_CHANGED) {
        int group, count;

        if (ui_window_find(WND_TAB_RELAY) == sender) {
            ui_tab_control_select_tab(self->tabs, val1);
            return 0;
        }
        group = user_hotkey_tab_group(g_userHotkeyMgr, val1);
        if (self->group != group || group != 1) {
            ui_button_show((struct UIButton *)(self->prev), 0);
            ui_button_show((struct UIButton *)(self->next), 0);
        }
        if (self->group == group)
            return 0;
        self->selected = -1;
        self->group = group;
        self->paged = 0;
        self->page = 1;
        count = hotkey_count(group);
        if (count > HOTKEY_PAGE_ROWS) {
            self->paged = 1;
            self->pages = count / HOTKEY_PAGE_ROWS;
            if (count % HOTKEY_PAGE_ROWS != 0)
                self->pages++;
            ui_button_show((struct UIButton *)(self->next), 1);
        }
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIHotKeyWnd;
struct UIWindow;
void ui_hot_key_wnd_destroy(struct UIWindow *window);
void ui_hot_key_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_hot_key_wnd_on_draw(struct UIWindow *self);
void ui_hot_key_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_hot_key_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_hot_key_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_hot_key_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_hot_key_wnd_ops = {
    .destroy = ui_hot_key_wnd_destroy,
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
    .on_create = ui_hot_key_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_hot_key_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_hot_key_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_hot_key_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_hot_key_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_hot_key_wnd_send_msg,
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

struct UIHotKeyWnd *ui_hot_key_wnd_construct(struct UIHotKeyWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_hot_key_wnd_ops;
    self->selected = -1;
    self->rows = 0x12;
    self->title_height = 0x11;
    self->bar_height = 0;
    self->tab_height = 0x14;
    self->margin_x = 3;
    self->margin_y = 3;
    self->row_height = 0x14;
    self->key_width = 0x50;
    self->action_width = 0x87;
    for (i = 0; i < HOTKEY_GROUPS; i++)
        tree_init(&self->changes[i]);
    self->balloon = NULL;
    self->group = 0;
    self->page = 1;
    self->pages = 0;
    self->paged = 0;
    return self;
}

struct UIWindow *ui_hot_key_wnd_new(void)
{
    return (struct UIWindow *)ui_hot_key_wnd_construct(mem_alloc(sizeof(struct UIHotKeyWnd)));
}
