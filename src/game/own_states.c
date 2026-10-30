#include "game/own_states.h"
#include "replay/snapshot.h"
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

enum { STATES_BEGIN = 0x4a38, STATES_END = 0x4a39, STATE_ENTRY = 0x4a43,
       STATE_WIRE_SIZE = 20 };
static void write_word(unsigned char *p, uint32_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}
static uint32_t read_word(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
int own_states_save(uint32_t now, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    size_t i;
    if (!out) return 0;
    snapshot_write(&writer, STATES_BEGIN, NULL, 0);
    for (i = 0; i < count && !writer.failed; ++i) {
        const struct OwnStateEntry *entry = &entries[i];
        unsigned char wire[STATE_WIRE_SIZE];
        uint32_t end = entry->end_tick;
        if (end && end != OWN_STATE_NO_END) end -= now;
        write_word(wire, (uint32_t)entry->index);
        write_word(wire + 4, end);
        write_word(wire + 8, (uint32_t)entry->value);
        write_word(wire + 12, (uint32_t)entry->total_ms);
        write_word(wire + 16, (uint32_t)entry->remain_ms);
        snapshot_write(&writer, STATE_ENTRY, wire, sizeof wire);
    }
    snapshot_write(&writer, STATES_END, NULL, 0);
    return snapshot_finish(&writer, out);
}
int own_states_load(const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    struct OwnStateEntry *loaded = NULL;
    size_t loaded_count = 0, i = 0;
    int used = -1;
    if (!snapshot_open(&reader, data, size, STATES_BEGIN)) return -1;
    while (snapshot_next(&reader, &record) == 1) {
        if (record.tag == STATES_END) {
            if (record.value.size) return -1;
            used = (int)reader.offset;
            break;
        }
        if (record.tag == STATES_BEGIN) return -1;
        if (record.tag == STATE_ENTRY) {
            if (record.value.size != STATE_WIRE_SIZE) return -1;
            ++loaded_count;
        }
    }
    if (used < 0 || loaded_count > SIZE_MAX / sizeof *loaded) return -1;
    if (loaded_count) {
        loaded = malloc(loaded_count * sizeof *loaded);
        if (!loaded) return -1;
    }
    snapshot_open(&reader, data, (size_t)used, STATES_BEGIN);
    while (snapshot_next(&reader, &record) == 1 && record.tag != STATES_END) {
        const unsigned char *wire = record.value.data;
        if (record.tag != STATE_ENTRY) continue;
        loaded[i].index = (int32_t)read_word(wire);
        loaded[i].end_tick = read_word(wire + 4);
        loaded[i].value = (int32_t)read_word(wire + 8);
        loaded[i].total_ms = (int32_t)read_word(wire + 12);
        loaded[i].remain_ms = (int32_t)read_word(wire + 16);
        ++i;
    }
    free(entries);
    entries = loaded;
    count = capacity = loaded_count;
    return used;
}
