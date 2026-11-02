#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "ui/chat/speaker_name.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include "game/context.h"
#include <string.h>
#include <mbstring.h>
#include "ui/chat/chat_room_wnd.h"
#include "ui/chat/markup_links.h"
#include "ui/ui_std.h"
#include <time.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "ui/wndmgr/wndmgr.h"

struct UIChatRoomWnd;
struct UIWindow;

const struct UIWindowOps ui_chat_room_wnd_ops = {
    .destroy = ui_chat_room_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_chat_room_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_chat_room_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_chat_room_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_chat_room_wnd_on_draw,
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
    .send_msg = ui_chat_room_wnd_send_msg,
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

#define CHAT_ROOM_ID 0xb0
#define CHAT_ROOM_EDIT_ID 0x1e6
#define CHAT_ROOM_GRIP_EDGE 7
#define CHAT_ROOM_TITLE_HEIGHT 0x11

#define CHAT_ROOM_MIN_WIDTH 0x118
#define CHAT_ROOM_MIN_HEIGHT 0x78
#define CHAT_ROOM_MAX_WIDTH 0x200
#define CHAT_ROOM_MAX_HEIGHT 0x180
#define CHAT_ROOM_SIZE_STEP 32

#define CHAT_ROOM_MSG_PAGE_UP 3
#define CHAT_ROOM_MSG_PAGE_DOWN 4
#define CHAT_ROOM_MSG_SET_ROOM 0x22
#define CHAT_ROOM_MSG_ADD_LINE 0x25
#define CHAT_ROOM_MSG_CLEAR_MEMBERS 0x2b
#define CHAT_ROOM_MSG_ADD_MEMBER 0x2c
#define CHAT_ROOM_MSG_DEL_MEMBER 0x2d
#define CHAT_ROOM_MSG_SET_MEMBER 0x2e
#define CHAT_ROOM_MSG_OPEN_MENU 0x31
#define CHAT_ROOM_MSG_SET_INFO 0x35

#define CHAT_ROOM_ID_SEND UI_ID_OK

#define MEMBER_OWNER_VALUE 0
#define MEMBER_NO_MENU (-1)
#define MEMBER_OWNER_COLOUR 0x808000
#define MEMBER_COLOUR 0

#define WND_ID_MENU 0x12
#define MENU_MSG_FIT 0x1e
#define MENU_MSG_ADD_ITEM 0x1f
#define MENU_MSG_CHOICE 0x30
#define MENU_MSG_SET_OWNER 0x53
#define CHAT_ROOM_MENU_KICK 0
#define CHAT_ROOM_MENU_GIVE_OWNER 1
#define CHAT_ROOM_MENU_SETUP 2
#define CHAT_ROOM_MENU_NOTE 3
#define CHAT_ROOM_MENU_SAVE 0xb
#define CHAT_ROOM_MENU_ADD_FRIEND 0x15
#define MSG_MENU_OWNER_0 0x7f
#define MSG_MENU_OWNER_1 0x80
#define MSG_MENU_SETUP 0x7e
#define MSG_MENU_NOTE 0x81
#define MSG_MENU_SAVE 0x138
#define MSG_MENU_ADD_FRIEND 0x166
#define MSG_MENU_NOTE_TEXT 0xbf

#define MODE_REQ_WHISPER 0xb
#define MODE_REQ_TALK 0x2a
#define MODE_REQ_MENU_OWNER_1 0x2e
#define MODE_REQ_MENU_OWNER_0 0x2f
#define MODE_REQ_LEAVE_ROOM 0x30
#define TALK_TYPE_PLAIN 0x32
#define TALK_TYPE_SAVE 0x33
#define TALK_TYPE_SAVE_ROOM 0x34

#define MSG_ITEM_LINK_REFUSED 0xbe2
#define MSG_TEXT_NOT_ASCII 0xbe
#define MSG_ROOM_RESTORED 0x40
#define MSG_ROOM_PRIVATE_LABEL 0x59
#define MSG_ROOM_PUBLIC_LABEL 0x5a
#define UIWM_MSG_SHOW_CHAT 3

#define LIST_ITEM(list, i) (&((struct String *)(list)->items.first)[i])

static int member_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

void chat_room_member_destroy(void *value)
{
    string_free(&((struct ChatRoomMember *)value)->name);
}

static int *members_at(struct Tree *map, struct String *key, int move)
{
    struct ChatRoomMember *member = tree_find(map, key, member_less), fresh;

    if (member)
        return &member->value;
    if (move) {
        fresh.name = *key;
        string_init(key);
    } else {
        string_copy(&fresh.name, key);
    }
    fresh.value = 0;
    member = tree_add(map, &fresh, sizeof fresh, member_less, NULL);
    return &member->value;
}

struct UIChatRoomWnd *ui_chat_room_wnd_construct(struct UIChatRoomWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_chat_room_wnd_ops;
    string_init(&self->room.title);
    string_init(&self->room.password);
    tree_init(&self->members);
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_chat_room_wnd_new(void)
{
    return (struct UIWindow *)ui_chat_room_wnd_construct(mem_alloc(sizeof(struct UIChatRoomWnd)));
}

void ui_chat_room_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIChatRoomWnd *self = (struct UIChatRoomWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit;
    void *mem;
    int member_width;

    self->frame.default_id = CHAT_ROOM_ID;

    mem = mem_alloc(sizeof(struct UIResizer));
    self->resizer = mem ? (struct UIWindow *)ui_resizer_construct(mem) : NULL;
    ui_window_create(self->resizer, 0x10, 0x10);
    ui_resizer_set_edge((struct UIResizer *)self->resizer, CHAT_ROOM_GRIP_EDGE);
    ui_resizer_set_bitmap((struct UIResizer *)self->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(self->resizer, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours((struct UIResizer *)self->resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, self->resizer);

    mem = mem_alloc(sizeof(struct UIChatEditCtrl));
    self->edit = mem ? ui_chat_edit_ctrl_construct(mem) : NULL;
    edit = &self->edit->edit.base;
    ui_window_create(edit, wnd->width - 0x1e, 0x10);
    ui_window_move_base(edit, 3, wnd->height - 0x12);
    ((struct UIEditCtrl *)edit)->max_chars = 0x46;
    ui_edit_ctrl_set_back_colour(edit, 0xf2, 0xf2, 0xf2);
    ui_window_add_child(wnd, edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, edit);
    edit->id = CHAT_ROOM_EDIT_ID;

    mem = mem_alloc(sizeof(struct UIChatHisBox2));
    self->talk = mem ? ui_chat_his_box2_construct(mem) : NULL;
    member_width = chat_room_member_width(wnd->width);
    ui_chat_his_box2_create(&self->talk->box.list, 0, 0, wnd->width - member_width - 3, wnd->height - 0x26, 0);
    ui_window_move_base((struct UIWindow *)self->talk, 1, CHAT_ROOM_TITLE_HEIGHT);
    ((struct UIWindow *)self->talk)->id = CHAT_ROOM_ID;
    ui_window_add_child(wnd, (struct UIWindow *)self->talk);

    mem = mem_alloc(sizeof(struct UIMemberListBox));
    self->member_list = mem ? &ui_member_list_box_construct(mem)->base : NULL;
    ui_chat_his_box2_create(&self->member_list->box.list, 0, 0, chat_room_member_width(wnd->width), wnd->height - 0x26, 0);
    ui_window_move_base((struct UIWindow *)self->member_list, wnd->width - chat_room_member_width(wnd->width) - 1,
             CHAT_ROOM_TITLE_HEIGHT);
    ((struct UIWindow *)self->member_list)->id = CHAT_ROOM_ID;
    ui_window_add_child(wnd, (struct UIWindow *)self->member_list);

    self->close = ui_window_add_close_button(wnd, "/q", 0);

    mem = mem_alloc(sizeof(struct UIStaticText));
    self->title = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(self->title, wnd->width - 0x1e, 0xf);
    ui_window_move(self->title, 9, 2);
    ui_static_text_set_text_ellipsis(self->title, "");
    ui_static_text_set_font(self->title, 0, 0xc, 0, 0);
    ui_window_add_child(wnd, self->title);
}

static void send_line(struct UIChatRoomWnd *self, int type)
{
    struct UIWindow *edit = &self->edit->edit.base;
    struct UIListBox *members = &self->member_list->box.list;
    struct String line, name, parts[3];
    char buf[0x100];
    BYTE own = 0;
    int at, i;

    if (!ui_edit_ctrl_get_text_size(edit))
        return;
    if (navi_links_has_link(text_view_cstr(ui_edit_ctrl_get_text(edit)))) {
        ui_edit_ctrl_set_text_default((struct UIEditCtrl *)edit, "");
        ui_chat_line(msg_str(MSG_ITEM_LINK_REFUSED), 0xff);
        return;
    }

    string_init(&line);
    string_assign_cstr(&line, ui_edit_ctrl_get_text(edit));
    if (((struct UIEditCtrl *)edit)->navigation_link) {
        struct TextView link = ui_edit_ctrl_link_text((struct UIEditCtrl *)edit);

        string_assign(&line, link.data, link.size);
    } else if (_mbsstr((const unsigned char *)ui_edit_ctrl_get_text(edit), (const unsigned char *)"<") &&
               self->edit->popup && self->edit->popup->linked_text.size) {
        string_assign(&line, string_cstr(&self->edit->popup->linked_text), self->edit->popup->linked_text.size);
    }
    if (line.size)
        strcpy(buf, string_cstr(&line));
    else
        strcpy(buf, ui_edit_ctrl_get_text(edit));

    if (g_serviceType == 1 && !session_is_ascii_name(&g_session1, buf)) {
        ui_chat_line(msg_str(MSG_TEXT_NOT_ASCII), 0xff);
        string_free(&line);
        return;
    }

    if (members->selected != -1) {
        const char *me;

        string_copy(&name, LIST_ITEM(members, members->selected));
        me = session_character_name(&g_session1);
        own = string_equal_cstr(&name, me);
        string_free(&name);
    }
    if (members->selected != -1 && buf[0] != '/' && !own) {
        string_assign_cstr(&g_windowMgr.chat_line, buf);
        string_copy(&name, LIST_ITEM(members, members->selected));
        mode_send_message(client_current_mode(), MODE_REQ_WHISPER, (intptr_t)string_cstr(&name), 0, 0, 0);
        string_free(&name);
    } else if (!session_is_emotion_text(&g_session1, buf)) {
        for (i = 0; i < 3; i++)
            string_init(&parts[i]);
        at = session_get_talk_type(&g_session1, buf, &type, parts);
        if (at != -1)
            ui_window_mgr_set_chat_line(&g_windowMgr, buf + at);
        if (type == 0)
            type = TALK_TYPE_PLAIN;
        else if (type == TALK_TYPE_SAVE)
            type = TALK_TYPE_SAVE_ROOM;
        mode_send_message(client_current_mode(), MODE_REQ_TALK, type, (int)(INT_PTR)parts, 0, 0);
        for (i = 3; i-- > 0;)
            string_free(&parts[i]);
    }
    ui_chat_edit_ctrl_commit(self->edit);
    ui_edit_ctrl_set_text_default((struct UIEditCtrl *)edit, "");
    ui_window_mgr_set_focus_edit(&g_windowMgr, edit);
    string_free(&line);
}

static void add_line(struct UIChatRoomWnd *self, char *text, COLORREF colour)
{
    char name[0x19];
    struct String line;
    struct TextArray lines = {0};
    int width, in, out, colon;
    unsigned i;

    memset(name, 0, sizeof name);
    width = self->frame.base.width * 80 / 640;
    string_init(&line);
    string_assign_cstr(&line, text);
    in = string_find(&line, ")님이 입장 하였습니다."
                          , 0);
    out = string_find(&line, ")님이 퇴장 하였습니다."
                           , 0);
    colon = string_find(&line, " :", 0);
    chat_speaker_name(name, text, in, out, colon, g_serviceType == SERVICE_UAE && g_windowMgr.arabic_text);
    if (strcmp(name, session_character_name(&g_session1)) == 0)
        name[0] = 0;
    string_free(&line);

    if (_mbsstr((const unsigned char *)text, (const unsigned char *)string_cstr(&text_tag_ITEML)) ||
        _mbsstr((const unsigned char *)text, (const unsigned char *)string_cstr(&text_tag_ITEML_END))) {
        if (!text_array_append(&lines, text_view_cstr(text))) abort();
    } else {
        text_split_lines(text, &lines, width);
    }
    for (i = 0; i < lines.size; i++)
        ui_chat_his_box_add_line(&self->talk->box, text_array_at(&lines, i).data, colour, name);
    text_array_clear(&lines);
}

static void set_room(struct UIChatRoomWnd *self, const struct ChatRoomInfo *room)
{
    struct ChatRoomInfo *mine = &self->room;
    struct String label, head;
    char line[0x100];
    const char *cr;

    string_assign(&mine->title, string_cstr(&room->title), room->title.size);
    string_assign(&mine->password, string_cstr(&room->password), room->password.size);
    mine->type = room->type;
    mine->users = room->users;
    mine->max_users = room->max_users;
    mine->room_id = room->room_id;

    cr = mine->title.size ? memchr(string_cstr(&mine->title), '\r', mine->title.size) : NULL;
    if (cr) {
        size_t at = (size_t)(cr - string_cstr(&mine->title));

        string_init(&head);
        string_assign(&head, string_cstr(&mine->title), at < mine->title.size ? at : mine->title.size);
        string_free(&mine->title);
        string_move(&mine->title, &head);
    }

    string_init(&label);
    string_assign_cstr(&label,
                           msg_str(mine->type == CHAT_ROOM_PRIVATE ? MSG_ROOM_PRIVATE_LABEL : MSG_ROOM_PUBLIC_LABEL));
    sprintf(line, "%s%s (%d/%d)", string_cstr(&label), string_cstr(&mine->title), mine->users,
                mine->max_users);
    ui_static_text_set_text_ellipsis(self->title, line);
    ui_window_invalidate_base(&self->frame.base);
    string_free(&label);
}

static void list_members(struct UIChatRoomWnd *self)
{
    struct TreeNode *node;

    ui_chat_his_box_clear_items(&self->member_list->box.list);
    for (node = tree_first(&self->members); node != self->members.head; node = tree_next(node)) {
        struct ChatRoomMember *member = TREE_VALUE(node, struct ChatRoomMember);

        ui_chat_his_box_add_line(&self->member_list->box, string_cstr(&member->name), member->value != MEMBER_OWNER_VALUE ? MEMBER_COLOUR : MEMBER_OWNER_COLOUR, NULL);
    }
}

static struct ChatRoomMember *find_member(struct UIChatRoomWnd *self, const char *name)
{
    struct String key;
    struct ChatRoomMember *member;

    string_init(&key);
    string_assign_cstr(&key, name);
    member = tree_find(&self->members, &key, member_less);
    string_free(&key);
    return member;
}

static void add_member(struct UIChatRoomWnd *self, const char *name, int value)
{
    struct String key;
    COLORREF colour;

    string_init(&key);
    string_assign_cstr(&key, name);
    *members_at(&self->members, &key, 1) = value;
    string_free(&key);
    colour = MEMBER_COLOUR;
    if (value == MEMBER_OWNER_VALUE)
        colour = MEMBER_OWNER_COLOUR;
    ui_chat_his_box_add_line(&self->member_list->box, name, colour, NULL);
}

static void set_member(struct UIChatRoomWnd *self, const char *name, int value)
{
    struct String key;

    if (find_member(self, name)) {
        string_init(&key);
        string_assign_cstr(&key, name);
        *members_at(&self->members, &key, 1) = value;
        string_free(&key);
    }
    list_members(self);
}

static void del_member(struct UIChatRoomWnd *self, const char *name)
{
    struct ChatRoomMember *member = find_member(self, name);

    if (!member)
        return;
    tree_erase(&self->members, TREE_NODE_OF(member), chat_room_member_destroy);
    list_members(self);
}

static void clear_members(struct UIChatRoomWnd *self)
{
    ui_chat_his_box_clear_items(&self->member_list->box.list);
    tree_clear(&self->members, chat_room_member_destroy);
}

static void add_menu_entry(struct UIChatRoomWnd *self, int msg, int id)
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

static int chosen_member(struct UIChatRoomWnd *self, struct String *out)
{
    struct UIListBox *members = &self->member_list->box.list;

    if (members->selected == -1)
        return 0;
    string_copy(out, LIST_ITEM(members, members->selected));
    return 1;
}

static int fill_menu(struct UIChatRoomWnd *self, int my_value, const struct String *me)
{
    struct String target;

    if (!chosen_member(self, &target)) {
        if (my_value == MEMBER_OWNER_VALUE)
            add_menu_entry(self, MSG_MENU_SETUP, CHAT_ROOM_MENU_SETUP);
        add_menu_entry(self, MSG_MENU_SAVE, CHAT_ROOM_MENU_SAVE);
        return 1;
    }
    if (!tree_find(&self->members, &target, member_less)
        || *members_at(&self->members, &target, 0) == MEMBER_NO_MENU) {
        string_free(&target);
        return 0;
    }
    switch (my_value) {
    case MEMBER_OWNER_VALUE:
        if (!string_equal(&target, me)) {
            add_menu_entry(self, MSG_MENU_OWNER_0, CHAT_ROOM_MENU_KICK);
            add_menu_entry(self, MSG_MENU_OWNER_1, CHAT_ROOM_MENU_GIVE_OWNER);
        }
        add_menu_entry(self, MSG_MENU_NOTE, CHAT_ROOM_MENU_NOTE);
        if (g_serverType != 3 && !ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&target)) &&
            !string_equal(&target, me))
            add_menu_entry(self, MSG_MENU_ADD_FRIEND, CHAT_ROOM_MENU_ADD_FRIEND);
        add_menu_entry(self, MSG_MENU_SETUP, CHAT_ROOM_MENU_SETUP);
        break;
    case 1:
        add_menu_entry(self, MSG_MENU_NOTE, CHAT_ROOM_MENU_NOTE);
        if (!ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&target)) && !string_equal(&target, me) &&
            g_serverType != 3)
            add_menu_entry(self, MSG_MENU_ADD_FRIEND, CHAT_ROOM_MENU_ADD_FRIEND);
        break;
    }
    string_free(&target);
    return 1;
}

static void open_menu(struct UIChatRoomWnd *self)
{
    struct UIMenuEntry *entry;
    struct String me;
    struct UIWindow *menu;
    unsigned i;
    int my_value, x, y;

    for (entry = MENU_ENTRIES(&self->menu); entry != (struct UIMenuEntry *)self->menu.last; entry++)
        string_free(&entry->text);
    self->menu.last = self->menu.first;
    if (g_windowMgr.menu_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_MENU);

    string_init(&me);
    string_assign_cstr(&me, session_character_name(&g_session1));
    if (!tree_find(&self->members, &me, member_less))
        goto done;
    my_value = *members_at(&self->members, &me, 0);
    if (my_value == MEMBER_NO_MENU || !fill_menu(self, my_value, &me))
        goto done;

    if (MENU_ENTRY_COUNT(&self->menu)) {
        menu = ui_window_open(WND_ID_MENU);
        for (i = 0; i < MENU_ENTRY_COUNT(&self->menu); i++)
            ui_window_send_msg(menu, NULL, MENU_MSG_ADD_ITEM, (intptr_t)string_cstr(&MENU_ENTRIES(&self->menu)[i].text),
                     0, 0, 0);
        ui_window_send_msg(menu, NULL, MENU_MSG_SET_OWNER, 0x1c, 0, 0, 0);
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
done:
    string_free(&me);
}

static void menu_choice(struct UIChatRoomWnd *self, int index)
{
    struct String name;
    struct UIWindow *setup;

    switch (MENU_ENTRIES(&self->menu)[index].id) {
    case CHAT_ROOM_MENU_KICK:
        if (chosen_member(self, &name)) {
            game_send_message(MODE_REQ_MENU_OWNER_0, (intptr_t)string_cstr(&name), 0, 0, 0);
            string_free(&name);
        }
        break;
    case CHAT_ROOM_MENU_GIVE_OWNER:
        if (chosen_member(self, &name)) {
            game_send_message(MODE_REQ_MENU_OWNER_1, (intptr_t)string_cstr(&name), 0, 0, 0);
            string_free(&name);
        }
        break;
    case CHAT_ROOM_MENU_SETUP: {
        struct ChatRoomInfo *room;
        if (!game_current_mode())
            break;
        setup = ui_make_window(WND_ID_CHAT_ROOM_CHANGE);
        room = game_current_chat_room();
        if (setup && room)
            ui_window_send_msg(setup, NULL, CHAT_ROOM_MSG_SET_ROOM, (intptr_t)room, 0, 0, 0);
        else if (setup)
            ui_window_mgr_post_quit2(&g_windowMgr, setup);
        break;
    }
    case CHAT_ROOM_MENU_NOTE:
        ui_message_box(msg_str(MSG_MENU_NOTE_TEXT));
        break;
    case CHAT_ROOM_MENU_SAVE:
        ui_chat_room_wnd_save_talk(self);
        break;
    case CHAT_ROOM_MENU_ADD_FRIEND:
        if (chosen_member(self, &name)) {
            ui_window_mgr_request_add_friend(&g_windowMgr, string_cstr(&name));
            string_free(&name);
        }
        break;
    }
}

static void resize(struct UIChatRoomWnd *self, int width, int height)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit = &self->edit->edit.base;
    struct String title;

    if (width > CHAT_ROOM_MAX_WIDTH)
        width = CHAT_ROOM_MAX_WIDTH;
    if (height > CHAT_ROOM_MAX_HEIGHT)
        height = CHAT_ROOM_MAX_HEIGHT;
    if (width < CHAT_ROOM_MIN_WIDTH)
        width = CHAT_ROOM_MIN_WIDTH;
    if (height < CHAT_ROOM_MIN_HEIGHT)
        height = CHAT_ROOM_MIN_HEIGHT;
    width = (width - CHAT_ROOM_MIN_WIDTH) / CHAT_ROOM_SIZE_STEP * CHAT_ROOM_SIZE_STEP + CHAT_ROOM_MIN_WIDTH;
    height = (height - CHAT_ROOM_MIN_HEIGHT) / CHAT_ROOM_SIZE_STEP * CHAT_ROOM_SIZE_STEP + CHAT_ROOM_MIN_HEIGHT;
    if (width != wnd->width || height != wnd->height) {
        ui_window_resize_base(wnd, width, height);
        ui_window_move_base(self->resizer, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_resize_base(edit, wnd->width - 0x1e, 0x10);
        ui_window_move_base(edit, 3, wnd->height - 0x12);
        ui_window_resize((struct UIWindow *)self->talk, wnd->width - chat_room_member_width(wnd->width) - 3,
                 wnd->height - 0x26);
        ui_window_move_base((struct UIWindow *)self->talk, 1, CHAT_ROOM_TITLE_HEIGHT);
        ui_chat_his_box_resize_to(&self->member_list->box, chat_room_member_width(wnd->width), wnd->height - 0x26,
                              0);
        ui_window_move_base((struct UIWindow *)self->member_list, wnd->width - chat_room_member_width(wnd->width) - 1,
                 CHAT_ROOM_TITLE_HEIGHT);
        ui_window_move_base(self->close, wnd->width - 0xe, 3);
        string_init(&title);
        string_assign_cstr(&title, string_cstr(&((struct UIStaticText *)self->title)->tooltip));
        ui_window_resize(self->title, wnd->width - 0x1e, 0xc);
        ui_static_text_set_text_ellipsis(self->title, string_cstr(&title));
        string_free(&title);
    }
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

intptr_t ui_chat_room_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                     intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIChatRoomWnd *self = (struct UIChatRoomWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case CHAT_ROOM_MSG_PAGE_UP:
    case CHAT_ROOM_MSG_PAGE_DOWN:
        if (self->talk->box.list.has_vscroll)
            ui_chat_his_box_send_msg((struct UIWindow *)self->talk, NULL,
                     msg == CHAT_ROOM_MSG_PAGE_UP ? UI_MSG_PAGE_UP : UI_MSG_PAGE_DOWN, 0, 0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
        if (val1 == CHAT_ROOM_ID_SEND)
            send_line(self, val2);
        else if (val1 == UI_ID_CLOSE)
            mode_send_message(client_current_mode(), MODE_REQ_LEAVE_ROOM, 0, 0, 0, 0);
        else if (val1 == UI_ID_HISTORY_LINE)
            ui_window_mgr_set_whisper_name(&g_windowMgr, (const char *)val2);
        return 0;
    case UI_MSG_RESIZE:
        if (val1 == CHAT_ROOM_GRIP_EDGE)
            resize(self, val2, val3);
        return 0;
    case CHAT_ROOM_MSG_SET_ROOM:
        set_room(self, (const struct ChatRoomInfo *)val1);
        return 0;
    case CHAT_ROOM_MSG_ADD_LINE:
        add_line(self, (char *)val1, (COLORREF)val2);
        return 0;
    case CHAT_ROOM_MSG_CLEAR_MEMBERS:
        clear_members(self);
        return 0;
    case CHAT_ROOM_MSG_ADD_MEMBER:
        add_member(self, (const char *)val1, val2);
        return 0;
    case CHAT_ROOM_MSG_DEL_MEMBER:
        del_member(self, (const char *)val1);
        return 0;
    case CHAT_ROOM_MSG_SET_MEMBER:
        set_member(self, (const char *)val1, val2);
        return 0;
    case MENU_MSG_CHOICE:
        menu_choice(self, val1);
        return 0;
    case CHAT_ROOM_MSG_OPEN_MENU:
        open_menu(self);
        return 0;
    case CHAT_ROOM_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!chat_room_info_is_valid(self->info))
            chat_room_info_set_default(self->info);
        ui_chat_room_wnd_send_msg(wnd, NULL, UI_MSG_RESIZE, CHAT_ROOM_GRIP_EDGE, self->info->width, self->info->height, 0);
        ui_chat_room_wnd_move(wnd, self->info->x, self->info->y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
