#include "web/party_booking_json.h"
#include "json/service_json.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int booking_live, booking_attempts, booking_fail;

static void *booking_malloc(size_t size)
{
    unsigned char *p;
    ++booking_attempts;
    if (booking_attempts == booking_fail) return NULL;
    assert(size <= SIZE_MAX - 64);
    p = malloc(size + 64);
    assert(p);
    memset(p, 0xa7, size + 64);
    *(size_t *)p = size;
    ++booking_live;
    return p + 32;
}

static void *booking_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = booking_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}

static void booking_free(void *pointer)
{
    unsigned char *p;
    size_t size, i;
    if (!pointer) return;
    p = (unsigned char *)pointer - 32;
    size = *(size_t *)p;
    for (i = sizeof(size_t); i < 32; ++i) assert(p[i] == 0xa7);
    for (i = 0; i < 32; ++i) assert(p[32 + size + i] == 0xa7);
    assert(booking_live > 0);
    --booking_live;
    free(p);
}

#define malloc booking_malloc
#define calloc booking_calloc
#define free booking_free
#include "../src/core/owned_text.c"
#include "../src/web/party_booking_data.c"
#include "../src/web/party_booking_json.c"
#undef malloc
#undef calloc
#undef free

void test_party_booking_json(void)
{
    static const char response[] =
        "[{\"AID\":4294967295,\"GID\":42,\"Tanker\":1,\"Healer\":0,"
        "\"Dealer\":7,\"Assist\":0,\"MinLV\":7,\"MaxLV\":120,\"Type\":4,"
        "\"CharName\":\"na\\u0000me\",\"Memo\":\"memo\"},"
        "{\"CharName\":true,\"Memo\":false}]";
    struct JsonTree root, empty;
    struct PartyBookingList list = {0};
    struct AdvenPartyInfo single, *saved;
    int i, before;
    json_tree_init(&root);
    json_tree_init(&empty);
    assert(json_parse_text(&root, response, sizeof response - 1));
    assert(party_booking_list_read(&list, &root));
    saved = list.items;
    before = booking_live;
    for (i = 1; i <= 5; ++i) {
        booking_attempts = 0; booking_fail = i;
        assert(!party_booking_list_read(&list, &root));
        assert(list.items == saved && list.count == 2 && booking_live == before);
        assert(list.items[0].name.size == 5 && !memcmp(list.items[0].name.data, "na\0me", 5));
    }
    booking_fail = 0;
    assert(party_booking_list_read(&list, &root));
    json_tree_free(&root);
    assert(list.count == 2 && list.items[0].aid == -1 && list.items[0].gid == 42);
    assert(list.items[0].min_level == 7 && list.items[0].max_level == 120 && list.items[0].purpose == 4);
    assert(!memcmp(list.items[0].roles, "\1\0\1\0", 4));
    assert(list.items[0].name.size == 5 && !memcmp(list.items[0].name.data, "na\0me", 5));
    assert(!strcmp(owned_text_cstr(&list.items[0].memo), "memo"));
    assert(!list.items[1].min_level && list.items[1].max_level == 999 && list.items[1].purpose == 1);
    assert(!memcmp(list.items[1].roles, "\1\1\1\1", 4));
    assert(!strcmp(owned_text_cstr(&list.items[1].name), "true"));
    assert(!strcmp(owned_text_cstr(&list.items[1].memo), "false"));
    party_booking_init(&single);
    assert(party_booking_read(&single, &empty));
    assert(!single.name.size && !single.memo.size && single.max_level == 999);
    party_booking_clear(&single);
    assert(party_booking_list_read(&list, &empty));
    assert(!list.items && !list.count && !booking_live);
    party_booking_list_clear(&list);
}
