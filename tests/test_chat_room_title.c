/* Actor message 0x20 (actor_sendmsg.c) carries the chat room an actor shows in par1, a pointer
   (packets_chat.c, ACTOR_MSG_CHAT_ROOM), and hands it to the room's title window as val1 of window
   message 0x22, which reads it as a struct ChatRoomInfo * (chat_room.c). The case is extracted and
   run against recording stubs: the value must reach the window whole. On a 64-bit build the test
   passes a value with bits above bit 31 (never dereferenced); there a cut to int loses them. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int64_t RenderMsgArg;
struct UIWindow { int unused; };
struct UIWindowMgr { int unused; };
struct UIChatRoomTitle { struct UIWindow base; };
struct GameActor {
    struct UIWindow *chatroom_title_wnd;
    int balloon_font;
    int screen_x, screen_y;
};

struct UIWindowMgr g_windowMgr;
static struct UIChatRoomTitle title;
static intptr_t sent_val1, sent_val2;
static int created, listed, sent, moved, moved_x, moved_y;

struct UIChatRoomTitle *ui_chat_room_title_new(void)
{
    return &title;
}

void ui_window_create(struct UIWindow *self, int width, int height)
{
    assert(self == &title.base && width == 0x8c && height == 0x22);
    created++;
}

void ui_window_mgr_add_to_window_list(struct UIWindowMgr *manager, struct UIWindow *window)
{
    assert(manager == &g_windowMgr && window == &title.base);
    listed++;
}

static intptr_t ui_window_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1,
                                   intptr_t val2, intptr_t val3, intptr_t val4)
{
    assert(self == &title.base && sender == NULL && msg == 0x22 && val3 == 0 && val4 == 0);
    sent_val1 = val1;
    sent_val2 = val2;
    sent++;
    return 0;
}

static void ui_window_move(struct UIWindow *self, int x, int y)
{
    assert(self == &title.base);
    moved_x = x;
    moved_y = y;
    moved++;
}

int scale_y_from480(int n)
{
    return n;
}

#include "chat_room_title.inc"

int main(void)
{
    struct GameActor actor;
    /* A room address with bits above bit 31 on a 64-bit build; any address on a 32-bit one. */
    const uintptr_t room = (uintptr_t)UINT64_C(0x00001234569abcd0);
    int pass;

    memset(&actor, 0, sizeof actor);
    actor.balloon_font = 3;
    actor.screen_x = 400;
    actor.screen_y = 300;
    for (pass = 0; pass < 2; pass++) {
        sent_val1 = 0;
        actor_chat_room_message(&actor, (RenderMsgArg)(intptr_t)room);
        if ((uintptr_t)sent_val1 != room) {
            printf("chat room title: the room pointer reached the window as %#llx, not %#llx\n",
                   (unsigned long long)(uintptr_t)sent_val1, (unsigned long long)room);
            return 1;
        }
        assert(sent_val2 == 3 && moved_x == 400 - 0x46 && moved_y == 300 - 0x6e);
    }
    /* The title window is made once and then reused. */
    assert(created == 1 && listed == 1 && sent == 2 && moved == 2 && actor.chatroom_title_wnd == &title.base);
    printf("Chat room title: the room pointer reaches the title window whole (%d-bit pointers).\n",
           (int)(sizeof(void *) * 8));
    return 0;
}
