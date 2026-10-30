#include "replay/replay.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int fail_allocation;
static unsigned allocations;
static void *allocate(size_t size)
{
    void *p;
    if (fail_allocation) return NULL;
    p = malloc(size); assert(p); ++allocations; return p;
}
static void release(void *p)
{
    if (p) { assert(allocations); --allocations; free(p); }
}
#define malloc allocate
#define free release
#include "../src/replay/snapshot.c"
#include "../src/replay/replay_session.c"
#undef malloc
#undef free

static unsigned char expected[300];
static size_t used;
static void field(unsigned tag, const void *data, size_t size)
{
    unsigned i;
    expected[used++] = (unsigned char)tag;
    expected[used++] = (unsigned char)(tag >> 8);
    for (i = 0; i < 4; ++i) expected[used++] = (unsigned char)(size >> (i * 8));
    if (size) memcpy(expected + used, data, size);
    used += size;
}

int main(void)
{
    struct ReplaySessionInfo info = {0}, out, saved;
    struct ReplayBuffer buffer = {0};
    unsigned char altered[300];
    size_t i, pos;
    const unsigned offsets[] = {12,16,20,24,88,108,112,116,120,124,128,132,136,166};
    const unsigned sizes[] = {4,4,4,64,20,4,4,4,4,4,4,4,30,4};
    for (i = 0; i < sizeof info; ++i) ((unsigned char *)&info)[i] = (unsigned char)(i * 17 + 9);
    field(0x3b6, NULL, 0);
    for (i = 0; i < 14; ++i) field(i == 13 ? 0x3c0 : 0x3c1 + (unsigned)i, (unsigned char *)&info + offsets[i], sizes[i]);
    field(0x3b7, NULL, 0);
    assert(used == 254);
    assert(replay_build_session_chunk(&info, &buffer));
    assert(buffer.size == (int)used && !memcmp(buffer.data, expected, used));
    out = info; memset((unsigned char *)&out + 12, 0, sizeof out - 12);
    assert(replay_parse_session_chunk(&out, expected, used) == (int)used);
    assert(!memcmp(&out, &info, sizeof info));
    memset(&saved, 0xa5, sizeof saved);
    for (i = 0; i < used; ++i) {
        out = saved;
        assert(replay_parse_session_chunk(&out, expected, i) == -1);
        assert(!memcmp(&out, &saved, sizeof out));
    }
    pos = 6;
    for (i = 0; i < 14; ++i) {
        memcpy(altered, expected, used);
        altered[pos + 2]++;
        out = saved;
        assert(replay_parse_session_chunk(&out, altered, used) == -1);
        assert(!memcmp(&out, &saved, sizeof out));
        pos += 6 + sizes[i];
    }
    memcpy(altered, expected, used - 6);
    altered[used - 6] = 0x34; altered[used - 5] = 0x12;
    memset(altered + used - 4, 0, 4);
    memcpy(altered + used, expected + used - 6, 6);
    out = info;
    assert(replay_parse_session_chunk(&out, altered, used + 6) == (int)used + 6);
    assert(!memcmp(&out, &info, sizeof info));
    out = saved;
    assert(replay_parse_session_chunk(&out, expected + used - 6, 6) == -1);
    fail_allocation = 1;
    assert(!replay_build_session_chunk(&info, &buffer));
    assert(buffer.size == (int)used && !memcmp(buffer.data, expected, used));
    fail_allocation = 0;
    assert(replay_build_session_chunk(&info, &buffer));
    assert(allocations == 1);
    replay_buffer_destroy(&buffer); replay_buffer_destroy(&buffer);
    assert(!allocations);
    puts("Replay session: frozen bytes, every-byte truncation, exact field widths, unknown tags, atomic parsing and allocation failure passed");
    return 0;
}
