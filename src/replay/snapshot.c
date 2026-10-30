#include "replay/snapshot.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define RECORD_HEADER_SIZE 6

static uint32_t read_size(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write_size(unsigned char *p, size_t size)
{
    p[0] = (unsigned char)size;
    p[1] = (unsigned char)(size >> 8);
    p[2] = (unsigned char)(size >> 16);
    p[3] = (unsigned char)(size >> 24);
}

void replay_buffer_destroy(struct ReplayBuffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}

int snapshot_next(struct SnapshotReader *reader, struct SnapshotRecord *record)
{
    const unsigned char *head;
    struct SnapshotRecord result;
    size_t remaining;
    if (reader->input.size > INT_MAX || reader->offset > reader->input.size ||
        (!reader->input.data && reader->input.size))
        return -1;
    remaining = reader->input.size - reader->offset;
    if (!remaining)
        return 0;
    if (remaining < RECORD_HEADER_SIZE)
        return -1;
    head = reader->input.data + reader->offset;
    result.value.size = read_size(head + 2);
    if (result.value.size > remaining - RECORD_HEADER_SIZE)
        return -1;
    result.tag = (uint16_t)(head[0] | (unsigned)head[1] << 8);
    result.value.data = head + RECORD_HEADER_SIZE;
    reader->offset += RECORD_HEADER_SIZE + result.value.size;
    *record = result;
    return 1;
}

int snapshot_open(struct SnapshotReader *reader, const void *data, size_t size, uint16_t tag)
{
    struct SnapshotReader candidate = {{data, size}, 0};
    struct SnapshotRecord record;
    if (snapshot_next(&candidate, &record) != 1 || record.tag != tag || record.value.size)
        return 0;
    *reader = candidate;
    return 1;
}

static int reserve(struct SnapshotWriter *writer, size_t extra)
{
    size_t needed, capacity;
    void *data;
    if (writer->failed)
        return 0;
    if (writer->buffer.size > INT_MAX || extra > INT_MAX - writer->buffer.size) {
        writer->failed = 1;
        return 0;
    }
    needed = writer->buffer.size + extra;
    if (needed <= writer->capacity)
        return 1;
    capacity = writer->capacity <= INT_MAX / 2 ? writer->capacity * 2 : INT_MAX;
    if (capacity < 256) capacity = 256;
    if (capacity < needed) capacity = needed;
    data = realloc(writer->buffer.data, capacity);
    if (!data) {
        writer->failed = 1;
        return 0;
    }
    writer->buffer.data = data;
    writer->capacity = capacity;
    return 1;
}

void snapshot_write(struct SnapshotWriter *writer, uint16_t tag, const void *data, size_t size)
{
    unsigned char *head;
    if ((size && !data) || size > INT_MAX - RECORD_HEADER_SIZE) {
        writer->failed = 1;
        return;
    }
    if (!reserve(writer, RECORD_HEADER_SIZE + size))
        return;
    head = writer->buffer.data + writer->buffer.size;
    head[0] = (unsigned char)tag;
    head[1] = (unsigned char)(tag >> 8);
    write_size(head + 2, size);
    if (size)
        memcpy(head + RECORD_HEADER_SIZE, data, size);
    writer->buffer.size += RECORD_HEADER_SIZE + size;
}

size_t snapshot_begin_record(struct SnapshotWriter *writer, uint16_t tag)
{
    size_t start = writer->buffer.size;
    snapshot_write(writer, tag, NULL, 0);
    return start;
}

void snapshot_end_record(struct SnapshotWriter *writer, size_t start)
{
    if (writer->failed)
        return;
    if (start > writer->buffer.size || writer->buffer.size - start < RECORD_HEADER_SIZE) {
        writer->failed = 1;
        return;
    }
    write_size(writer->buffer.data + start + 2, writer->buffer.size - start - RECORD_HEADER_SIZE);
}

void snapshot_discard(struct SnapshotWriter *writer)
{
    replay_buffer_destroy(&writer->buffer);
    writer->capacity = 0;
    writer->failed = 0;
}

int snapshot_finish(struct SnapshotWriter *writer, struct ReplayBuffer *out)
{
    if (writer->failed) {
        snapshot_discard(writer);
        return 0;
    }
    replay_buffer_destroy(out);
    *out = writer->buffer;
    memset(writer, 0, sizeof *writer);
    return 1;
}
