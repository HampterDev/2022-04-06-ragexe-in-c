#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "core/text_array.h"
#include "platform/code_page.h"
#include "core/text_format.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include <string.h>
#include <stdio.h>
#include "ui/chat/chat_window.h"
#include "ui/chat/markup_links.h"
#include "ui/chat/sub_chat.h"
#include "core/msg_str.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "ui/wndmgr/wndmgr.h"

#define CHAT_MSG_STEP_KEY 2
#define CHAT_MSG_PAGE_UP 3
#define CHAT_MSG_PAGE_DOWN 4
#define CHAT_MSG_ADD_LINE 0x25
#define CHAT_MSG_FOCUS_LINE 0x36
#define CHAT_MSG_FORGET_WHISPER 0x46
#define CHAT_MSG_LOG_LINE 0x73
#define CHAT_MSG_RESOLVE_PENDING_NAME 0x9a
#define CHAT_MSG_SET_STEP 0xf
#define CHAT_MSG_SHOW_IF_HIDDEN 0x10
#define CHAT_MSG_PLACE 0x22

#define CHAT_CMD_SEND 0xb0
#define CHAT_CMD_SET_WHISPER_NAME 0x153
#define CHAT_CMD_SHOW_LINES 0x15d

#define CHAT_COMBO_WHISPER_NAMES 0
#define CHAT_COMBO_TARGET 1
#define CHAT_COMBO_ROW_HEIGHT 0x10
#define CHAT_COMBO_MAX_HEIGHT 0x50
#define CHAT_COMBO_WIDTH 0x70
#define CHAT_TARGET_ALL 0
#define CHAT_TARGET_PARTY 1
#define CHAT_TARGET_GUILD 2

#define CHAT_KIND_ALL 0x16
#define CHAT_KIND_PLAIN 0
#define CHAT_COLOUR_RED 0xff
#define CHAT_NAME_MAX 0x18
#define CHAT_WRAP_MARGIN 0x14
#define CHAT_SPLIT_WIDTH 0x5e
#define CHAT_TAB_ROOM 0x5a

#define MSG_CHAT_TAB_TOO_LONG 0x6f0
#define MSG_CHAT_NO_WHISPER_NAMES 0xc0
#define MSG_SEND_TO_ALL 0x55
#define MSG_SEND_TO_PARTY 0x56
#define MSG_SEND_TO_GUILD 0x1b5
#define MSG_CHAT_ITEM_LINK 0xbe2
#define MSG_CHAT_NAME_LINK 0xafc
#define MSG_CHAT_TOO_MANY_TABS 0x62a
#define CHAT_NOTICE_COLOUR 0x64ffff

#define MODE_REQ_WHISPER 0xb
#define MODE_REQ_TALK 0x2a
#define TALK_NAME_EX 0x12
#define TALK_NAME_MAKE_PARTY 0x1a
#define TALK_NAME_MAKE_GUILD 0x37

#define CHAT_LINE_SIZE 0x16a

static void renumber_sub_chat_wnds(void)
{
    struct Tree copy;
    struct TreeNode *node;
    int key = 0;

    tree_copy(&copy, &g_windowMgr.sub_chat_wnds, sizeof(struct SubChatWnd), NULL);
    tree_clear(&g_windowMgr.sub_chat_wnds, NULL);
    for (node = tree_first(&copy); node != copy.head; node = tree_next(node), key++) {
        struct UIWindow *wnd = TREE_VALUE(node, struct SubChatWnd)->wnd;
        struct SubChatWnd *sub;

        ((struct UISubChatHisWnd *)wnd)->key = key;
        sub = tree_get(&g_windowMgr.sub_chat_wnds, &key, sizeof key, sizeof *sub, tree_less_int);
        sub->wnd = wnd;
    }
    tree_free(&copy, NULL);
}

static void clear_tab_strip(struct UINewChatWnd *self, int extra)
{
    RECT rect;

    rect.left = 3;
    rect.top = 0;
    rect.right = extra + self->tabs->width + 3;
    rect.bottom = self->tabs->height;
    ui_window_fill_canvas(&self->frame.base, CHAT_COLOUR_KEY, &rect);
}

static void erase_box(struct UINewChatWnd *self, struct UIWindow **where)
{
    memmove(where, where + 1, (BYTE *)self->boxes.last - (BYTE *)(where + 1));
    self->boxes.last = (struct UIWindow **)self->boxes.last - 1;
}

static void add_box(struct UINewChatWnd *self, int height)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIChatHisBox *box = mem_alloc(sizeof(struct UIChatHisBox));
    struct UIWindow *value;

    if (box)
        box = ui_chat_his_box_construct(box);
    ui_list_box_create_window(&box->list, 0, 0, wnd->width - 8, height, 0);
    ui_window_move_base((struct UIWindow *)box, OFF_SCREEN - wnd->width, OFF_SCREEN);
    ui_window_add_child(wnd, (struct UIWindow *)box);
    value = (struct UIWindow *)box;
    vec_push(&self->boxes, &value, sizeof value);
}

static void spread_tabs(struct UINewChatWnd *self)
{
    self->tab_width = (unsigned)self->tabs->width / g_session1.chatWndTabs.size;
}

static void tell_battle_options(struct UINewChatWnd *self, struct UIWindow *options, int how)
{
    ui_window_send_msg(options, &self->frame.base, UI_MSG_COMMAND, BATTLE_OPTION_FOR_TAB, self->tab, 1, how);
}

static void tear_out_tab(struct UINewChatWnd *self, struct UIWindow *sender, int dragged_tab)
{
    struct ChatTab *node;
    struct UIWindow *sub, **where;
    int key, tab_key;

    if (!CHAT_TABS_EDITABLE || g_session1.chatWndTabs.size <= 1)
        return;
    node = chat_tabs_find(&g_session1.chatWndTabs, self->tab);
    if (!node)
        return;
    key = g_session1.subChatWnds.size;
    if (!chat_tabs_set(&g_session1.subChatWnds, key, &node->value))
        return;
    tab_key = node->key;
    chat_tabs_remove(&g_session1.chatWndTabs, node->key);
    if (tab_key != (int)g_session1.chatWndTabs.size)
        chat_tabs_renumber(&g_session1.chatWndTabs);
    ui_tab_control_remove_tab(self->tabs, self->tab);
    spread_tabs(self);
    sub = ui_window_mgr_open_sub_chat(&g_windowMgr, g_windowMgr.sub_chat_wnds.size, -1, -1, 0, 0);
    if (!sub)
        return;
    where = &CHAT_BOX(self, self->tab);
    if (where != (struct UIWindow **)self->boxes.last) {
        ui_chat_his_box_copy_log(((struct UISubChatHisWnd *)sub)->box, (struct UIChatHisBox *)*where);
        ui_window_remove_child(&self->frame.base, *where, 1);
        erase_box(self, where);
    }
    clear_tab_strip(self, self->tab_width);
    if (self->tab > 0) {
        self->tab--;
        CHAT_TABS(self)->selected = self->tab;
    }
    ui_new_chat_wnd_show_tab(self, 1);
    if (sender != &self->frame.base) {
        int x = 0, y = 0;

        ui_window_to_screen(&self->frame.base, &x, &y);
        ui_window_mgr_release_capture(&g_windowMgr);
        ui_window_move(sub, x, y);
        ui_window_on_l_btn_down(sub, g_mouse.x - self->tab_width * dragged_tab - x, g_mouse.y - y);
    }
    ui_new_chat_wnd_place_title_buttons(self);
}

static void stick_tab(struct UINewChatWnd *self, struct UIWindow *sender)
{
    struct UIWindow *wnd = &self->frame.base;
    struct ChatTab *node;
    int key, tab_key, count, height;
    struct UIChatHisBox *box;

    if (!CHAT_TABS_EDITABLE)
        return;
    if (self->tabs->width + self->tab_width > wnd->width - CHAT_TAB_ROOM)
        return;
    key = ((struct UISubChatHisWnd *)sender)->key;
    node = chat_tabs_find(&g_session1.subChatWnds, key);
    if (!node)
        return;
    count = g_session1.chatWndTabs.size;
    if (ui_window_text_width(wnd, owned_text_cstr(&node->value.name), 0, 0, 0xc, 0, 0) * (count + 1) >
        wnd->width - CHAT_TAB_ROOM) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_chat_line(msg_str(MSG_CHAT_TAB_TOO_LONG), CHAT_COLOUR_RED);
        ui_window_mgr_raise_window(&g_windowMgr, sender);
        return;
    }
    if (!chat_tabs_set(&g_session1.chatWndTabs, count, &node->value))
        return;
    ui_tab_control_add_tab(self->tabs, owned_text_cstr(&node->value.name));
    spread_tabs(self);
    tab_key = node->key;
    chat_tabs_remove(&g_session1.subChatWnds, node->key);
    if (tab_key != (int)g_session1.subChatWnds.size)
        chat_tabs_renumber(&g_session1.subChatWnds);

    height = wnd->height - self->tab_height - self->bar_height - 4;
    if (height < 0)
        height = self->bar_height;
    box = mem_alloc(sizeof(struct UIChatHisBox));
    if (box)
        box = ui_chat_his_box_construct(box);
    ui_list_box_create_window(&box->list, 0, 0, wnd->width - 8, height, 0);
    ui_window_move_base((struct UIWindow *)box, OFF_SCREEN - wnd->width, OFF_SCREEN);
    ui_window_add_child(wnd, (struct UIWindow *)box);
    {
        struct SubChatWnd *sub = tree_get(&g_windowMgr.sub_chat_wnds, &key, sizeof key, sizeof *sub, tree_less_int);

        ui_chat_his_box_copy_log(box, ((struct UISubChatHisWnd *)sub->wnd)->box);
    }
    {
        struct UIWindow *value = (struct UIWindow *)box;

        vec_push(&self->boxes, &value, sizeof value);
    }
    ui_window_mgr_close_sub_chat(&g_windowMgr, key);
    if (key != (int)g_windowMgr.sub_chat_wnds.size)
        renumber_sub_chat_wnds();

    clear_tab_strip(self, 0);
    self->tab = count;
    CHAT_TABS(self)->selected = count;
    ui_new_chat_wnd_show_tab(self, 1);
    ui_new_chat_wnd_place_title_buttons(self);
}

static void tab_renamed(struct UINewChatWnd *self, struct UIWindow *sender, const char *name, int tab,
                        int old_width)
{
    struct UIWindow *wnd = &self->frame.base;
    RECT rect;

    if (wnd->height <= self->bar_height + self->tab_height)
        return;
    if (sender != self->tabs) {
        rect.left = 5;
        rect.top = self->tab_height + 2;
        rect.right = wnd->width - 6;
        rect.bottom = wnd->height - self->bar_height - 2;
        ui_window_fill_canvas(wnd, CHAT_COLOUR_KEY, &rect);
        return;
    }
    if (name) {
        struct UIWindow *options;

        struct ChatTabSettings *value = chat_tabs_get_or_add(&g_session1.chatWndTabs, tab);

        if (!value || !owned_text_assign(&value->name, text_view_cstr(name)))
            return;
        spread_tabs(self);
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->focus_edit);
        options = ui_window_find(WND_ID_BATTLE_MSG_OPTION);
        if (options)
            tell_battle_options(self, options, 2);
    }
    rect.left = 3;
    rect.top = 0;
    rect.right = old_width ? old_width + 3 : self->tabs->width + 3;
    rect.bottom = self->tabs->height;
    ui_window_fill_canvas(wnd, CHAT_COLOUR_KEY, &rect);
    ui_new_chat_wnd_place_title_buttons(self);
    ui_window_invalidate_base(wnd);
}

static void new_tab(struct UINewChatWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct ChatTabSettings tab = {0};
    char name[32];
    int count = g_session1.chatWndTabs.size, sub_count = g_session1.subChatWnds.size;

    if (count + sub_count == self->max_tabs) {
        ui_message_box(msg_str(MSG_CHAT_TOO_MANY_TABS));
        return;
    }
    snprintf(name, sizeof name, "NewTab_%d", count + sub_count + 1);
    if (!owned_text_assign(&tab.name, text_view_cstr(name)))
        return;
    if ((count + 1) * self->tab_width > wnd->width - CHAT_TAB_ROOM) {
        if (!chat_tabs_set(&g_session1.subChatWnds, sub_count, &tab)) {
            chat_tab_settings_clear(&tab);
            return;
        }
        ui_window_mgr_open_sub_chat(&g_windowMgr, g_windowMgr.sub_chat_wnds.size, -1, -1, 0, 0);
    } else {
        if (!chat_tabs_set(&g_session1.chatWndTabs, count, &tab)) {
            chat_tab_settings_clear(&tab);
            return;
        }
        ui_tab_control_add_tab(self->tabs, owned_text_cstr(&tab.name));
        spread_tabs(self);
        add_box(self, wnd->height - self->bar_height - self->tab_height - 4);
        clear_tab_strip(self, 0);
        self->tab = count;
        CHAT_TABS(self)->selected = count;
        ui_new_chat_wnd_show_tab(self, 1);
        ui_new_chat_wnd_place_title_buttons(self);
    }
    chat_tab_settings_clear(&tab);
}

static void close_tab(struct UINewChatWnd *self)
{
    struct ChatTab *node;
    struct UIWindow **where, *options;
    int tab_key;

    if (g_session1.chatWndTabs.size <= 1)
        return;
    node = chat_tabs_find(&g_session1.chatWndTabs, self->tab);
    if (!node)
        return;
    tab_key = node->key;
    chat_tabs_remove(&g_session1.chatWndTabs, node->key);
    if (tab_key != (int)g_session1.chatWndTabs.size)
        chat_tabs_renumber(&g_session1.chatWndTabs);
    where = &CHAT_BOX(self, self->tab);
    if (where != (struct UIWindow **)self->boxes.last) {
        ui_window_remove_child(&self->frame.base, *where, 1);
        erase_box(self, where);
    }
    ui_tab_control_remove_tab(self->tabs, self->tab);
    spread_tabs(self);
    clear_tab_strip(self, self->tab_width);
    if (self->tab > 0) {
        self->tab--;
        CHAT_TABS(self)->selected = self->tab;
    }
    ui_new_chat_wnd_show_tab(self, 1);
    ui_new_chat_wnd_place_title_buttons(self);
    options = ui_window_find(WND_ID_BATTLE_MSG_OPTION);
    if (options)
        tell_battle_options(self, options, 1);
}

static void add_line(struct UINewChatWnd *self, const char *text, COLORREF colour, int kind, const char *speaker)
{
    char name[CHAT_NAME_MAX + 1];
    struct TextArray lines = {0};
    struct String plain;
    struct ChatTab *node;
    BYTE has_link = 0, every_tab;
    int i, cursor_key;

    memset(name, 0, sizeof name);
    if (speaker)
        strcpy(name, speaker);
    if (kind == 1 || kind == 3 || kind == 4) {
        const char *at = strstr(text, " :");

        if (at) {
            int len = at - text;

            if (len > 0 && len < (int)strlen(text) && len < CHAT_NAME_MAX) {
                memcpy(name, text, len);
                name[len] = 0;
            }
        }
    }
    if (!strcmp(name, session_character_name(&g_session1)))
        name[0] = 0;

    string_init(&plain);
    string_assign_cstr(&plain, text);
    if (plain.size)
        plain.size = text_strip_colour_codes(plain.data, plain.size);
    if (navi_links_has_link(string_view(&plain))) {
        has_link = 1;
        if (!text_array_append(&lines, string_view(&plain))) abort();
    } else {
        text_split_lines(string_cstr(&plain), &lines, CHAT_SPLIT_WIDTH);
    }

    every_tab = kind == CHAT_KIND_ALL || (kind == CHAT_KIND_PLAIN && colour == CHAT_COLOUR_RED);
    i = 0;
    for (node = chat_tabs_first(&g_session1.chatWndTabs); node;
         node = chat_tabs_after(&g_session1.chatWndTabs, cursor_key), i++) {
        struct UIChatHisBox *box;
        unsigned j;

        cursor_key = node->key;

        if (!every_tab && !chat_tab_shows_kind(&node->value, kind))
            continue;
        box = (struct UIChatHisBox *)CHAT_BOX(self, i);
        if (!box)
            continue;
        ui_chat_his_box_log_message(box, text, colour, name);
        for (j = 0; j < lines.size; j++) {
            const char *line = text_array_at(&lines, j).data;
            int box_width = ((struct UIWindow *)CHAT_BOX(self, i))->width;

            if (box_width - CHAT_WRAP_MARGIN < ui_window_text_width(&self->frame.base, line, 0, 0, 0xc, 0, 0) &&
                !has_link)
                ui_chat_his_box_add_wrapped((struct UIChatHisBox *)CHAT_BOX(self, i), line, colour, name);
            else
                ui_chat_his_box_add_line((struct UIChatHisBox *)(CHAT_BOX(self, i)), line, colour, name);
        }
    }
    i = 0;
    for (node = chat_tabs_first(&g_session1.subChatWnds); node;
         node = chat_tabs_after(&g_session1.subChatWnds, cursor_key), i++) {
        cursor_key = node->key;
        if (every_tab || chat_tab_shows_kind(&node->value, kind)) {
            struct SubChatWnd *sub = tree_get(&g_windowMgr.sub_chat_wnds, &i, sizeof i, sizeof *sub, tree_less_int);

            ui_window_send_msg(sub->wnd, &self->frame.base, CHAT_MSG_ADD_LINE, (intptr_t)text, (int)colour, (intptr_t)name, 0);
        }
    }
    string_free(&plain);
    text_array_clear(&lines);
}

static void forget_whisper_name(struct UINewChatWnd *self, const char *text)
{
    struct UIWhisperEditCtrl *whisper = self->whisper;
    struct TextView name = text_view_cstr(text);
    unsigned count = whisper->history.size, i;

    for (i = 0; i < count; ++i) {
        struct TextView line = text_array_at(&whisper->history, i);
        if (line.size == name.size && !memcmp(line.data, name.data, name.size)) {
            text_array_erase(&whisper->history, i);
            if (self->whisper->index == i)
                self->whisper->index = count - 1;
            else if (self->whisper->index > i)
                self->whisper->index--;
            break;
        }
    }
}

static void open_whisper_names(struct UINewChatWnd *self)
{
    struct UIWhisperEditCtrl *whisper = self->whisper;
    struct UIWindow *combo;
    int count, height, x, y, i;

    self->combo_kind = CHAT_COMBO_WHISPER_NAMES;
    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_COMBO_BOX);
    count = whisper->history.size;
    if (!count) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_chat_line(msg_str(MSG_CHAT_NO_WHISPER_NAMES), CHAT_COLOUR_RED);
        return;
    }
    combo = ui_window_open(WND_ID_COMBO_BOX);
    for (i = 0; i < count; i++)
        ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE,
                 (intptr_t)text_array_at(&self->whisper->history, i).data, 0, 0, 0);
    ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)"", 0, 0, 0);
    height = (count + 1) * CHAT_COMBO_ROW_HEIGHT;
    if (height > CHAT_COMBO_MAX_HEIGHT)
        height = CHAT_COMBO_MAX_HEIGHT;
    ui_window_send_msg(combo, NULL, COMBO_MSG_SELECT, self->whisper->index, 0, 0, 0);
    ui_window_resize(combo, CHAT_COMBO_WIDTH, height);
    ui_window_send_msg(combo, NULL, COMBO_MSG_OPENER_ID, WND_ID_CHAT, 0, 0, 0);
    x = 0;
    y = self->frame.base.height - self->bar_height;
    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(combo, x + 1, y - height + 1);
}

static void open_targets(struct UINewChatWnd *self)
{
    struct UIWindow *combo;
    int x, y;

    self->combo_kind = CHAT_COMBO_TARGET;
    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_COMBO_BOX);
    combo = ui_window_open(WND_ID_COMBO_BOX);
    if (!combo)
        return;
    ui_window_send(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)msg_str(MSG_SEND_TO_ALL), 0, 0, 0);
    ui_window_send(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)msg_str(MSG_SEND_TO_PARTY), 0, 0, 0);
    ui_window_send(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)msg_str(MSG_SEND_TO_GUILD), 0, 0, 0);
    ui_window_send(combo, NULL, COMBO_MSG_SELECT, g_session1.chatTarget, 0, 0, 0);
    ui_window_resize(combo, CHAT_COMBO_WIDTH, 0x30);
    ui_window_send(combo, NULL, COMBO_MSG_OPENER_ID, WND_ID_CHAT, 0, 0, 0);
    x = self->frame.base.width - 0x73;
    y = self->frame.base.height - self->bar_height;
    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(combo, x + 1, y - 0x2f);
}

static void combo_chosen(struct UINewChatWnd *self, int line)
{
    struct UIWhisperEditCtrl *whisper;
    int count;

    if (self->combo_kind == CHAT_COMBO_TARGET) {
        struct UIWindow *edit = &self->edit->edit.base;

        g_session1.chatTarget = line;
        if (line == CHAT_TARGET_ALL)
            ui_edit_ctrl_set_text_colour(edit, 0, 0, 0, -1);
        else if (line == CHAT_TARGET_PARTY)
            ui_edit_ctrl_set_text_colour(edit, 0x80, 0, 0x80, -1);
        else if (line == CHAT_TARGET_GUILD)
            ui_edit_ctrl_set_text_colour(edit, 0, 0x80, 0x80, -1);
        ui_window_invalidate_base(&self->frame.base);
        return;
    }
    if (self->combo_kind != CHAT_COMBO_WHISPER_NAMES)
        return;
    whisper = self->whisper;
    count = whisper->history.size;
    if (line >= 0 && line <= count) {
        ui_whisper_edit_ctrl_set_text(&whisper->edit, line == count ? ""
                                   : text_array_at(&whisper->history, line).data);
        self->whisper->index = line;
        whisper = self->whisper;
    }
    ui_window_mgr_set_focus_edit(&g_windowMgr, &whisper->edit.base);
    ui_window_mgr_set_focus_edit(&g_windowMgr, &self->edit->edit.base);
}

static void refuse_link(struct UINewChatWnd *self)
{
    ui_edit_ctrl_set_text_default(&self->edit->edit, "");
    ui_chat_line(msg_str(MSG_CHAT_ITEM_LINK), CHAT_COLOUR_RED);
}

static int line_text(struct UINewChatWnd *self, struct String *text)
{
    struct UIChatEditCtrl *edit = self->edit;
    struct UIItemTagCtrl *popup;
    struct String t;
    int has_link;
    size_t pos, size;

    string_init(text);
    string_assign_cstr(text, ui_edit_ctrl_get_text(&edit->edit.base));
    pos = string_find(text, "\n", 0);
    if (pos != STRING_NPOS)
        string_truncate(text, pos);
    string_init(&t);
    string_assign_cstr(&t, ui_edit_ctrl_get_text(&self->edit->edit.base));
    has_link = ui_chat_edit_ctrl_has_item_link(self->edit, &t);
    string_free(&t);
    if (has_link == 1)
        return 0;
    popup = self->edit->popup;
    if (popup && popup->linked_text.size) {
        string_assign(text, string_cstr(&popup->linked_text), popup->linked_text.size);
        string_clear(&self->edit->popup->linked_text);
        ui_item_tag_ctrl_clear_tags(self->edit->popup);
        size = ui_edit_ctrl_link_text(&self->edit->edit).size;
        if (size)
            return 0;
    }
    size = ui_edit_ctrl_link_text(&self->edit->edit).size;
    if (size) {
        struct TextView link = ui_edit_ctrl_link_text(&self->edit->edit);
        string_assign(text, link.data, link.size);
    }
    string_assign_cstr(&t, ui_edit_ctrl_get_text(&self->edit->edit.base));
    has_link = ui_chat_edit_ctrl_has_navi_link(self->edit, &t);
    string_free(&t);
    return has_link != 1;
}

static void send_line(struct UINewChatWnd *self)
{
    char line[CHAT_LINE_SIZE];
    struct String text, parts[3];
    int type, at, i;

    memset(line, 0, sizeof line);
    if (!line_text(self, &text)) {
        refuse_link(self);
        string_free(&text);
        return;
    }
    snprintf(line, sizeof line, "%s", string_cstr(&text));
    ui_chat_edit_ctrl_commit(self->edit);
    self->focus_edit = &self->edit->edit.base;
    ui_window_mgr_set_focus_edit(&g_windowMgr, &self->edit->edit.base);
    string_clear(&self->edit->edit.raw_text);
    if (!strlen(line))
        goto done;
    if (ui_edit_ctrl_get_text_size(&self->whisper->edit.base) && line[0] != '/') {
        string_assign_cstr(&g_windowMgr.chat_line, line);
        mode_send_message(client_current_mode(), MODE_REQ_WHISPER, (intptr_t)ui_edit_ctrl_get_text(&self->whisper->edit.base), 0, 0,
                      0);
        goto done;
    }
    if (session_is_emotion_text(&g_session1, line))
        goto done;
    for (i = 0; i < 3; i++)
        string_init(&parts[i]);
    at = session_get_talk_type(&g_session1, line, &type, parts);
    if ((type == TALK_NAME_MAKE_GUILD || type == TALK_NAME_MAKE_PARTY || type == TALK_NAME_EX) &&
        (strstr(line, string_cstr(&text_tag_ITEML)) ||
         strstr(line, string_cstr(&text_tag_ITEML_END)))) {
        ui_chat_line(msg_str(MSG_CHAT_NAME_LINK), CHAT_NOTICE_COLOUR);
    } else {
        if (at != -1)
            ui_window_mgr_set_chat_line(&g_windowMgr, line + at);
        mode_send_message(client_current_mode(), MODE_REQ_TALK, type, (intptr_t)parts, 0, 0);
    }
    for (i = 3; i-- > 0;)
        string_free(&parts[i]);
done:
    string_free(&text);
}

static void place_saved(struct UINewChatWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int x = g_windowMgr.chat_wnd_x;

    if (x < 0 || g_windowMgr.chat_wnd_y < 0 || x > g_renderer->width - wnd->width ||
        g_windowMgr.chat_wnd_y > g_renderer->height - wnd->height) {
        x = 0;
        g_windowMgr.chat_wnd_x = 0;
        g_windowMgr.chat_wnd_y = g_renderer->height - wnd->height;
    }
    wnd->x = x;
    wnd->y = g_windowMgr.chat_wnd_y;
    ui_new_chat_wnd_set_size_step(self, g_windowMgr.chat_wnd_status);
    ui_new_chat_wnd_move(wnd, wnd->x, wnd->y);
}

static void page_box(struct UINewChatWnd *self, int msg)
{
    struct UIListBox *box = (struct UIListBox *)CHAT_BOX(self, self->tab);

    if (box && box->has_vscroll)
        ui_window_send_msg((struct UIWindow *)box, NULL, msg, 0, 0, 0, 0);
}

static void command(struct UINewChatWnd *self, int id, intptr_t val2)
{
    struct UIWindow *options;

    switch (id) {
    case CHAT_CMD_SEND:
        if (!g_windowMgr.chat_bar_shown && g_windowMgr.chat_bar_toggleable)
            return;
        send_line(self);
        return;
    case CHAT_ID_BATTLE_OPTION:
        options = ui_window_open(WND_ID_BATTLE_MSG_OPTION);
        if (options)
            tell_battle_options(self, options, 0);
        return;
    case CHAT_ID_MINI:
        close_tab(self);
        return;
    case CHAT_ID_BAR_SYS_1:
        ui_new_chat_wnd_send_msg(&self->frame.base, NULL, CHAT_MSG_STEP_KEY, 0, 0, 0, 0);
        return;
    case CHAT_ID_BAR_DIALOG:
        open_whisper_names(self);
        return;
    case CHAT_ID_BAR_SYS_2:
        open_targets(self);
        return;
    case CHAT_CMD_SET_WHISPER_NAME:
        ui_window_mgr_set_whisper_name(&g_windowMgr, (const char *)val2);
        return;
    case CHAT_CMD_SHOW_LINES:
        g_windowMgr.chat_wnd_stick_on = 1;
        self->tab = 0;
        if (!self->saved_step)
            self->saved_step = 1;
        ui_new_chat_wnd_set_size_step(self, self->saved_step);
        ui_window_invalidate_base(&self->frame.base);
        return;
    case CHAT_ID_STICK:
        ui_new_chat_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_TAB_DRAGGED, self->tab, 0, 0, 0);
        return;
    case CHAT_ID_TAB_OPTION:
        new_tab(self);
        return;
    }
}

intptr_t ui_new_chat_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                    intptr_t val3, intptr_t val4)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    struct ChatTab *node;
    int step, cursor_key;

    switch (msg) {
    case CHAT_MSG_STEP_KEY:
        if (!CHAT_LINES_SHOWN)
            break;
        step = val1 == 0 ? 1 : -1;
        if (SIZE_STEP(self))
            step *= 3;
        step += SIZE_STEP(self);
        if (step > 0)
            ui_new_chat_wnd_set_size_step(self, step / 3 * 3 + 1);
        else
            ui_new_chat_wnd_set_size_step(self, SIZE_STEP(self) ? 0 : step);
        break;
    case CHAT_MSG_PAGE_UP:
        page_box(self, UI_MSG_PAGE_UP);
        break;
    case CHAT_MSG_PAGE_DOWN:
        page_box(self, UI_MSG_PAGE_DOWN);
        break;
    case UI_MSG_COMMAND:
        command(self, val1, val2);
        break;
    case UI_MSG_RESIZE:
        if (val1 != UI_EDGE_TOP || !CHAT_LINES_SHOWN)
            break;
        if (val3 >= self->tab_height + 0x38) {
            step = (val3 - (self->tab_height + 0x38)) / 14 + 1;
            if (step > 15)
                step = 15;
            if (SIZE_STEP(self) != step)
                ui_new_chat_wnd_set_size_step(self, step);
        } else if (SIZE_STEP(self)) {
            ui_new_chat_wnd_set_size_step(self, 0);
        }
        break;
    case CHAT_MSG_SET_STEP:
        ui_new_chat_wnd_set_size_step(self, val1);
        break;
    case CHAT_MSG_SHOW_IF_HIDDEN:
        if (CHAT_LINES_SHOWN && !SIZE_STEP(self))
            ui_new_chat_wnd_set_size_step(self, 1);
        break;
    case UI_MSG_TAB_CHANGED:
        if (val1 != self->tab) {
            self->tab = val1;
            ui_new_chat_wnd_show_tab(self, 1);
        } else {
            ui_new_chat_wnd_show_tab(self, 0);
        }
        break;
    case CHAT_MSG_PLACE:
        place_saved(self);
        break;
    case CHAT_MSG_ADD_LINE:
        add_line(self, (const char *)val1, (COLORREF)val2, val3, (const char *)val4);
        break;
    case CHAT_MSG_LOG_LINE:
        break;
    case COMBO_MSG_CHOSEN:
        combo_chosen(self, val1);
        break;
    case CHAT_MSG_FOCUS_LINE:
        ui_window_mgr_set_focus_edit(&g_windowMgr, &self->edit->edit.base);
        break;
    case CHAT_MSG_FORGET_WHISPER:
        forget_whisper_name(self, (const char *)val1);
        break;
    case UI_MSG_TAB_RENAMED:
        tab_renamed(self, sender, (const char *)val1, val2, val3);
        break;
    case UI_MSG_TAB_DRAGGED:
        tear_out_tab(self, sender, val1);
        break;
    case CHAT_MSG_STICK_TAB:
        stick_tab(self, sender);
        break;
    case CHAT_MSG_RESOLVE_PENDING_NAME:
        for (node = chat_tabs_first(&g_session1.chatWndTabs); node;
             node = chat_tabs_after(&g_session1.chatWndTabs, cursor_key)) {
            cursor_key = node->key;

            ui_chat_his_box_resolve_pending_name((struct UIChatHisBox *)CHAT_BOX(self, node->key), (const char *)val1);
        }
        break;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
    return 0;
}
