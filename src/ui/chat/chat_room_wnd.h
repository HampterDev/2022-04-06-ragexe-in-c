#include "session/character_name.h"
#ifndef UI_CHAT_CHAT_ROOM_WND_H
#define UI_CHAT_CHAT_ROOM_WND_H

#include "core/markup_tags.h"
#include "ui/chat/chat.h"
#include "ui/chat/chat_history.h"
#include "ui/ui_std.h"

struct ChatRoomMember {
    struct String name;
    int value;
};

void chat_room_member_destroy(void *value);

struct UIChatRoomWnd {
    struct UIFrameWnd frame;
    struct ChatRoomInfo room;
    struct UIWindow *resizer;
    struct UIChatEditCtrl *edit;
    struct UIChatHisBox2 *talk;
    struct UIChatHisBox2 *member_list;
    struct UIWindow *close;
    struct UIWindow *title;
    struct Tree members;
    struct Vec menu;
    struct WindowInfo *info;
};

static __inline int chat_room_member_width(int width)
{
    return width * 56 / 280;
}

struct UIChatRoomWnd *ui_chat_room_wnd_construct(struct UIChatRoomWnd *self);
struct UIWindow *ui_chat_room_wnd_new(void);
void ui_chat_room_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
int ui_chat_room_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1,
                                     int val2, int val3, int val4);
void ui_chat_room_wnd_save_talk(struct UIChatRoomWnd *self);

char guild_string_differs(const struct String *a, const struct String *b);
struct UIMemberListBox {
    struct UIChatHisBox2 base;
};
struct UIMemberListBox *ui_member_list_box_construct(struct UIMemberListBox *self);
int chat_room_info_is_valid(struct WindowInfo *self);
void chat_room_info_set_default(struct WindowInfo *self);

void ui_chat_room_wnd_destroy(struct UIWindow *window);

void ui_chat_room_wnd_move(struct UIWindow *window, int x, int y);
void ui_chat_room_wnd_store_info(struct UIWindow *window);
void ui_chat_room_wnd_on_draw(struct UIWindow *window);

struct UIChatRoomTitle *ui_chat_room_title_new(void);

#endif
