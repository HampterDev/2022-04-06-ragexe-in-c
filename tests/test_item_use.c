#include "game/mode_msg.h"
#include "session/session_items.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "ui/items/refining_internal.h"
#include "ui/wndmgr/wndmgr.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct GameMode mode;
static struct World world;
static struct Player player;
static int item_id, inventory_index, effect_state, blocked;
static int sends, reads, releases, notices;
static unsigned char sent[8];
static int connection_token;

static int get_effect_state(struct RenderObject *object)
{
    assert(object == (struct RenderObject *)&player);
    return effect_state;
}
static const struct RenderObjectOps render_ops = { .get_effect_state = get_effect_state };

/* Other entry points in msg_items.c must never be reached by item use. */
struct ItemInfo *item_info_construct_with_arg(struct ItemInfo *self) { abort(); }
struct ItemInfo *item_info_assign(struct ItemInfo *self, const struct ItemInfo *src) { abort(); }
struct UIWindow *ui_window_find(int id) { abort(); }
struct UIWindow *ui_window_open(int id) { abort(); }
intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d) { abort(); }
intptr_t game_mode_send_msg(struct Mode *mode, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d) { abort(); }
int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type, int a, int b, int c,
                              const char *title, int width, int height, int d) { abort(); }
int *session_equip_slots_of(int *out, unsigned location) { abort(); }
struct ItemInfo *session_get_equipped_item(struct Session *self, struct ItemInfo *out, unsigned slot) { abort(); }
BYTE session_is_worn_index(struct Session *self, int index) { abort(); }
BYTE session_has_inventory_index(struct Session *self, int index) { abort(); }
struct ItemInfo *session_get_worn_costume(struct Session *self, struct ItemInfo *out, int slot) { abort(); }
void ui_grade_enchant_wnd_reset(struct UIGradeEnchantWnd *self) { abort(); }
void ui_refining_wnd_reset(struct UIRefiningWnd *self) { abort(); }
BYTE ui_window_mgr_show_option_window(struct UIWindowMgr *self, int window_id, int show) { abort(); }
void ui_window_mgr_close_drag_plate(struct UIWindowMgr *self) { abort(); }
BYTE ui_window_mgr_drop_on(struct UIWindowMgr *self, int x, int y, struct DragInfo *drag) { abort(); }

char game_mode_is_blocking_window_open(void) { return (char)blocked; }
struct ItemInfo *session_get_inventory_item(struct Session *session, struct ItemInfo *out, int index)
{
    assert(session == &g_session1 && index == inventory_index);
    ++reads;
    memset(out, 0, sizeof *out);
    out->index = index;
    out->count = 1;
    return out;
}
int item_info_get_item_id(struct ItemInfo *item) { assert(item->index == inventory_index); return item_id; }
void string_free(struct String *string) { (void)string; ++releases; }
char *msg_str(int id) { assert(id == 0x6a3 || id == 0x297); return "refused"; }
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)&connection_token; }
int rag_connection_get_packet_length(struct RagConnection *connection, int id)
{
    assert(connection == rag_connection_instance() && id == 0x439);
    return sizeof sent;
}
char rag_connection_send_packet(struct RagConnection *connection, int length, const void *packet)
{
    assert(connection == rag_connection_instance() && length == sizeof sent);
    assert(mode.use_item_pending == 1);
    memcpy(sent, packet, sizeof sent);
    ++sends;
    return 1;
}
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(mgr == &g_windowMgr && msg == UIM_CHAT_LINE);
    assert(!strcmp((const char *)a, "refused") && (unsigned)b == CHAT_COLOR_RED && !c && !d);
    ++notices;
    return 0;
}

static void reset(int id)
{
    memset(&mode, 0, sizeof mode);
    memset(&world, 0, sizeof world);
    memset(&player, 0, sizeof player);
    memset(&g_windowMgr, 0, sizeof g_windowMgr);
    mode.world = &world;
    world.player = &player;
    player.ops = &render_ops.base;
    g_session1.ownAid = 0x12345678;
    item_id = id;
    inventory_index = 0x1234;
    effect_state = blocked = sends = reads = releases = notices = 0;
    memset(sent, 0, sizeof sent);
}

static void use(int expected_sends, int expected_reads, int expected_notices)
{
    static const unsigned char packet[8] = {0x39, 0x04, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12};
    game_mode_msg_use_item(&mode, inventory_index);
    assert(sends == expected_sends && reads == expected_reads && notices == expected_notices);
    assert(releases == 2 * reads);
    if (expected_sends) assert(!memcmp(sent, packet, sizeof packet));
}

int main(void)
{
    static const int ordinary_and_box_ids[] = {501, 512, 101060, 101062, 1000000};
    static const int restricted_ids[] = {0x259, 0x25a, 0x263, 0x25d, 0x25e, 0x5813};
    unsigned i;
    for (i = 0; i < sizeof ordinary_and_box_ids / sizeof ordinary_and_box_ids[0]; ++i) {
        reset(ordinary_and_box_ids[i]); use(1, 1, 0);
        game_mode_msg_use_item(&mode, inventory_index);
        assert(sends == 1 && reads == 1 && releases == 2);
    }
    reset(501); g_windowMgr.exchange_wnd = (void *)&mode; use(0, 0, 1);
    reset(501); blocked = 1; use(0, 0, 0);
    reset(501); mode.use_item_pending = 1; use(0, 0, 0);
    reset(501); mode.throw_item_pending = 1; use(0, 0, 0);
    reset(501); mode.player_dead = 1; use(0, 0, 0);
    reset(501); inventory_index = 0; use(0, 0, 0);
    reset(501); effect_state = 2; use(0, 1, 0);
    reset(501); effect_state = 4; use(1, 1, 0);
    for (i = 1; i <= 4; i += 3) {
        reset(501); player.body_state = i; use(0, 1, 0);
        reset(0x31b5); player.body_state = i; use(1, 1, 0);
    }
    for (i = 0; i < sizeof restricted_ids / sizeof restricted_ids[0]; ++i) {
        reset(restricted_ids[i]); player.state = 6; use(0, 1, 1);
        reset(restricted_ids[i]); use(1, 1, 0);
    }
    reset(501); player.state = 6; use(1, 1, 0);
    puts("Item use: ordinary and former selection boxes send normal packets; trade, pending actions, death and status restrictions and cleanup passed.");
    return 0;
}
