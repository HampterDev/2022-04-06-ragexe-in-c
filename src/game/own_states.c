#include "game/own_states.h"
#include <stdlib.h>
#include <string.h>

static struct OwnStateEntry *entries;
static size_t count, capacity;

size_t own_states_count(void) { return count; }
const struct OwnStateEntry *own_states_at(size_t position)
{
    return position < count ? &entries[position] : NULL;
}
const struct OwnStateEntry *own_states_find(int index)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (entries[i].index == index) return &entries[i];
    return NULL;
}
int own_states_push(const struct OwnStateEntry *entry)
{
    struct OwnStateEntry copy, *grown;
    size_t next;
    if (!entry) return 0;
    copy = *entry;
    if (count == capacity) {
        if (capacity >= SIZE_MAX / sizeof *entries) return 0;
        next = capacity > SIZE_MAX / sizeof *entries / 2 ?
            SIZE_MAX / sizeof *entries : capacity ? capacity * 2 : 16;
        grown = realloc(entries, next * sizeof *entries);
        if (!grown) return 0;
        entries = grown;
        capacity = next;
    }
    entries[count++] = copy;
    return 1;
}
void own_states_erase_at(size_t position)
{
    if (position >= count) return;
    memmove(entries + position, entries + position + 1,
            (count - position - 1) * sizeof *entries);
    --count;
}
void own_states_remove(int low, int high)
{
    size_t read, write = 0;
    for (read = 0; read < count; ++read)
        if ((uint32_t)entries[read].index - (uint32_t)low >
            (uint32_t)high - (uint32_t)low)
            entries[write++] = entries[read];
    count = write;
}
void own_states_set_value(int index, int value)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (entries[i].index == index) {
            entries[i].value = value;
            return;
        }
}
int own_states_set_present(int index, int present)
{
    size_t i;
    struct OwnStateEntry entry = {0};
    for (i = 0; i < count; ++i)
        if (entries[i].index == index) {
            if (!present) own_states_erase_at(i);
            return 1;
        }
    if (!present) return 1;
    entry.index = index;
    entry.end_tick = OWN_STATE_NO_END;
    return own_states_push(&entry);
}
void own_states_clear(void) { count = 0; }
void own_states_destroy(void)
{
    free(entries);
    entries = NULL;
    count = capacity = 0;
}
