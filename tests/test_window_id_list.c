#include "session/state.h"
#include "render/renderer.h"
#include "ui/window_manager_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/wndmgr/make_window_internal.h"
#include "std/list.h"
#include "std/tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Run the production ui_window_mgr_make_window and ESC window-ID list helpers
   with controlled sub-factories: ESC closes the listed IDs one by one, so a
   request that leaves no window must not leave its ID behind. */
int g_replayPlaying;
struct Session g_session1;
struct Renderer *g_renderer;
static struct UIWindowMgr mgr;
static struct UIWindow windows[8];
static int created;

enum { NOT_MINE, NO_WINDOW, NEW_WINDOW };
static int factory_mode(int id)
{
    switch (id) {
    case 0x58: case 0x71: case 0x28: return NO_WINDOW; /* no pet/homunculus, refused */
    case 0xaa: case 0xcb: return NO_WINDOW;            /* "already open" requests */
    case 0x8: case 0xa: case 0x25: case 0xe: return NEW_WINDOW;
    }
    return NOT_MINE;
}

void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); return p; }
void mem_free(void *p) { free(p); }
int ui_window_mgr_is_dragging_member_plate(struct UIWindowMgr *self) { assert(self == &mgr); return 0; }
BYTE ui_window_mgr_is_hot_key_of_window(struct UIWindowMgr *self) { (void)self; return 0; }

static struct UIWindow *factory(struct UIWindowMgr *self, int window_id)
{
    assert(self == &mgr);
    switch (factory_mode(window_id)) {
    case NO_WINDOW: return NULL;
    case NEW_WINDOW:
        assert(created < (int)(sizeof windows / sizeof *windows));
        return wm_register(self, window_id, &windows[created++]);
    }
    return WM_NOT_MINE;
}
struct UIWindow *wm_make_character(struct UIWindowMgr *self, int window_id) { return factory(self, window_id); }
struct UIWindow *wm_make_hud(struct UIWindowMgr *self, int window_id) { (void)self; (void)window_id; return WM_NOT_MINE; }
struct UIWindow *wm_make_trade(struct UIWindowMgr *self, int window_id) { (void)self; (void)window_id; return WM_NOT_MINE; }
struct UIWindow *wm_make_social(struct UIWindowMgr *self, int window_id) { (void)self; (void)window_id; return WM_NOT_MINE; }

#include "window_id_list.inc"

static void expect_list(const int *ids, int count)
{
    struct ListNode *node = mgr.id_list.head->next;
    int i;

    assert((int)mgr.id_list.size == count);
    for (i = 0; i < count; i++, node = node->next)
        assert(*LIST_VALUE(node, int) == ids[i]);
    assert(node == mgr.id_list.head);
}

static void request(int id, int made)
{
    struct UIWindow *wnd = ui_window_mgr_make_window(&mgr, id);

    assert(made ? wnd != NULL && wnd->id == id : wnd == NULL);
}

int main(void)
{
    static const int none[1] = {0};
    static const int item[] = {0x8};
    static const int equip_item[] = {0xa, 0x8};
    static const int skill_equip_item[] = {0x25, 0xa, 0x8};
    static const int pet_equip_item[] = {0x58, 0xa, 0x8};

    list_init(&mgr.id_list);
    list_init(&mgr.plates);
    tree_init(&mgr.windows_by_id);

    request(0xd, 0);                /* no factory: the retired option window */
    expect_list(none, 0);
    request(0xec, 0);
    expect_list(none, 0);
    request(0x8, 1);
    expect_list(item, 1);
    request(0x58, 0);               /* claimed, but nothing to show */
    expect_list(item, 1);
    request(0x28, 0);
    expect_list(item, 1);
    request(0xcb, 0);               /* listed at the back, then dropped */
    expect_list(item, 1);
    request(0xa, 1);
    expect_list(equip_item, 2);
    request(0xe, 1);                /* untracked IDs never enter the list */
    expect_list(equip_item, 2);
    request(0x25, 1);
    expect_list(skill_equip_item, 3);

    /* "Already open": no new window, but one is registered under the ID. */
    wm_register(&mgr, 0xaa, &windows[created++]);
    request(0xaa, 0);
    assert(*LIST_VALUE(mgr.id_list.head->next, int) == 0xaa);
    ui_window_mgr_forget_window_id(&mgr, 0xaa);
    ui_window_mgr_forget_window_id(&mgr, 0x25);
    expect_list(equip_item, 2);

    /* A stale entry for an ID that again produces nothing is dropped too. */
    list_insert(&mgr.id_list, mgr.id_list.head->next, &(int){0x58}, sizeof(int));
    expect_list(pet_equip_item, 3);
    request(0x71, 0);
    expect_list(pet_equip_item, 3);
    request(0x58, 0);
    expect_list(equip_item, 2);

    puts("Window ID list: requests that leave no window are not listed for ESC; open windows stay listed.");
    return 0;
}
