#include "world/record_wait.h"

static void remove_at(struct RecordWait *wait, int at)
{
    int i;

    for (i = at + 1; i < wait->count; i++)
        wait->items[i - 1] = wait->items[i];
    wait->count--;
}

void record_wait_put(struct RecordWait *wait, uint32_t now, const struct RecordWaiting *item)
{
    if (wait->count == RECORD_WAIT_ITEMS)
        remove_at(wait, 0);
    wait->items[wait->count] = *item;
    wait->items[wait->count].since = now;
    wait->count++;
}

int record_wait_take(struct RecordWait *wait, uint32_t now, int (*present)(void *context, uint32_t gid),
                     void *context, struct RecordWaiting *item)
{
    int i = 0;

    while (i < wait->count) {
        if (now - wait->items[i].since > RECORD_WAIT_MS) {
            remove_at(wait, i);
            continue;
        }
        if (present(context, wait->items[i].gid)) {
            *item = wait->items[i];
            remove_at(wait, i);
            return 1;
        }
        i++;
    }
    return 0;
}

void record_wait_clear(struct RecordWait *wait)
{
    wait->count = 0;
}
