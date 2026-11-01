#include "contents/agency_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char sent[256], reply[2048];
static size_t sent_size;
static uint32_t tick;
static int open = 1, accepts = 1, presentations, last_op, last_status, live, attempts, fail_at;

static void *checked_malloc(size_t size)
{
    void *p;
    if (++attempts == fail_at) return NULL;
    p = malloc(size); if (p) ++live;
    return p;
}
static void *checked_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void checked_free(void *p) { if (p) { --live; free(p); } }
#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/contents/party_booking_data.c"
#include "../src/contents/adventurer_agency.c"
#undef malloc
#undef calloc
#undef free

int agency_transport_open(void) { return open; }
int agency_transport_send(const void *p, size_t size)
{ assert(size <= sizeof sent); memcpy(sent, p, size); sent_size = size; return accepts; }
uint32_t agency_clock(void) { return tick; }
void agency_present(struct AdventurerAgencyMgr *self, int op, int status, const struct PartyBookingList *data)
{ (void)self; (void)data; ++presentations; last_op = op; last_status = status; }

static size_t answer(int op, int status, unsigned count)
{
    unsigned i;
    size_t size = 16;
    memset(reply, 0, sizeof reply);
    write16(reply, AGENCY_REPLY); reply[4] = 1; reply[5] = (unsigned char)op;
    write32(reply + 6, g_adventurerAgency->pending[op].serial);
    reply[10] = (unsigned char)status; reply[11] = (unsigned char)count;
    write16(reply + 12, 1); write16(reply + 14, 1);
    for (i = 0; i < count; ++i) {
        write32(reply + size, 100 + i); write32(reply + size + 4, 200 + i);
        write16(reply + size + 8, 30); write16(reply + size + 10, 90);
        reply[size + 12] = 5; reply[size + 13] = 2;
        reply[size + 14] = 3; reply[size + 15] = 4;
        memcpy(reply + size + 16, "BobHunt", 7); size += 23;
    }
    write16(reply + 2, (unsigned)size);
    return size;
}

int main(int argc, char **argv)
{
    struct AdventurerAgencyMgr *m;
    struct AdvenPartyInfo ad;
    size_t size, cut;
    int before, baseline, allocation;
    uint32_t serial;
    fail_at = 1;
    assert(!adventurer_agency_create() && !live);
    fail_at = 0;
    assert(adventurer_agency_create() && !adventurer_agency_create());
    m = g_adventurerAgency;
    party_booking_init(&ad); ad.min_level = 30; ad.max_level = 90; ad.purpose = 2;
    ad.roles[0] = ad.roles[2] = 1;
    assert(owned_text_assign(&ad.memo, text_view_cstr("Hunt")));
    assert(adventurer_agency_register(m, &ad));
    assert(sent_size == 28 && read16(sent) == 0x0bf0 && read16(sent + 2) == 28);
    assert(sent[4] == 1 && sent[5] == 4 && read32(sent + 12) == 0);
    assert(read16(sent + 16) == 30 && read16(sent + 18) == 90 && sent[20] == 5 && sent[21] == 2);
    assert(read16(sent + 22) == 4 && !memcmp(sent + 24, "Hunt", 4));
    ad.memo.data[0] = 'X'; assert(sent[24] == 'H');
    assert(!adventurer_agency_register(m, &ad));
    size = answer(AGENCY_REGISTER, 0, 0); adventurer_agency_receive(m, reply, size);
    assert(m->recruiting);
    adventurer_agency_cancel_own_booking(m); assert(sent[5] == AGENCY_DELETE && !read32(sent + 12));
    size = answer(AGENCY_DELETE, 0, 0); adventurer_agency_receive(m, reply, size); assert(!m->recruiting);
    assert(adventurer_agency_search(m, &ad, 5)); assert(read16(sent + 10) == 5);
    size = answer(AGENCY_LIST, 0, 10); adventurer_agency_receive(m, reply, size);
    assert(m->parties.count == 10 && !strcmp(m->parties.items[0].name.data, "Bob"));
    assert(!strcmp(m->parties.items[9].memo.data, "Hunt") && m->parties.items[9].aid == 109);
    /* A newer query supersedes an older one; duplicate replies have no effects. */
    assert(adventurer_agency_load_list(m, 2)); size = answer(AGENCY_LIST, 0, 0);
    assert(adventurer_agency_load_list(m, 3)); before = presentations;
    adventurer_agency_receive(m, reply, size); assert(presentations == before && m->parties.count == 10);
    size = answer(AGENCY_LIST, 0, 0); adventurer_agency_receive(m, reply, size);
    assert(!m->parties.count && m->page == 1 && m->pages == 1);
    before = presentations; adventurer_agency_receive(m, reply, size); assert(presentations == before);
    /* Every truncated prefix preserves the old list and releases any partial decode. */
    assert(adventurer_agency_load_list(m, 1)); size = answer(AGENCY_LIST, 0, 1);
    adventurer_agency_receive(m, reply, size); baseline = live;
    for (cut = 0; cut < size; ++cut) {
        assert(adventurer_agency_load_list(m, 1)); size = answer(AGENCY_LIST, 0, 1);
        write16(reply + 2, (unsigned)cut);
        adventurer_agency_receive(m, reply, cut);
        assert(m->parties.count == 1 && live == baseline);
    }
    for (allocation = 1; allocation <= 3; ++allocation) {
        assert(adventurer_agency_load_list(m, 1)); size = answer(AGENCY_LIST, 0, 1);
        fail_at = attempts + allocation;
        adventurer_agency_receive(m, reply, size); fail_at = 0;
        assert(m->parties.count == 1 && live == baseline && last_status == AGENCY_UNAVAILABLE);
    }
    /* Bad ranges, embedded NUL, wrong target and cancellation are all rejected. */
    assert(adventurer_agency_load_list(m, 1)); size = answer(AGENCY_LIST, 0, 1);
    write16(reply + 24, 1000); adventurer_agency_receive(m, reply, size); assert(last_status == AGENCY_UNAVAILABLE);
    assert(adventurer_agency_load_list(m, 1)); size = answer(AGENCY_LIST, 0, 1);
    reply[32] = 0; adventurer_agency_receive(m, reply, size); assert(last_status == AGENCY_UNAVAILABLE);
    adventurer_agency_load_party_info(m, 999); size = answer(AGENCY_DETAIL, 0, 1);
    adventurer_agency_receive(m, reply, size); assert(last_status == AGENCY_UNAVAILABLE);
    adventurer_agency_load_party_info(m, 100); size = answer(AGENCY_DETAIL, 0, 1);
    adventurer_agency_cancel_detail(m); before = presentations;
    adventurer_agency_receive(m, reply, size); assert(presentations == before);
    tick = UINT32_MAX - 5000; assert(adventurer_agency_load_list(m, 1));
    tick += 10000; adventurer_agency_tick(m); assert(last_op == AGENCY_LIST && last_status == AGENCY_UNAVAILABLE);
    accepts = 0; serial = m->pending[AGENCY_LIST].serial;
    assert(!adventurer_agency_load_list(m, 1) && m->pending[AGENCY_LIST].serial == serial); accepts = 1;
    assert(adventurer_agency_load_list(m, 1)); size = answer(AGENCY_LIST, 0, 1);
    adventurer_agency_reset(m); assert(adventurer_agency_load_list(m, 1)); before = presentations;
    adventurer_agency_receive(m, reply, size); assert(presentations == before);
    open = 0; adventurer_agency_tick(m); assert(!m->pending[AGENCY_LIST].serial && !m->parties.count);
    assert(!adventurer_agency_load_list(m, 1)); open = 1;
    ad.max_level = 29; assert(!adventurer_agency_search(m, &ad, 1)); ad.max_level = 90;
    assert(!adventurer_agency_load_list(m, 0) && !adventurer_agency_load_list(m, 65536));
    /* The server harness emits a real response from the patched Hercules handler. */
    if (argc == 2) {
        FILE *f = fopen(argv[1], "rb"); assert(f);
        size = fread(reply, 1, sizeof reply, f); assert(!ferror(f)); fclose(f);
        m->serial = 40; assert(adventurer_agency_load_list(m, 1));
        adventurer_agency_receive(m, reply, size);
        assert(m->parties.count == 10 && m->pages == 2 && m->parties.items[0].aid == 100);
    }
    party_booking_clear(&ad); adventurer_agency_destroy(); adventurer_agency_destroy();
    assert(!live);
    puts("Agency packets: wire bytes, ownership, pages, malformed/truncated replies, allocation failures, stale replies, cancellation, timeout and disconnect passed.");
    return 0;
}
