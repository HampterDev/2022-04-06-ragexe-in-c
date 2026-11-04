#include "game/chat_room.h"
#include "game/packets.h"
#include "std/tree.h"
#include "ui/actor_labels.h"
#include <stdlib.h>
#include <string.h>

struct ChatRoomInfo *chat_room_info_copy(struct ChatRoomInfo *room,
                                                  const struct ChatRoomInfo *from)
{
    string_copy(&room->title, &from->title);
    string_copy(&room->password, &from->password);
    room->type = from->type;
    room->users = from->users;
    room->max_users = from->max_users;
    room->room_id = from->room_id;
    return room;
}

struct ChatRoomInfo *chat_room_info_assign(struct ChatRoomInfo *room,
                                                    const struct ChatRoomInfo *from)
{
    if (room != from)
        string_assign(&room->title, string_cstr(&from->title), from->title.size);
    string_assign(&room->password, string_cstr(&from->password), from->password.size);
    room->type = from->type;
    room->users = from->users;
    room->max_users = from->max_users;
    room->room_id = from->room_id;
    return room;
}

enum {
    ACTOR_MSG_LEFT_ROOM_ENTRY = 0x21,
    WID_CHAT_ROOM = 0x1c,
    CHAT_ROOM_MSG_CLEAR = 0x2b,
    CHAT_ROOM_MSG_INFO = 0x22,
    CHAT_ROOM_MSG_ADD_MEMBER = 0x2c
};

struct EnteredMember {
    int role;
    char name[25];
};

void game_mode_zc_enter_room(struct GameMode *self, const struct PACKET_ZC_ENTER_ROOM *packet)
{
    struct GameActor *owner;
    struct ChatRoomOwner *room_owner;
    struct ChatRoomInfo room;
    const struct ChatRoomInfo *shown;
    struct EnteredMember *members = NULL;
    struct UIWindow *window;
    size_t count, i;
    unsigned room_id;
    int is_this_room = 0;

    if (packet->length < (int)offsetof(struct PACKET_ZC_ENTER_ROOM, members))
        return;
    self->chat_room_pending = 0;
    room_id = (unsigned)packet->room_id;
    room_owner = tree_get(&self->chat_room_owners, &room_id, sizeof room_id, sizeof *room_owner, tree_less_uint);
    owner = world_get_game_actor_by_aid(self->world, room_owner->aid);
    if (!owner) return;
    string_init(&room.title);
    string_init(&room.password);
    shown = ui_chat_room_title_info(owner->chatroom_title_wnd);
    if (shown) {
        chat_room_info_assign(&room, shown);
        is_this_room = packet->room_id == shown->room_id;
    }
    actor_send(owner, ACTOR_MSG_LEFT_ROOM_ENTRY, 0, 0);
    if (!is_this_room) goto done;

    count = ((unsigned)packet->length - offsetof(struct PACKET_ZC_ENTER_ROOM, members))
        / sizeof(struct ChatRoomMemberEntry);
    if (count) {
        members = malloc(count * sizeof *members);
        if (!members) goto done;
        for (i = 0; i < count; ++i) {
            members[i].role = packet->members[i].role;
            memcpy(members[i].name, packet->members[i].name, 24);
            members[i].name[24] = '\0';
        }
    }
    window = ui_window_open(WID_CHAT_ROOM);
    ui_window_send(window, NULL, CHAT_ROOM_MSG_CLEAR, 0, 0, 0, 0);
    room.users = (int)count;
    ui_window_send(window, NULL, CHAT_ROOM_MSG_INFO, (intptr_t)&room, 0, 0, 0);
    for (i = 0; i < count; ++i)
        ui_window_send(window, NULL, CHAT_ROOM_MSG_ADD_MEMBER, (intptr_t)members[i].name, members[i].role, 0, 0);
    chat_room_info_assign(&self->own_chat_room, &room);

done:
    free(members);
    string_free(&room.password);
    string_free(&room.title);
}
