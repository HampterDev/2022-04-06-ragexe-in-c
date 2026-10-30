#include "session/state.h"
#include "session/party_types.h"
#include "session/snapshot_party.h"
#include "std/std.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fixtures/party_snapshot_wire.h"

static unsigned allocations, attempts, fail_at;
void *mem_alloc(size_t size)
{
    void *p = malloc(size ? size : 1); assert(p); ++allocations; return p;
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

static struct List *party_list(struct Session *s, unsigned i)
{ return i ? &s->friendList : &s->partyMembers; }
static struct PartyMember *first_member(struct Session *s, unsigned i)
{ return LIST_VALUE(party_list(s, i)->head->next, struct PartyMember); }
static void initialize(struct Session *s, int filled)
{
    unsigned i;
    memset(s, 0, sizeof *s);
    string_init(&s->newPartyName); string_init(&s->partyName);
    if (filled) {
        s->partyInviteRefused = 1; s->map_rule_flags[4] = 7; s->partyLeader = 8;
        string_assign_cstr(&s->newPartyName, "new party");
        string_assign_cstr(&s->partyName, "party");
    }
    for (i = 0; i < 2; ++i) {
        struct List *list = party_list(s, i);
        list->head = mem_alloc(sizeof(struct ListNode) + sizeof(struct PartyMember));
        list->head->next = list->head->prev = list->head;
        if (filled) {
            struct ListNode *node = mem_alloc(sizeof *node + sizeof(struct PartyMember));
            struct PartyMember *member = LIST_VALUE(node, struct PartyMember);
            const char *name = i ? "second" : "first", *map = i ? "geffen.rsw" : "prontera.rsw";
            memset(member, 0, sizeof *member);
            member->found = 1; member->aid = 100 + i; member->char_id = 200 + i;
            member->role = 3; member->state = 4; member->color = 0x11223344;
            string_init(&member->name); string_init(&member->map_name);
            string_assign_cstr(&member->name, name);
            string_assign_cstr(&member->map_name, map);
            node->next = node->prev = list->head;
            list->head->next = list->head->prev = node; list->size = 1;
        }
    }
}
static void destroy(struct Session *s)
{
    unsigned i;
    for (i = 0; i < 2; ++i) {
        struct List *list = party_list(s, i);
        struct ListNode *node = list->head->next;
        while (node != list->head) {
            struct ListNode *next = node->next;
            struct PartyMember *member = LIST_VALUE(node, struct PartyMember);
            string_free(&member->name); string_free(&member->map_name);
            mem_free(node); node = next;
        }
        mem_free(list->head);
    }
    string_free(&s->newPartyName); string_free(&s->partyName);
}

static void frozen_bytes_and_malformed(void)
{
    struct Session source, target, before;
    struct ReplayBuffer out = {0};
    size_t i;
    unsigned original;
    unsigned char bad[sizeof expected_party];
    initialize(&source, 1); initialize(&target, 0);
    assert(session_party_save(&source, &out));
    assert(out.size == sizeof expected_party && !memcmp(out.data, expected_party, out.size));
    before = target; original = allocations;
    for (i = 0; i < sizeof expected_party; ++i) {
        assert(session_party_load(&target, expected_party, i) == -1);
        assert(!memcmp(&target, &before, sizeof target) && allocations == original);
    }
    memcpy(bad, expected_party, sizeof bad);
    bad[8] = 4;
    assert(session_party_load(&target, bad, sizeof bad) == -1);
    assert(!memcmp(&target, &before, sizeof target) && allocations == original);
    {
        struct SnapshotReader reader = {{expected_party, sizeof expected_party}, 0};
        struct SnapshotRecord record;
        while (snapshot_next(&reader, &record) == 1) {
            if (record.tag == 0xfb0) {
                size_t nested = (size_t)(record.value.data - expected_party);
                memcpy(bad, expected_party, sizeof bad);
                bad[nested + 8] = 5;
                assert(session_party_load(&target, bad, sizeof bad) == -1);
                assert(!memcmp(&target, &before, sizeof target) && allocations == original);
            }
        }
    }
    assert(session_party_load(&target, expected_party, sizeof expected_party) == sizeof expected_party);
    assert(session_party_save(&target, &out));
    assert(out.size == sizeof expected_party && !memcmp(out.data, expected_party, out.size));
    memcpy(bad, expected_party, sizeof bad);
    bad[sizeof bad - 6] = 0x95; bad[sizeof bad - 5] = 0x11;
    assert(session_party_load(&target, bad, sizeof bad) == sizeof bad);
    assert(target.partyMembers.size == 2 && target.friendList.size == 2);
    assert(first_member(&target, 0)->aid == 100 && first_member(&target, 1)->aid == 101);
    replay_buffer_destroy(&out); destroy(&target); destroy(&source); assert(!allocations);
}

static void strings_sparse_and_capacity(void)
{
    struct Session target;
    struct SnapshotWriter writer = {0};
    struct ReplayBuffer out = {0};
    char large[700];
    struct PartyMember *member;
    size_t start;
    initialize(&target, 0); memset(large, 'x', sizeof large);
    snapshot_write(&writer, 0xfa0, NULL, 0);
    snapshot_write(&writer, 0xfad, large, sizeof large);
    start = snapshot_begin_record(&writer, 0xfaf);
    snapshot_write(&writer, 0xfb, NULL, 0);
    snapshot_write(&writer, 0x100, large, sizeof large);
    snapshot_write(&writer, 0x7777, "unknown", 7);
    large[3] = 0;
    snapshot_write(&writer, 0x101, large, sizeof large);
    snapshot_write(&writer, 0xfc, NULL, 0);
    snapshot_end_record(&writer, start);
    snapshot_write(&writer, 0xfa1, NULL, 0);
    snapshot_write(&writer, 0x7777, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    assert(session_party_load(&target, out.data, out.size) == (int)out.size - 6);
    member = first_member(&target, 0);
    assert(member->name.size == 700 && member->map_name.size == 3 && target.newPartyName.size == 700);
    assert(member->found == 1 && !member->aid && !member->char_id && !member->state && !member->job && !member->level);
    replay_buffer_destroy(&out); destroy(&target); assert(!allocations);
}
static void allocation_failures(void)
{
    struct Session source;
    unsigned failure, original;
    initialize(&source, 1); original = allocations;
    for (failure = 1; failure <= 4; ++failure) {
        struct ReplayBuffer out = {0};
        unsigned char *old;
        fail_at = 0; out.data = resize(NULL, 3); out.size = 3; memcpy(out.data, "old", 3); old = out.data;
        attempts = 0; fail_at = failure;
        if (session_party_save(&source, &out)) {
            assert(failure > attempts);
            assert(out.size == sizeof expected_party && !memcmp(out.data, expected_party, out.size));
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
    frozen_bytes_and_malformed(); strings_sparse_and_capacity(); allocation_failures();
    puts("Session party: frozen bytes, both closing tags, nested validation, bounded names, defaults, append ownership and allocation failures passed");
    return 0;
}
