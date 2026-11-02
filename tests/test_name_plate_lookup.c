#include "ui/window_manager_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "replay/replay.h"
#include "std/list.h"
#include "std/tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Run the production window-ID lookup: window 0x12d is the first party name
   plate, and the cash shop is reachable only through its own ID 0x13e. */
int g_replayPlaying;
static struct UIWindowMgr mgr;
static struct WmWindowEntry registered;
static int tree_lookups, cases;

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    return p;
}

void mem_free(void *p)
{
    free(p);
}

int tree_less_int(const void *a, const void *b)
{
    return *(const int *)a < *(const int *)b;
}

void *tree_find(const struct Tree *tree, const void *key, TreeLessFn less)
{
    assert(tree == &mgr.windows_by_id && less == tree_less_int);
    ++tree_lookups;
    return registered.wnd && registered.id == *(const int *)key ? &registered : NULL;
}

#include "name_plate_lookup.inc"

static void expect(int id, struct UIWindow *expected)
{
    assert(ui_window_mgr_get_window(&mgr, id) == expected);
    ++cases;
}

int main(void)
{
    struct UIWindow cash_shop = {0}, first_plate = {0}, second_plate = {0}, other = {0};
    struct UIWindow *plate;

    list_init(&mgr.plates);
    expect(0x12d, NULL);
    expect(0x13e, NULL);

    mgr.cash_shop_wnd = &cash_shop;
    expect(0x12d, NULL);
    expect(0x13e, &cash_shop);

    plate = &first_plate;
    list_push_back(&mgr.plates, &plate, sizeof plate);
    plate = &second_plate;
    list_push_back(&mgr.plates, &plate, sizeof plate);
    expect(0x12d, &first_plate);
    expect(0x13e, &cash_shop);

    list_clear(&mgr.plates, NULL);
    expect(0x12d, NULL);

    tree_lookups = 0;
    registered.id = 0x154;
    registered.wnd = &other;
    expect(0x154, &other);
    assert(tree_lookups == 1);
    list_free(&mgr.plates, NULL);
    printf("Name plate lookup: %d cases passed (first plate or none; cash shop only as 0x13e).\n", cases);
    return 0;
}
