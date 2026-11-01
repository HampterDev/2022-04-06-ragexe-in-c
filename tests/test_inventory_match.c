#include "session/state.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/std.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned live;
void *mem_alloc(size_t n) { void *p = malloc(n ? n : 1); assert(p); ++live; return p; }
void mem_free(void *p) { if (p) { assert(live); --live; free(p); } }
void fatal_error(const char *message) { (void)message; abort(); }

static void destroy_item(void *item) { item_info_destruct(item); }

static void expect(struct Session *session, const struct ItemInfo *wanted, int index)
{
    struct InventoryMatch match;
    session_find_inventory_item(session, &match, wanted);
    assert(match.found == (index != 0));
    if (index) assert(match.item.index == index);
    item_info_destruct(&match.item);
}

int main(void)
{
    struct Session *session = calloc(1, sizeof *session);
    struct ItemInfo wanted, candidate, *first, *second;
    int i;
    assert(session); list_init(&session->storeCandidateItems);
    item_info_construct(&wanted); item_info_set_item_id(&wanted, 501);
    wanted.identified = 1; wanted.refine = 7; wanted.slot[0] = 4001; wanted.option_count = 2;
    for (i = 0; i < 5; ++i) {
        wanted.options[i].id = (unsigned short)(100 + i);
        wanted.options[i].value = (short)(-5 + i);
        wanted.options[i].param = (unsigned char)i;
    }
    item_info_copy(&candidate, &wanted); candidate.index = 11; candidate.options[1].value = 99;
    first = list_push_back(&session->storeCandidateItems, NULL, sizeof *first); item_info_copy(first, &candidate);
    candidate.index = 12; candidate.options[1].value = wanted.options[1].value;
    second = list_push_back(&session->storeCandidateItems, NULL, sizeof *second); item_info_copy(second, &candidate);
    expect(session, &wanted, 12);
    second->options[0].param = 99; expect(session, &wanted, 0);
    second->options[0].param = wanted.options[0].param;
    wanted.option_count = second->option_count = 5; expect(session, &wanted, 12);
    second->options[4].id = 999; expect(session, &wanted, 0);
    second->options[4].id = wanted.options[4].id;
    wanted.option_count = second->option_count = 6; expect(session, &wanted, 0);
    wanted.option_count = second->option_count = -1; expect(session, &wanted, 0);
    wanted.option_count = second->option_count = 0; expect(session, &wanted, 12);
    second->slot[0] = 0; expect(session, &wanted, 0); second->slot[0] = wanted.slot[0];
    second->refine = 0; expect(session, &wanted, 0); second->refine = wanted.refine;
    second->identified = 0; expect(session, &wanted, 0); second->identified = wanted.identified;
    item_info_set_item_id(second, 502); expect(session, &wanted, 0);
    item_info_destruct(&candidate); item_info_destruct(&wanted);
    list_free(&session->storeCandidateItems, destroy_item); free(session);
    assert(!live);
    puts("Inventory matching: all option fields must match, five options supported, invalid counts rejected, item/card/refine/identification isolation and owned-copy cleanup passed");
    return 0;
}
