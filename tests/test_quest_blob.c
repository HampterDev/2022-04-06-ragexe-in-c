#include "contents/renew_quest.h"
#include "world_state_memory.h"
#include <limits.h>
#include <stdio.h>

static int fail_blob_allocation;
static unsigned blob_blocks;
static void *blob_allocate(size_t size)
{
    void *p;
    if (fail_blob_allocation) return NULL;
    p = malloc(size); assert(p); ++blob_blocks; return p;
}
static void blob_release(void *p)
{
    if (p) { assert(blob_blocks); --blob_blocks; free(p); }
}
#define malloc blob_allocate
#define free blob_release
#include "../src/contents/quest_blob.c"
#undef malloc
#undef free

static struct RenewQuestMgr manager;
static struct TreeNode head;
static struct {
    struct TreeNode links;
    struct OngoingQuestEntry entry;
} entries[8];
static unsigned stored, clears, refreshes;

static void relink(void)
{
    unsigned i;
    head.left = stored ? &entries[0].links : &head;
    for (i = 0; i < stored; ++i)
        entries[i].links.right = i + 1 < stored ? &entries[i + 1].links : &head;
    manager.quests_b.head = &head;
    manager.quests_b.size = stored;
}
struct TreeNode *tree_next(struct TreeNode *node) { return node->right; }
void renew_quest_clear_ongoing(struct RenewQuestMgr *quests)
{
    assert(quests == &manager);
    while (stored) quest_hunt_destroy(&entries[--stored].entry.quest);
    relink(); ++clears;
}
char renew_quest_set_active(struct RenewQuestMgr *quests, unsigned id, char active)
{
    unsigned i;
    assert(quests == &manager);
    for (i = 0; i < stored; ++i) if (entries[i].entry.id == id) {
        entries[i].entry.quest.base.active = active; return 1;
    }
    return 0;
}
void renew_quest_read_ongoing(struct RenewQuestMgr *quests, unsigned id, struct RenewQuestB *quest)
{
    assert(quests == &manager && stored < 8);
    quest->cool_time_quest = 23; quest->cool_time_quest_high = -1;
    string_assign_cstr(&quest->base.text[0], "Quest text from the catalog");
    entries[stored].entry.id = id;
    quest_hunt_copy(&entries[stored++].entry.quest, quest);
    relink();
}
void renew_quest_refresh_windows(void)
{ ++refreshes; }

static unsigned char input[4096];
static size_t used;
static void put16(unsigned value) { input[used++] = (unsigned char)value; input[used++] = (unsigned char)(value >> 8); }
static void put32(unsigned value) { put16(value); put16(value >> 16); }
static void header(unsigned tag, unsigned size) { put16(tag); put32(size); }
static void entry(unsigned id, int count, int names)
{
    int i;
    put32(id); input[used++] = 1; input[used++] = 0; put16((unsigned)count);
    put16(2030); put16(12); put16(31); put16(23); put16(59);
    memset(input + used, 0, 6); used += 6;
    for (i = 0; i < count; ++i) {
        put32(0xf0000000u + i); put32(10 + i); put32(1000 + i);
        put16(1 + i); put16(90 + i); put16(2 + i); put16(20 + i);
        memset(input + used, 'A' + i, names - 1); input[used + names - 1] = 0; used += names;
    }
}
static void fixture(int names, int hunts, int duplicate)
{
    used = 0; header(0xdac, 0);
    if (names != 24) { header(0xdae, 4); put32(names); }
    header(0xdb6, 4 + (duplicate ? 3 : 2) * (24 + hunts * (20 + names)));
    put32(duplicate ? 3 : 2);
    entry(123, hunts, names);
    if (duplicate) entry(123, hunts, names);
    entry(456, hunts, names);
    header(0xdad, 0);
}

static void unchanged_on_bad_input(const void *data, size_t size)
{
    struct RenewQuestB before[8];
    unsigned n = stored, c = clears, r = refreshes, i;
    for (i = 0; i < stored; ++i) before[i] = entries[i].entry.quest;
    assert(quest_blob_load(&manager, data, size) == -1);
    assert(n == stored && c == clears && r == refreshes);
    for (i = 0; i < stored; ++i) assert(!memcmp(&before[i], &entries[i].entry.quest, sizeof before[i]));
}

int main(void)
{
    struct QuestBlob out = {0};
    unsigned char valid[4096], *old;
    size_t size, i, list, first;
    int names, hunts;
    assert(TREE_VALUE(&entries[0].links, struct OngoingQuestEntry) == &entries[0].entry);
    relink();
    for (names = 24; names <= 51; names += 27) for (hunts = 0; hunts <= 3; ++hunts) {
        fixture(names, hunts, 0);
        size = used; memcpy(valid, input, size);
        assert(quest_blob_load(&manager, input, size) == (int)size);
        assert(stored == 2 && entries[1].entry.id == 456);
        assert(entries[0].entry.quest.cool_time_quest == 23 && entries[0].entry.quest.cool_time_quest_high == -1);
        assert(entries[0].entry.quest.plain.hunt_count == hunts);
        if (hunts) {
            assert(entries[0].entry.quest.plain.hunt_id[0] == (int)0xf0000000u);
            assert(strlen(entries[0].entry.quest.plain.hunt_name[0]) == (size_t)names - 1);
        }
        assert(quest_blob_save(&manager, &out));
        if (names == 51) assert(out.size == size && !memcmp(out.data, valid, size));
        for (i = 0; i < size; ++i) unchanged_on_bad_input(input, i);
        list = names == 24 ? 12 : 22; first = list + 4;
        for (i = 0; i < 4; ++i) input[list + i] = 0xff;
        unchanged_on_bad_input(input, size); memcpy(input, valid, size);
        input[list + 3] = 0x7f; unchanged_on_bad_input(input, size); memcpy(input, valid, size);
        input[first + 6] = 4; unchanged_on_bad_input(input, size); memcpy(input, valid, size);
        input[first + 6] = input[first + 7] = 0xff;
        unchanged_on_bad_input(input, size); memcpy(input, valid, size);
        input[list - 4]--; unchanged_on_bad_input(input, size); memcpy(input, valid, size);
        input[2] = 1; unchanged_on_bad_input(input, size); memcpy(input, valid, size);
        if (names == 51) {
            input[12] = 50; unchanged_on_bad_input(input, size); memcpy(input, valid, size);
            input[8] = 5; unchanged_on_bad_input(input, size); memcpy(input, valid, size);
        }
    }
    fixture(51, 3, 1);
    assert(quest_blob_load(&manager, input, used) == (int)used);
    assert(stored == 2 && entries[1].entry.id == 456 && entries[1].entry.quest.plain.hunt_goal[2] == 22);
    for (names = 24; names <= 51; names += 27) {
        fixture(names, 1, 0); first = (names == 24 ? 16 : 26);
        memset(input + first + 24 + 20, 'X', names);
        assert(quest_blob_load(&manager, input, used) == (int)used);
        assert(strlen(entries[0].entry.quest.plain.hunt_name[0]) == (size_t)(names == 24 ? 24 : 50));
    }
    used = 0; header(0xdac, 0); header(0x1234, 3);
    input[used++] = 1; input[used++] = 2; input[used++] = 3; header(0xdad, 0);
    size = used; input[used++] = 0xff;
    assert(quest_blob_load(&manager, input, used) == (int)size && !stored);
    unchanged_on_bad_input(NULL, 0); unchanged_on_bad_input(input, (size_t)INT_MAX + 1);
    fixture(51, 3, 0); assert(quest_blob_load(&manager, input, used) == (int)used);
    assert(quest_blob_save(&manager, &out)); old = out.data; size = out.size;
    fail_blob_allocation = 1; assert(!quest_blob_save(&manager, &out)); fail_blob_allocation = 0;
    assert(out.data == old && out.size == size);
    entries[0].entry.quest.plain.hunt_count = -1; assert(!quest_blob_save(&manager, &out));
    entries[0].entry.quest.plain.hunt_count = 4; assert(!quest_blob_save(&manager, &out));
    assert(out.data == old && out.size == size);
    entries[0].entry.quest.plain.hunt_count = 3;
    quest_blob_destroy(&out); quest_blob_destroy(&out); quest_blob_destroy(NULL);
    renew_quest_clear_ongoing(&manager);
    assert(!live_blocks && !blob_blocks);
    puts("Quest blobs: byte-exact saves, both name widths, every-byte truncation, invalid counts/spans, duplicate quests, bounded names and atomic output ownership passed");
    return 0;
}
