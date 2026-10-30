#include "actor/object_ops.h"
#include "game/gamemode.h"
#include "std/std.h"
#include "game/packets.h"
#include "ui/actor_labels.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *container_peer_value(struct ListNode *node);
struct TreeNode *container_peer_next(struct TreeNode *node);

static struct { void *p; size_t size; } blocks[256];
static int live, fail_next, copies;
static void *tracked_alloc(size_t size)
{
    int i;
    unsigned char *p;
    for (i = 0; i < 256 && blocks[i].p; ++i) {}
    assert(i < 256); p = malloc(size + 16); assert(p);
    memset(p + size, 0xa7, 16); blocks[i].p = p; blocks[i].size = size; ++live;
    return p;
}
static void tracked_free(void *p, size_t expected)
{
    int i, n;
    if (!p) return;
    for (i = 0; i < 256 && blocks[i].p != p; ++i) {}
    assert(i < 256 && (!expected || expected == blocks[i].size));
    for (n = 0; n < 16; ++n) assert(((unsigned char *)p)[blocks[i].size + n] == 0xa7);
    blocks[i].p = NULL; --live; free(p);
}
void *mem_alloc(size_t size) { void *p = tracked_alloc(size); assert(p); return p; }
void mem_free(void *p)
{ tracked_free(p, 0); }
void fatal_error(const char *s) { (void)s; abort(); }

static void *snapshot_alloc(size_t size)
{
    ++copies;
    if (fail_next) { fail_next = 0; return NULL; }
    return tracked_alloc(size);
}
static void snapshot_free(void *p) { tracked_free(p, 0); }
#define malloc snapshot_alloc
#define free snapshot_free
#include "../src/game/chat_room_enter.c"
#undef malloc
#undef free

static struct GameMode mode;
static struct World world;
static struct GameActor actor;
static struct ChatRoomInfo shown;
static struct PACKET_ZC_ENTER_ROOM *wire;
static size_t wire_size;
static int absent, hidden, actor_calls, opens, clears, infos, rows, window_missing;
static unsigned owner_id = 44;
static int window_marker;
static struct GameObjectOps actor_table;
static char names[3][25];
static int roles[3];

struct GameActor *world_get_game_actor_by_aid(struct World *w, unsigned aid)
{
    assert(w == &world && aid == owner_id); return absent ? NULL : &actor;
}
const struct ChatRoomInfo *ui_chat_room_title_info(const struct UIWindow *w)
{
    assert(w == actor.chatroom_title_wnd); return hidden ? NULL : &shown;
}
static void actor_message(struct GameObject *a, struct GameObject *sender, RenderMsgArg msg,
    RenderMsgArg p1, RenderMsgArg p2, RenderMsgArg p3, RenderMsgArg p4)
{
    assert((void *)a == &actor && !sender && msg == 0x21 && !p1 && !p2 && !p3 && !p4);
    ++actor_calls;
}
struct UIWindow *ui_window_open(int id)
{
    assert(id == 0x1c); ++opens;
    memset(wire, 0xdd, wire_size);
    return window_missing ? NULL : (struct UIWindow *)&window_marker;
}
int ui_window_send(struct UIWindow *w, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    assert(!sender && !c && !d);
    if (!w) return 0;
    assert(w == (struct UIWindow *)&window_marker);
    if (msg == 0x2b) { assert(!a && !b); ++clears; }
    else if (msg == 0x22) { assert(((struct ChatRoomInfo *)a)->users == 3 || ((struct ChatRoomInfo *)a)->users == 0); ++infos; }
    else {
        assert(msg == 0x2c && rows < 3);
        assert(!strcmp((const char *)a, names[rows]) && b == roles[rows]); ++rows;
    }
    return 0;
}

static void make_packet(size_t count)
{
    size_t i;
    free(wire); wire_size = offsetof(struct PACKET_ZC_ENTER_ROOM, members) + count * sizeof *wire->members;
    wire = calloc(1, wire_size); assert(wire);
    wire->id = 0xdb; wire->length = (short)wire_size; wire->room_id = 77;
    for (i = 0; i < count; ++i) {
        wire->members[i].role = roles[i]; memcpy(wire->members[i].name, names[i], 24);
    }
}

static void chat_tests(void)
{
    struct ChatRoomOwner room_owner;
    int baseline;
    strcpy(names[0], "한글"); memset(names[1], 'x', 24); names[1][24] = 0;
    memcpy(names[2], "a\0ignored", 9); roles[0] = 0; roles[1] = -3; roles[2] = 7;
    mode.world = &world; actor.ops = &actor_table; actor_table.send_msg = actor_message;
    actor.chatroom_title_wnd = (struct UIWindow *)&window_marker;
    string_init(&shown.title); string_init(&shown.password);
    string_assign_cstr(&shown.title, "room"); shown.room_id = 77; shown.max_users = 8; shown.type = 1;
    string_init(&mode.own_chat_room.title); string_init(&mode.own_chat_room.password);
    mode.own_chat_room.room_id = 12; mode.chat_room_pending = 1;
    room_owner.room_id = 77; room_owner.aid = owner_id; tree_init(&mode.chat_room_owners);
    tree_add(&mode.chat_room_owners, &room_owner, sizeof room_owner, tree_less_uint, NULL); baseline = live;
    make_packet(3); wire->length = 7; game_mode_zc_enter_room(&mode, wire);
    assert(mode.chat_room_pending == 1 && !actor_calls && !opens && !copies);
    make_packet(3); wire->length = -1; game_mode_zc_enter_room(&mode, wire);
    assert(mode.chat_room_pending == 1 && !actor_calls && !opens && !copies);
    make_packet(3); absent = 1; game_mode_zc_enter_room(&mode, wire); absent = 0;
    assert(!mode.chat_room_pending && !actor_calls && !opens);
    make_packet(3); hidden = 1; game_mode_zc_enter_room(&mode, wire); hidden = 0;
    assert(actor_calls == 1 && !opens && !copies);
    shown.room_id = 78; make_packet(3); game_mode_zc_enter_room(&mode, wire); shown.room_id = 77;
    assert(actor_calls == 2 && !opens && !copies);
    make_packet(3); fail_next = 1; game_mode_zc_enter_room(&mode, wire);
    assert(actor_calls == 3 && !opens && copies == 1 && live == baseline && mode.own_chat_room.room_id == 12);
    make_packet(3); game_mode_zc_enter_room(&mode, wire);
    assert(opens == 1 && clears == 1 && infos == 1 && rows == 3 && copies == 2 && live == baseline + 1);
    assert(mode.own_chat_room.room_id == 77 && mode.own_chat_room.users == 3 && mode.own_chat_room.max_users == 8);
    assert(!strcmp(string_cstr(&mode.own_chat_room.title), "room"));
    make_packet(0); game_mode_zc_enter_room(&mode, wire);
    assert(opens == 2 && clears == 2 && infos == 2 && rows == 3 && copies == 2 && !mode.own_chat_room.users);
    make_packet(3); window_missing = 1; game_mode_zc_enter_room(&mode, wire);
    assert(opens == 3 && rows == 3 && mode.own_chat_room.users == 3 && live == baseline + 1);
    free(wire); wire = NULL;
    string_free(&shown.title); string_free(&shown.password);
    string_free(&mode.own_chat_room.title); string_free(&mode.own_chat_room.password);
    assert(mode.chat_room_owners.size == 1); tree_free(&mode.chat_room_owners, NULL);
    assert(!live);
}

static void container_tests(void)
{
    struct List list = {0};
    struct Tree tree = {0};
    struct TreeNode *node;
    struct IntIntPair *pair;
    int values[127], i;
    list_init(&list);
    for (i = 0; i < 127; ++i) {
        void *value = &values[i]; values[i] = i;
        list_push_back(&list, &value, sizeof value);
    }
    for (i = 0; i < 127; ++i) {
        assert(container_peer_value(list.head->next) == &values[i]);
        list_erase(&list, list.head->next, NULL);
    }
    assert(!list.size && list.head->next == list.head && list.head->prev == list.head);
    list_free(&list, NULL); assert(!live);
    tree_init(&tree); assert(container_peer_next(tree.head) == tree.head);
    for (i = 0; i < 127; ++i) {
        int key = (i * 53) % 127 - 63;
        pair = tree_get(&tree, &key, sizeof key, sizeof *pair, tree_less_int);
        pair->value = key * 2;
    }
    assert(tree.size == 127); node = tree_first(&tree);
    for (i = -63; i <= 63; ++i) {
        pair = TREE_VALUE(node, struct IntIntPair);
        assert(node != tree.head && pair->key == i && pair->value == i * 2);
        node = container_peer_next(node);
    }
    assert(node == tree.head); tree_free(&tree, NULL); assert(!live);
}

int main(void)
{
    container_tests(); chat_tests();
    puts("Shared container headers: both include orders, cross-TU payloads/traversal, exact node frees; chat snapshot ownership, Korean names, callback reuse and allocation rollback passed");
    return 0;
}
