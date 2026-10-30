#include "replay/snapshot.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned blocks, attempts, fail_at;
static void *resize(void *old, size_t size)
{
    void *p;
    if (++attempts == fail_at) return NULL;
    p = realloc(old, size); assert(p);
    if (!old) ++blocks;
    return p;
}
static void release(void *p)
{
    if (p) { assert(blocks); --blocks; free(p); }
}
#define realloc resize
#define free release
#include "../src/replay/snapshot.c"
#undef realloc
#undef free

static void records_and_bounds(void)
{
    static const unsigned char expected[] = {
        1,0,0,0,0,0,
        2,0,15,0,0,0, 3,0,3,0,0,0,'a','b','c', 4,0,0,0,0,0,
        5,0,0,0,0,0
    };
    struct SnapshotWriter writer = {0};
    struct ReplayBuffer buffer = {0};
    struct SnapshotReader reader, saved;
    struct SnapshotRecord record, previous;
    size_t start, i;
    unsigned char bad[sizeof expected];
    snapshot_write(&writer, 1, NULL, 0);
    start = snapshot_begin_record(&writer, 2);
    snapshot_write(&writer, 3, "abc", 3);
    snapshot_write(&writer, 4, NULL, 0);
    snapshot_end_record(&writer, start);
    snapshot_write(&writer, 5, NULL, 0);
    assert(snapshot_finish(&writer, &buffer));
    assert(buffer.size == sizeof expected && !memcmp(buffer.data, expected, sizeof expected));
    assert(!writer.buffer.data && !writer.capacity && !writer.failed);
    assert(snapshot_open(&reader, buffer.data, buffer.size, 1));
    assert(snapshot_next(&reader, &record) == 1 && record.tag == 2 && record.value.size == 15);
    assert(snapshot_open(&saved, record.value.data, record.value.size, 3) == 0);
    assert(snapshot_next(&reader, &record) == 1 && record.tag == 5 && !record.value.size);
    assert(!snapshot_next(&reader, &record));
    for (i = 1; i < 21; ++i) {
        reader.input.data = expected + 6; reader.input.size = i; reader.offset = 0;
        saved = reader; memset(&record, 0xa5, sizeof record); previous = record;
        assert(snapshot_next(&reader, &record) == -1);
        assert(!memcmp(&reader, &saved, sizeof reader) && !memcmp(&record, &previous, sizeof record));
    }
    memcpy(bad, expected, sizeof bad); memset(bad + 8, 0xff, 4);
    assert(snapshot_open(&reader, bad, sizeof bad, 1));
    assert(snapshot_next(&reader, &record) == -1);
    reader.input.data = NULL; reader.input.size = 6; reader.offset = 0;
    assert(snapshot_next(&reader, &record) == -1);
    reader.input.data = expected; reader.input.size = (size_t)INT_MAX + 1;
    assert(snapshot_next(&reader, &record) == -1);
    reader.input.size = 0; reader.offset = 1;
    assert(snapshot_next(&reader, &record) == -1);
    reader.offset = 0; assert(!snapshot_next(&reader, &record));
    replay_buffer_destroy(&buffer); replay_buffer_destroy(&buffer);
    assert(!blocks);
}

static void allocation_failures(void)
{
    unsigned failure, i;
    for (failure = 1; failure <= 8; ++failure) {
        struct SnapshotWriter writer = {0};
        struct ReplayBuffer out = {0};
        unsigned char *old;
        fail_at = 0;
        out.data = resize(NULL, 3); out.size = 3; memcpy(out.data, "old", 3); old = out.data;
        attempts = 0; fail_at = failure;
        for (i = 0; i < 300; ++i)
            snapshot_write(&writer, (uint16_t)i, "abc", 3);
        if (failure <= attempts) {
            assert(!snapshot_finish(&writer, &out));
            assert(out.data == old && out.size == 3 && !memcmp(out.data, "old", 3));
        } else {
            assert(snapshot_finish(&writer, &out) && out.size == 2700);
        }
        assert(!writer.buffer.data && !writer.failed && blocks == 1);
        snapshot_discard(&writer); replay_buffer_destroy(&out);
        assert(!blocks);
    }
    {
        struct SnapshotWriter writer = {0};
        struct ReplayBuffer out = {0};
        fail_at = 0;
        snapshot_write(&writer, 1, NULL, 1);
        assert(!snapshot_finish(&writer, &out));
        snapshot_write(&writer, 1, "x", (size_t)-1);
        assert(!snapshot_finish(&writer, &out));
        snapshot_write(&writer, 1, NULL, 0);
        snapshot_end_record(&writer, (size_t)-1);
        assert(!snapshot_finish(&writer, &out) && !blocks);
    }
}

int main(void)
{
    records_and_bounds(); allocation_failures();
    puts("Snapshot records: frozen nested bytes, truncation, integer bounds, cursor retention, every allocation failure and repeat cleanup passed");
    return 0;
}
