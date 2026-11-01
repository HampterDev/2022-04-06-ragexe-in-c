/* Compile the actual patched Hercules handler against a small map-server fixture. */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef uint16_t uint16;
typedef uint32_t uint32;
#define MAX_PARTY 12
#define SELF 0
#define DIFF_TICK(a,b) ((a)-(b))
#define aMalloc malloc
#define aFree free
struct block_list { int id; };
struct map_session_data {
    struct block_list bl;
    struct { int account_id, char_id, party_id; char name[24]; } status;
    int64_t agency_next[5];
};
struct party_data {
    struct { int party_id; struct { int account_id, char_id, leader; } member[MAX_PARTY]; } party;
    struct { struct map_session_data *sd; } data[MAX_PARTY];
    struct { int owner; unsigned short min_level, max_level; unsigned char roles, purpose; char memo[151]; } agency;
};
static struct party_data parties[12];
static struct map_session_data players[13];
static unsigned char request_bytes[512], response[2048];
static size_t response_size;
static int sent_count;
static int64_t now = 1000;
static struct party_data *find_party(int id) { return id >= 1 && id <= 12 ? &parties[id - 1] : NULL; }
static bool is_leader(struct map_session_data *sd, struct party_data *p)
{ return p && p->data[0].sd == sd && p->party.member[0].leader; }
static struct map_session_data *find_player(int aid)
{
    int i; for (i = 0; i < 13; ++i) if (players[i].status.account_id == aid) return &players[i];
    return NULL;
}
struct DBIterator { int at; };
static struct DBIterator iterator;
#define db_size(db) ((void)(db), 12)
#define db_iterator(db) ((void)(db), iterator.at = 0, &iterator)
#define dbi_first(it) (&parties[(it)->at = 0])
#define dbi_exists(it) ((it)->at < 12)
#define dbi_next(it) (++(it)->at < 12 ? &parties[(it)->at] : NULL)
#define dbi_destroy(it) ((void)(it))
static int64_t gettick(void) { return now; }
static void send_packet(const void *p, int size, struct block_list *bl, int scope)
{
    (void)bl; assert(scope == SELF && size >= 16 && size <= 1906);
    memcpy(response, p, size); response_size = size; ++sent_count;
}
static struct { struct party_data *(*search)(int); bool (*is_leader)(struct map_session_data *, struct party_data *); void *db; } party_api = {find_party, is_leader, NULL}, *party = &party_api;
static struct { struct map_session_data *(*id2sd)(int); } map_api = {find_player}, *map = &map_api;
static struct { int64_t (*gettick)(void); } timer_api = {gettick}, *timer = &timer_api;
static struct { void (*send)(const void *, int, struct block_list *, int); } clif_api = {send_packet}, *clif = &clif_api;
#define RFIFOP(fd,offset) ((void)(fd), request_bytes + (offset))
#define RFIFOW(fd,offset) ((void)(fd), (unsigned)(request_bytes[offset] | request_bytes[(offset)+1] << 8))

#include "agency_server_handler.inc"

static void prepare(unsigned op, unsigned page, uint32 serial)
{
    now += 300;
    memset(request_bytes, 0, sizeof request_bytes);
    agency_w16(request_bytes, 0x0bf0); agency_w16(request_bytes + 2, 24);
    request_bytes[4] = 1; request_bytes[5] = (unsigned char)op;
    agency_w32(request_bytes + 6, serial); agency_w16(request_bytes + 10, page);
    agency_w16(request_bytes + 18, 999);
}
static void call(int index) { clif_parse_agency(0, &players[index]); }
static void status(int expected) { assert(response[10] == expected); }

int main(int argc, char **argv)
{
    int i, before;
    unsigned char saved[2048];
    for (i = 0; i < 13; ++i) {
        players[i].status.account_id = 100 + i;
        players[i].status.char_id = 200 + i;
        players[i].bl.id = 100 + i;
        strcpy(players[i].status.name, "Bob");
        if (i == 12) break;
        players[i].status.party_id = i + 1;
        parties[i].party.party_id = i + 1;
        parties[i].party.member[0].account_id = 100 + i;
        parties[i].party.member[0].char_id = 200 + i;
        parties[i].party.member[0].leader = 1;
        parties[i].data[0].sd = &players[i];
        prepare(4, 1, i + 1);
        agency_w16(request_bytes + 16, 30); agency_w16(request_bytes + 18, 90);
        request_bytes[20] = 5; request_bytes[21] = 2;
        memcpy(request_bytes + 24, "Hunt", 4); agency_w16(request_bytes + 22, 4); agency_w16(request_bytes + 2, 28);
        call(i); status(0); assert(parties[i].agency.owner == 200 + i);
    }
    prepare(2, 1, 41); call(12); status(0);
    assert(response[11] == 10 && agency_u16(response + 14) == 2 && agency_u32(response + 16) == 100);
    assert(response_size == 246);
    if (argc == 2) {
        FILE *f = fopen(argv[1], "wb"); assert(f);
        assert(fwrite(response, 1, response_size, f) == response_size); assert(!fclose(f));
    }
    memcpy(saved, response, response_size);
    prepare(2, 20, 42); call(12); status(0);
    assert(response[11] == 2 && agency_u16(response + 12) == 2 && agency_u32(response + 16) == 110);
    prepare(2, 1, 43); request_bytes[20] = 2; call(12); status(0); assert(!response[11] && agency_u16(response + 14) == 1);
    prepare(2, 1, 44); request_bytes[21] = 3; call(12); status(0); assert(!response[11]);
    prepare(2, 1, 45); agency_w16(request_bytes + 16, 91); call(12); status(0); assert(!response[11]);
    prepare(2, 1, 46); memcpy(request_bytes + 24, "Hunt", 4); agency_w16(request_bytes + 22, 4); agency_w16(request_bytes + 2, 28); call(12); status(0); assert(response[11] == 10);
    prepare(3, 1, 47); agency_w32(request_bytes + 12, 101); call(12); status(0); assert(response[11] == 1 && agency_u32(response + 16) == 101);
    prepare(1, 1, 48); call(0); status(0); assert(response[11] == 1 && agency_u32(response + 16) == 100);
    /* Only the authenticated leader can mutate the party's listing. */
    prepare(5, 1, 49); agency_w32(request_bytes + 12, 101); call(0); status(1); assert(parties[1].agency.owner == 201);
    players[12].status.party_id = 1;
    prepare(4, 1, 50); call(12); status(2); assert(parties[0].agency.owner == 200);
    prepare(5, 1, 51); call(12); status(2); assert(parties[0].agency.owner == 200);
    prepare(4, 1, 52); request_bytes[20] = 16; call(0); status(1); assert(parties[0].agency.roles == 5);
    prepare(4, 1, 53); agency_w16(request_bytes + 22, 151); agency_w16(request_bytes + 2, 175); call(0); status(1);
    prepare(4, 1, 54); agency_w16(request_bytes + 22, 1); agency_w16(request_bytes + 2, 25); request_bytes[24] = '\n'; call(0); status(1);
    /* All short packets return without reading fields or emitting a response. */
    for (i = 0; i < 24; ++i) {
        prepare(4, 1, 55); agency_w16(request_bytes + 2, i); before = sent_count; call(0); assert(sent_count == before);
    }
    prepare(4, 1, 56); agency_w16(request_bytes + 22, 4); agency_w16(request_bytes + 2, 27); call(0); status(1);
    prepare(2, 1, 57); call(12); status(0); call(12); status(4);
    prepare(5, 1, 58); call(0); status(0); assert(!parties[0].agency.owner);
    /* A lost leader, offline owner or full party cannot retain an advertisement. */
    parties[1].party.member[0].leader = 0; assert(!agency_leader(&parties[1]) && !parties[1].agency.owner);
    parties[2].data[0].sd = NULL; assert(!agency_leader(&parties[2]) && !parties[2].agency.owner);
    for (i = 1; i < MAX_PARTY; ++i) parties[3].party.member[i].account_id = 500 + i;
    assert(!agency_leader(&parties[3]) && !parties[3].agency.owner);
    prepare(4, 1, 59); call(3); status(2);
    prepare(3, 1, 60); agency_w32(request_bytes + 12, 101); call(12); status(3);
    puts("Hercules Agency: actual handler validates packets, authenticates ownership, filters/pages ads, throttles requests and drops stale/full listings.");
    return 0;
}
