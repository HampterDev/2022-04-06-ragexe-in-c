#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "contents/player_names.h"
#include "contents/connection_api.h"
#include "owned_storage_test_allocator.h"

static DWORD ticks;
static unsigned sent_count, sent_ids[4096];
static char send_ok = 1;

static void *checked_realloc(void *old, size_t size)
{
    void *next = checked_malloc(size);
    if (!next) return NULL;
    if (old) {
        size_t before = ((struct Allocation *)old - 1)->size;
        memcpy(next, old, before < size ? before : size);
        checked_free(old);
    }
    return next;
}
static uint32_t fixture_ticks(void) { return ticks; }
#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#define timeGetTime fixture_ticks
#define platform_ticks_ms fixture_ticks
#include "../src/core/owned_text.c"
#include "../src/contents/player_names.c"
#undef timeGetTime
#undef platform_ticks_ms
#undef free
#undef realloc
#undef malloc

char *msg_str(int id) { assert(id == 0x245); return "이름 대기"; }
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)1; }
char rag_connection_send_packet(struct RagConnection *connection, int size, const void *bytes)
{
    const unsigned char *p = bytes;
    assert(connection == (struct RagConnection *)1 && size == 6 && p[0] == 0x69 && p[1] == 3);
    assert(sent_count < sizeof sent_ids / sizeof sent_ids[0]);
    sent_ids[sent_count++] = (unsigned)p[2] | (unsigned)p[3] << 8 | (unsigned)p[4] << 16 | (unsigned)p[5] << 24;
    return send_ok;
}

static void reset(void)
{
    player_names_destroy();
    assert(!outstanding);
    fail_at = allocation_calls = sent_count = 0;
    ticks = 100;
    send_ok = 1;
    assert(player_names_create() && !player_names_create());
}

static void equals(unsigned id, const char *name)
{
    struct TextView view = player_name_lookup(id);
    assert(view.size == strlen(name) && !memcmp(view.data, name, view.size));
}

static void ownership(void)
{
    char name[32] = "한국 이름";
    struct TextView view;
    unsigned i;

    reset();
    assert(player_names_set(UINT_MAX, text_view_cstr(name)));
    name[0] = 'x';
    equals(UINT_MAX, "한국 이름");
    view = player_name_lookup(UINT_MAX);
    for (i = 0; i < 1000; ++i) assert(player_names_set(i, text_view_cstr("other")));
    assert(view.data == player_name_lookup(UINT_MAX).data && !strcmp(view.data, "한국 이름"));
    assert(player_names_set(UINT_MAX, text_view(view.data + 2, view.size - 2)));
    assert(player_names_set(0x80000000u, text_view("a\0b", 3)));
    view = player_name_lookup(0x80000000u);
    assert(view.size == 3 && !memcmp(view.data, "a\0b", 3) && !view.data[3]);
    assert(player_names_set(0, text_view("", 0)) && player_names_has(0));
    equals(0, "");
    player_names_update();
    assert(!sent_count);
    player_names_clear();
    assert(!outstanding && !player_names_create() && !player_names_has(UINT_MAX));
    equals(77, "이름 대기");
    player_names_update();
    assert(sent_count == 1 && sent_ids[0] == 77);
    player_names_destroy();
    assert(!outstanding);
    equals(88, "이름 대기");
    assert(!player_names_set(88, text_view_cstr("inactive")));
    player_names_update();
    assert(sent_count == 1 && !outstanding);
}

static void requests(void)
{
    unsigned i;

    reset();
    equals(41, "이름 대기"); equals(41, "이름 대기");
    equals(42, "이름 대기"); equals(0, "이름 대기"); equals(UINT_MAX, "이름 대기");
    player_names_update(); player_names_update(); player_names_update();
    assert(sent_count == 2 && sent_ids[0] == 41 && sent_ids[1] == 42);
    player_names_update();
    assert(sent_count == 3 && sent_ids[2] == UINT_MAX);
    for (i = 0; i < 100; ++i) { equals(41, "이름 대기"); player_names_update(); }
    assert(sent_count == 3);
    ticks += 9999; equals(41, "이름 대기"); player_names_update();
    assert(sent_count == 3);
    ++ticks; equals(41, "이름 대기"); player_names_update();
    assert(sent_count == 4 && sent_ids[3] == 41);
    assert(player_names_set(41, text_view_cstr("arrived")));
    ticks += 700000; equals(41, "arrived"); player_names_update();
    assert(sent_count == 4);

    reset();
    equals(1, "이름 대기"); assert(player_names_set(1, text_view_cstr("fresh")));
    ticks += 600000; player_names_update(); assert(!sent_count);
    equals(2, "이름 대기"); assert(player_names_set(2, text_view_cstr("old")));
    ticks += 600001; player_names_update(); assert(sent_count == 1 && sent_ids[0] == 2);

    reset();
    ticks = UINT_MAX - 5000;
    equals(1, "이름 대기"); player_names_update();
    ticks += 9999; equals(1, "이름 대기"); player_names_update(); assert(sent_count == 1);
    ++ticks; equals(1, "이름 대기"); player_names_update(); assert(sent_count == 2);
    ticks = UINT_MAX - 300000;
    equals(2, "이름 대기"); assert(player_names_set(2, text_view_cstr("rollover")));
    ticks += 600000; player_names_update(); assert(sent_count == 2);

    reset();
    send_ok = 0;
    equals(3, "이름 대기"); player_names_update();
    equals(3, "이름 대기"); player_names_update(); assert(sent_count == 1);
    player_names_clear();
    for (i = 1; i <= 1000; ++i) equals(i, "이름 대기");
    sent_count = 0;
    for (i = 1; i <= 1000; ++i) { player_names_update(); assert(sent_count == i && sent_ids[i-1] == i); }
    player_names_update(); assert(sent_count == 1000);
}

static void allocation_failures(void)
{
    unsigned failure, i;

    reset();
    fail_at = allocation_calls + 1;
    equals(1, "이름 대기"); player_names_update(); assert(!sent_count);
    fail_at = 0; equals(1, "이름 대기");
    fail_at = allocation_calls + 1;
    player_names_update(); assert(!sent_count);
    fail_at = 0; player_names_update(); assert(sent_count == 1);
    assert(player_names_set(2, text_view_cstr("original")));
    fail_at = allocation_calls + 1;
    assert(!player_names_set(2, text_view_cstr("replacement")));
    equals(2, "original");
    fail_at = allocation_calls + 1;
    assert(!player_names_set(1, text_view_cstr("arrival")));
    fail_at = 0; equals(1, "이름 대기"); player_names_update(); assert(sent_count == 1);
    for (failure = 1; failure <= 100; ++failure) {
        reset();
        fail_at = failure;
        for (i = 1; i <= 40; ++i) {
            int stored = player_names_set(i, text_view_cstr("가나다 long owned name"));
            assert(player_names_has(i) == stored);
            if (stored) equals(i, "가나다 long owned name");
            else equals(i, "이름 대기");
        }
        for (i = 41; i <= 80; ++i) { equals(i, "이름 대기"); player_names_update(); }
        player_names_destroy(); assert(!outstanding);
    }
}

static void guarded_names(void)
{
    SYSTEM_INFO info;
    char *memory;
    DWORD protection;
    size_t size;

    reset();
    GetSystemInfo(&info);
    memory = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(memory && VirtualProtect(memory + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &protection));
    memset(memory, 'x', info.dwPageSize);
    for (size = 0; size <= info.dwPageSize; ++size) {
        struct TextView name = text_view(memory + info.dwPageSize - size, size);
        assert(player_names_set(1, name));
        assert(player_name_lookup(1).size == size);
    }
    assert(VirtualFree(memory, 0, MEM_RELEASE));
}

int main(void)
{
    ownership(); requests(); allocation_failures(); guarded_names();
    player_names_destroy(); player_names_destroy(); assert(!outstanding);
    puts("Player names: owned counted names, borrowed lifetimes, ordered requests, exact packets, time boundaries/rollover, allocation rollback and guarded endings passed.");
    return 0;
}
