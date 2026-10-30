#include "session/state.h"
#include "session/snapshot_items.h"
#include "iteminfo/iteminfo.h"
#include "iteminfo/item_snapshot.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fixtures/item_snapshot_wire.h"

static unsigned allocations, attempts, fail_at;
void *mem_alloc(size_t size)
{
    void *p = malloc(size ? size : 1);
    assert(p); ++allocations; return p;
}
void mem_free(void *p)
{
    if (p) { assert(allocations); --allocations; free(p); }
}
void fatal_error(const char *message) { (void)message; abort(); }
static void *resize(void *old, size_t size)
{
    void *p;
    if (++attempts == fail_at) return NULL;
    p = realloc(old, size); assert(p);
    if (!old) ++allocations;
    return p;
}
static void release(void *p) { mem_free(p); }
#define realloc resize
#define free release
#include "../src/replay/snapshot.c"
#undef realloc
#undef free

static void get_lists(struct Session *s, struct List *lists[13])
{
    lists[0] = &s->itemList; lists[1] = &s->shopItems; lists[2] = &s->purchaseItems;
    lists[3] = &s->snapshotOnlyItems; lists[4] = &s->sellItems; lists[5] = &s->storageItems;
    lists[6] = &s->cartItems; lists[7] = &s->otherStoreItems; lists[8] = &s->storeCandidateItems;
    lists[9] = &s->ownStoreItems; lists[10] = &s->storePurchaseItems;
    lists[11] = &s->storeSetupItems; lists[12] = &s->storePurchaseSent;
}
static void get_arrays(struct Session *s, struct ItemInfo *arrays[6])
{
    arrays[0] = s->equipSlots; arrays[1] = s->viewedEquips; arrays[2] = s->costumeSlots;
    arrays[3] = s->viewedCostumes; arrays[4] = s->exchangeA; arrays[5] = s->partnerTradeItems;
}
static void fill_item(struct ItemInfo *item, int index)
{
    item->type = 5; item->index = index; item->equip_location = 0x200;
    item->count = 3; item->price = 12345; item->charged_price = 12000; item->slot[0] = 501;
    item->identified = 1; item->refine = 7; item->bind_on_equip_type = 258;
    item->expire_time = 123456; item->view = 42;
    item_info_set_item_id(item, 501);
    string_assign(&item->name, "긴 이름의 검증용 장비 아이템", 28);
}
static void initialize(struct Session *s, int filled)
{
    struct List *lists[13];
    struct ItemInfo *arrays[6];
    unsigned i, j;
    memset(s, 0, sizeof *s);
    get_lists(s, lists); get_arrays(s, arrays);
    for (i = 0; i < 13; ++i) {
        struct List *list = lists[i];
        list_init(list);
        if (filled) for (j = 0; j < 2; ++j) {
            struct ItemInfo item = {0};
            item_info_construct(&item);
            fill_item(&item, 100 + (int)(i * 2 + j));
            item_info_copy(list_push_back(list, NULL, sizeof(struct ItemInfo)), &item);
            item_info_destruct(&item);
        }
    }
    for (i = 0; i < 6; ++i) for (j = 0; j < 10; ++j) {
        item_info_construct(&arrays[i][j]);
        if (filled) fill_item(&arrays[i][j], 1000 + (int)(i * 10 + j));
    }
    if (filled) { s->myTradeZeny = 4321; s->partnerTradeZeny = 9876; }
}
static void destroy(struct Session *s)
{
    struct List *lists[13];
    struct ItemInfo *arrays[6];
    unsigned i, j;
    get_lists(s, lists); get_arrays(s, arrays);
    for (i = 0; i < 13; ++i) list_free(lists[i], item_info_destroy);
    for (i = 0; i < 6; ++i) for (j = 0; j < 10; ++j) item_info_destruct(&arrays[i][j]);
}

static unsigned char expected[30000];
static size_t expected_size;
static void little32(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)value; p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16); p[3] = (unsigned char)(value >> 24);
}
static void header(unsigned tag, unsigned size)
{
    assert(expected_size + 6 <= sizeof expected);
    expected[expected_size] = (unsigned char)tag;
    expected[expected_size + 1] = (unsigned char)(tag >> 8);
    little32(expected + expected_size + 2, size); expected_size += 6;
}
static void expected_item(unsigned index)
{
    assert(expected_size + sizeof expected_tlv <= sizeof expected);
    memcpy(expected + expected_size, expected_tlv, sizeof expected_tlv);
    little32(expected + expected_size + 22, index);
    expected_size += sizeof expected_tlv;
}
static void make_fixture(void)
{
    unsigned i, j;
    expected_size = 0;
    header(0x1194, 0);
    header(0x125d, 4); little32(expected + expected_size, 4321); expected_size += 4;
    header(0x125e, 4); little32(expected + expected_size, 9876); expected_size += 4;
    for (i = 0; i < 13; ++i) {
        header(0x119e + i, 2 * sizeof expected_tlv);
        for (j = 0; j < 2; ++j) expected_item(100 + i * 2 + j);
    }
    for (i = 0; i < 6; ++i) {
        header(0x11f9 + i, 10 * sizeof expected_tlv);
        for (j = 0; j < 10; ++j) expected_item(1000 + i * 10 + j);
    }
    header(0x1195, 0);
}
static void legacy_bytes_and_bounds(void)
{
    struct Session source, target, before;
    struct ReplayBuffer out = {0};
    struct List *lists[13];
    size_t i;
    unsigned original;
    initialize(&source, 1); initialize(&target, 0); make_fixture();
    assert(session_items_save(&source, &out));
    assert(out.size == expected_size && !memcmp(out.data, expected, expected_size));
    before = target; original = allocations;
    for (i = 0; i < expected_size; ++i) {
        assert(session_items_load(&target, expected, i) == -1);
        assert(!memcmp(&target, &before, sizeof target) && allocations == original);
    }
    expected[expected_size - 6 - sizeof expected_tlv + 8] = 5;
    assert(session_items_load(&target, expected, expected_size) == -1);
    assert(!memcmp(&target, &before, sizeof target) && allocations == original);
    expected[expected_size - 6 - sizeof expected_tlv + 8] = 4;
    assert(session_items_load(&target, expected, expected_size) == (int)expected_size);
    assert(session_items_save(&target, &out));
    assert(out.size == expected_size && !memcmp(out.data, expected, expected_size));
    assert(target.myTradeZeny == 4321 && target.partnerTradeZeny == 9876);
    assert(session_items_load(&target, expected, expected_size) == (int)expected_size);
    get_lists(&target, lists);
    for (i = 0; i < 13; ++i) {
        struct ListNode *node = lists[i]->head->next;
        assert(lists[i]->size == 4);
        assert(LIST_VALUE(node, struct ItemInfo)->index == 100 + (int)i * 2);
        assert(LIST_VALUE(node->next, struct ItemInfo)->index == 101 + (int)i * 2);
    }
    replay_buffer_destroy(&out); destroy(&target); destroy(&source); assert(!allocations);
}

static void malformed_items(void)
{
    struct ItemInfo item = {0}, before;
    struct SnapshotReader reader = {{expected_tlv, sizeof expected_tlv}, 0};
    struct SnapshotRecord record;
    unsigned char bad[sizeof expected_tlv + 1];
    size_t i;
    unsigned original;
    item_info_construct(&item); fill_item(&item, 99); before = item; original = allocations;
    for (i = 0; i < sizeof expected_tlv; ++i) {
        assert(item_snapshot_load(&item, expected_tlv, i) == -1);
        assert(!memcmp(&item, &before, sizeof item) && allocations == original);
    }
    while (snapshot_next(&reader, &record) == 1) {
        size_t start = reader.offset - record.value.size - 6;
        if (record.tag == 0x124) continue;
        memcpy(bad, expected_tlv, reader.offset);
        little32(bad + start + 2, (unsigned)record.value.size + 1);
        bad[reader.offset] = 0;
        memcpy(bad + reader.offset + 1, expected_tlv + reader.offset, sizeof expected_tlv - reader.offset);
        assert(item_snapshot_load(&item, bad, sizeof bad) == -1);
        assert(!memcmp(&item, &before, sizeof item) && allocations == original);
    }
    item_info_destruct(&item); assert(!allocations);
}

static void sparse_and_capacity(void)
{
    struct Session target, before;
    struct SnapshotWriter writer = {0};
    struct ReplayBuffer out = {0};
    unsigned i, original;
    size_t start;
    int value = 41;
    char large[700];
    initialize(&target, 0);
    snapshot_write(&writer, 0x1194, NULL, 0);
    start = snapshot_begin_record(&writer, 0x11f9);
    snapshot_write(&writer, 0x119, NULL, 0);
    snapshot_write(&writer, 0x11b, &value, 4);
    snapshot_write(&writer, 0x11a, NULL, 0);
    snapshot_write(&writer, 0x119, NULL, 0);
    snapshot_write(&writer, 0x11a, NULL, 0);
    snapshot_end_record(&writer, start);
    snapshot_write(&writer, 0x7777, "ignored", 7);
    snapshot_write(&writer, 0x125f, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    target.equipSlots[2].type = 99;
    assert(session_items_load(&target, out.data, out.size) == (int)out.size);
    assert(target.equipSlots[0].type == 41 && target.equipSlots[1].type == 0 && target.equipSlots[2].type == 99);
    before = target; original = allocations;
    snapshot_write(&writer, 0x1194, NULL, 0);
    start = snapshot_begin_record(&writer, 0x11f9);
    for (i = 0; i < 11; ++i) {
        snapshot_write(&writer, 0x119, NULL, 0);
        snapshot_write(&writer, 0x11a, NULL, 0);
    }
    snapshot_end_record(&writer, start);
    snapshot_write(&writer, 0x1195, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    assert(session_items_load(&target, out.data, out.size) == -1);
    assert(!memcmp(&target, &before, sizeof target) && allocations == original);
    target.itemList.size = 0x1111110;
    assert(session_items_load(&target, expected, expected_size) == -1);
    target.itemList.size = 0;
    memset(large, 'x', sizeof large);
    snapshot_write(&writer, 0x119, NULL, 0);
    snapshot_write(&writer, 0x124, large, sizeof large);
    value = -2147483647 - 1;
    snapshot_write(&writer, 0x123, &value, 4);
    snapshot_write(&writer, 0x11a, NULL, 0);
    snapshot_write(&writer, 0x7777, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    assert(item_snapshot_load(&target.equipSlots[0], out.data, out.size) == (int)out.size - 6);
    assert(target.equipSlots[0].name.size == sizeof large);
    assert(!memcmp(string_cstr(&target.equipSlots[0].name), large, sizeof large));
    assert(!strcmp(string_cstr(&target.equipSlots[0].id_text), "-2147483648"));
    out.data[6 + 6 + 3] = 0;
    assert(item_snapshot_load(&target.equipSlots[0], out.data, out.size) == (int)out.size - 6);
    assert(target.equipSlots[0].name.size == 3);
    replay_buffer_destroy(&out); destroy(&target); assert(!allocations);
}

static void allocation_failures(void)
{
    struct Session source;
    unsigned failure, original;
    initialize(&source, 1); original = allocations;
    for (failure = 1; failure <= 10; ++failure) {
        struct ReplayBuffer out = {0};
        unsigned char *old;
        fail_at = 0; out.data = resize(NULL, 3); out.size = 3; memcpy(out.data, "old", 3); old = out.data;
        attempts = 0; fail_at = failure;
        if (session_items_save(&source, &out)) {
            assert(failure > attempts);
            assert(out.size == expected_size && !memcmp(out.data, expected, expected_size));
        } else {
            assert(failure == attempts && out.data == old && out.size == 3 && !memcmp(out.data, "old", 3));
        }
        assert(allocations == original + 1);
        replay_buffer_destroy(&out); assert(allocations == original);
    }
    fail_at = 0; destroy(&source); assert(!allocations);
}

int main(void)
{
    legacy_bytes_and_bounds(); malformed_items(); sparse_and_capacity(); allocation_failures();
    puts("Session items: legacy bytes, thirteen lists, six arrays, nested truncation/widths, sparse records, long names and allocation failures passed");
    return 0;
}
