#include "contents/byte_queue.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void reserve(struct ByteQueue *queue, size_t required)
{
    size_t capacity;
    char *data;

    if (required > INT_MAX)
        abort();
    if (required <= queue->capacity)
        return;
    capacity = queue->capacity <= INT_MAX / 2 ? queue->capacity * 2 : INT_MAX;
    if (capacity < required)
        capacity = required;
    data = realloc(queue->data, capacity);
    if (!data)
        abort();
    queue->data = data;
    queue->capacity = capacity;
}

struct ByteQueue *byte_queue_construct(struct ByteQueue *queue)
{
    memset(queue, 0, sizeof *queue);
    return queue;
}

void byte_queue_destroy(struct ByteQueue *queue)
{
    free(queue->data);
    memset(queue, 0, sizeof *queue);
}

void byte_queue_reset(struct ByteQueue *queue, int size)
{
    if (size < 0)
        abort();
    reserve(queue, (size_t)size);
    queue->read_pos = queue->write_pos = 0;
}

int byte_queue_size(const struct ByteQueue *queue)
{
    return (int)(queue->write_pos - queue->read_pos);
}

void byte_queue_write(struct ByteQueue *queue, int count, const void *src)
{
    size_t unread = queue->write_pos - queue->read_pos;

    if (count < 0 || (size_t)count > INT_MAX - unread)
        abort();
    if (!count)
        return;
    if ((size_t)count > queue->capacity - queue->write_pos) {
        if (unread)
            memmove(queue->data, queue->data + queue->read_pos, unread);
        queue->read_pos = 0;
        queue->write_pos = unread;
        reserve(queue, unread + (size_t)count);
    }
    memcpy(queue->data + queue->write_pos, src, (size_t)count);
    queue->write_pos += (size_t)count;
}

void byte_queue_prepend(struct ByteQueue *queue, int count, const void *src)
{
    size_t unread = queue->write_pos - queue->read_pos;

    if (count < 0 || (size_t)count > INT_MAX - unread)
        abort();
    if (!count)
        return;
    reserve(queue, unread + (size_t)count);
    if (unread)
        memmove(queue->data + count, queue->data + queue->read_pos, unread);
    memcpy(queue->data, src, (size_t)count);
    queue->read_pos = 0;
    queue->write_pos = unread + (size_t)count;
}

char byte_queue_peek(const struct ByteQueue *queue, int count, void *dst)
{
    if (count < 0 || (size_t)count > queue->write_pos - queue->read_pos)
        return 0;
    if (count)
        memcpy(dst, queue->data + queue->read_pos, (size_t)count);
    return 1;
}

char byte_queue_read(struct ByteQueue *queue, int count, void *dst)
{
    if (!byte_queue_peek(queue, count, dst))
        return 0;
    queue->read_pos += (size_t)count;
    return 1;
}

char byte_queue_skip(struct ByteQueue *queue, int count)
{
    if (count < 0 || (size_t)count > queue->write_pos - queue->read_pos)
        return 0;
    queue->read_pos += (size_t)count;
    return 1;
}
