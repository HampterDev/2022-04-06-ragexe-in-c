#ifndef CONTENTS_BYTE_QUEUE_H
#define CONTENTS_BYTE_QUEUE_H

#include <stddef.h>

struct ByteQueue {
    char *data;
    size_t read_pos;
    size_t write_pos;
    size_t capacity;
};

struct ByteQueue *byte_queue_construct(struct ByteQueue *queue);
void byte_queue_destroy(struct ByteQueue *queue);
void byte_queue_reset(struct ByteQueue *queue, int size);
int byte_queue_size(const struct ByteQueue *queue);
void byte_queue_write(struct ByteQueue *queue, int count, const void *src);
void byte_queue_prepend(struct ByteQueue *queue, int count, const void *src);
char byte_queue_read(struct ByteQueue *queue, int count, void *dst);
char byte_queue_peek(const struct ByteQueue *queue, int count, void *dst);
char byte_queue_skip(struct ByteQueue *queue, int count);

#endif
