#ifndef REPLAY_SNAPSHOT_H
#define REPLAY_SNAPSHOT_H
#include <stddef.h>
#include <stdint.h>

struct ReplayBuffer { unsigned char *data; size_t size; };
struct SnapshotSpan { const unsigned char *data; size_t size; };
struct SnapshotRecord { uint16_t tag; struct SnapshotSpan value; };
struct SnapshotReader { struct SnapshotSpan input; size_t offset; };
struct SnapshotWriter {
    struct ReplayBuffer buffer;
    size_t capacity;
    int failed;
};

void replay_buffer_destroy(struct ReplayBuffer *buffer);
int snapshot_next(struct SnapshotReader *reader, struct SnapshotRecord *record);
int snapshot_open(struct SnapshotReader *reader, const void *data, size_t size, uint16_t tag);
void snapshot_write(struct SnapshotWriter *writer, uint16_t tag, const void *data, size_t size);
size_t snapshot_begin_record(struct SnapshotWriter *writer, uint16_t tag);
void snapshot_end_record(struct SnapshotWriter *writer, size_t start);
int snapshot_finish(struct SnapshotWriter *writer, struct ReplayBuffer *out);
void snapshot_discard(struct SnapshotWriter *writer);
#endif
