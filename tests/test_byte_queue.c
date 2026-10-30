#include "contents/byte_queue.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct ByteQueue queue;
    unsigned char model[65536], input[32768], actual[65536];
    size_t length = 0;
    unsigned seed = 12345;
    int i;

    byte_queue_construct(&queue);
    byte_queue_reset(&queue, 1);
    for (i = 0; i < sizeof input; ++i)
        input[i] = (unsigned char)i;
    byte_queue_write(&queue, sizeof input, input);
    assert(byte_queue_read(&queue, sizeof input, actual));
    assert(memcmp(input, actual, sizeof input) == 0);
    byte_queue_reset(&queue, 0);
    assert(byte_queue_read(&queue, 0, NULL));
    assert(!byte_queue_read(&queue, -1, actual));
    assert(!byte_queue_skip(&queue, -1));

    for (i = 0; i < 50000; ++i) {
        unsigned op, count;
        seed = seed * 1664525u + 1013904223u;
        op = seed % 5;
        count = (seed >> 8) % 127;
        if (length + count >= sizeof model)
            op = 2;
        if (op == 0) {
            byte_queue_write(&queue, (int)count, input);
            memcpy(model + length, input, count);
            length += count;
        } else if (op == 1) {
            byte_queue_prepend(&queue, (int)count, input);
            memmove(model + count, model, length);
            memcpy(model, input, count);
            length += count;
        } else if (op == 2 || op == 3) {
            int success = op == 2 ? byte_queue_read(&queue, (int)count, actual)
                                  : byte_queue_skip(&queue, (int)count);
            assert(success == (count <= length));
            if (success) {
                if (op == 2)
                    assert(memcmp(model, actual, count) == 0);
                memmove(model, model + count, length - count);
                length -= count;
            }
        } else {
            assert(byte_queue_peek(&queue, (int)count, actual) == (count <= length));
            if (count <= length)
                assert(memcmp(model, actual, count) == 0);
        }
        assert(byte_queue_size(&queue) == (int)length);
        assert(byte_queue_peek(&queue, (int)length, actual));
        assert(memcmp(model, actual, length) == 0);
    }
    byte_queue_destroy(&queue);
    assert(queue.data == NULL && byte_queue_size(&queue) == 0);
    puts("byte queue: growth, prepend and 50000 mixed operations passed");
    return 0;
}
