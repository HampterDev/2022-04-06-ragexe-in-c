#include "contents/player_names.h"
#include "contents/connection_api.h"
#include "core/msg_str.h"
#include "core/owned_text.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "platform/win_types.h"
#include "base.h"
#include "platform/clock.h"

#pragma pack(push, 1)
struct PACKET_CZ_REQNAME_BYGID2 {
    WORD packet_type;
    DWORD char_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_CZ_REQNAME_BYGID2, 6);

enum { MSG_NAME_PENDING = 0x245, REQUEST_TIMEOUT = 10000, NAME_LIFETIME = 600000 };

struct PlayerNameEntry {
    unsigned char_id;
    struct OwnedText name;
    uint32_t time;
};

struct NameRequest {
    unsigned char_id;
    uint32_t sent;
};

static struct {
    int active;
    struct PlayerNameEntry *names;
    size_t count, capacity;
    unsigned *wanted;
    size_t wanted_first, wanted_count, wanted_capacity;
    struct NameRequest *asked;
    size_t asked_count, asked_capacity;
} cache;

static void *reserve(void *items, size_t *capacity, size_t count, size_t width)
{
    size_t grown, limit = SIZE_MAX / width;
    void *next;

    if (count <= *capacity) return items;
    if (count > limit) return NULL;
    grown = *capacity > limit / 2 ? limit : *capacity * 2;
    if (grown < 16) grown = limit < 16 ? limit : 16;
    if (grown < count) grown = count;
    next = realloc(items, grown * width);
    if (next) *capacity = grown;
    return next;
}

static size_t find_name(unsigned char_id)
{
    size_t first = 0, end = cache.count;

    while (first < end) {
        size_t mid = first + (end - first) / 2;
        if (cache.names[mid].char_id < char_id) first = mid + 1;
        else end = mid;
    }
    return first;
}

static size_t find_request(unsigned char_id)
{
    size_t i;
    for (i = 0; i < cache.asked_count; ++i)
        if (cache.asked[i].char_id == char_id) break;
    return i;
}

static void want(unsigned char_id)
{
    size_t i;
    unsigned *wanted;

    for (i = 0; i < cache.wanted_count; ++i)
        if (cache.wanted[cache.wanted_first + i] == char_id) return;
    if (cache.wanted_first && cache.wanted_first + cache.wanted_count == cache.wanted_capacity) {
        memmove(cache.wanted, cache.wanted + cache.wanted_first,
                cache.wanted_count * sizeof *cache.wanted);
        cache.wanted_first = 0;
    }
    if (cache.wanted_count == SIZE_MAX) return;
    wanted = reserve(cache.wanted, &cache.wanted_capacity,
                     cache.wanted_first + cache.wanted_count + 1, sizeof *wanted);
    if (!wanted) return;
    cache.wanted = wanted;
    cache.wanted[cache.wanted_first + cache.wanted_count++] = char_id;
}

static void pop_wanted(void)
{
    if (--cache.wanted_count) ++cache.wanted_first;
    else cache.wanted_first = 0;
}

int player_names_create(void)
{
    if (cache.active) return 0;
    cache.active = 1;
    return 1;
}

void player_names_clear(void)
{
    size_t i;
    int active = cache.active;

    for (i = 0; i < cache.count; ++i) owned_text_clear(&cache.names[i].name);
    free(cache.names);
    free(cache.wanted);
    free(cache.asked);
    memset(&cache, 0, sizeof cache);
    cache.active = active;
}

void player_names_destroy(void)
{
    player_names_clear();
    cache.active = 0;
}

int player_names_set(unsigned char_id, struct TextView name)
{
    struct OwnedText copy = {0};
    struct PlayerNameEntry *names;
    size_t at, request;

    if (!cache.active || !owned_text_assign(&copy, name)) return 0;
    at = find_name(char_id);
    if (at < cache.count && cache.names[at].char_id == char_id) {
        owned_text_clear(&cache.names[at].name);
    } else {
        names = cache.count == SIZE_MAX ? NULL :
            reserve(cache.names, &cache.capacity, cache.count + 1, sizeof *names);
        if (!names) { owned_text_clear(&copy); return 0; }
        cache.names = names;
        memmove(names + at + 1, names + at, (cache.count - at) * sizeof *names);
        ++cache.count;
        cache.names[at].char_id = char_id;
    }
    cache.names[at].name = copy;
    cache.names[at].time = platform_ticks_ms();
    request = find_request(char_id);
    if (request < cache.asked_count) {
        --cache.asked_count;
        memmove(cache.asked + request, cache.asked + request + 1,
                (cache.asked_count - request) * sizeof *cache.asked);
    }
    return 1;
}

int player_names_has(unsigned char_id)
{
    size_t at = find_name(char_id);
    return at < cache.count && cache.names[at].char_id == char_id;
}

struct TextView player_name_lookup(unsigned char_id)
{
    size_t at = find_name(char_id);

    if (at < cache.count && cache.names[at].char_id == char_id)
        return owned_text_view(&cache.names[at].name);
    if (cache.active) want(char_id);
    return text_view_cstr(msg_str(MSG_NAME_PENDING));
}

void player_names_update(void)
{
    size_t expired = 0;
    uint32_t now;

    if (!cache.active) return;
    now = platform_ticks_ms();
    while (expired < cache.asked_count &&
           (uint32_t)(now - cache.asked[expired].sent) >= REQUEST_TIMEOUT) ++expired;
    if (expired) {
        cache.asked_count -= expired;
        memmove(cache.asked, cache.asked + expired, cache.asked_count * sizeof *cache.asked);
    }
    while (cache.wanted_count) {
        unsigned char_id = cache.wanted[cache.wanted_first];
        size_t at = find_name(char_id);
        struct NameRequest *asked;
        struct PACKET_CZ_REQNAME_BYGID2 packet = {0x369};

        if ((at < cache.count && cache.names[at].char_id == char_id &&
             (uint32_t)(now - cache.names[at].time) <= NAME_LIFETIME) ||
            find_request(char_id) < cache.asked_count) {
            pop_wanted();
            continue;
        }
        if (!char_id) { pop_wanted(); return; }
        asked = cache.asked_count == SIZE_MAX ? NULL :
            reserve(cache.asked, &cache.asked_capacity, cache.asked_count + 1, sizeof *asked);
        if (!asked) return;
        cache.asked = asked;
        pop_wanted();
        packet.char_id = char_id;
        rag_connection_send_packet(rag_connection_instance(), sizeof packet, &packet);
        cache.asked[cache.asked_count++] = (struct NameRequest){char_id, platform_ticks_ms()};
        return;
    }
}
