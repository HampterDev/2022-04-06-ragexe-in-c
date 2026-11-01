#include "iteminfo/item_table_internal.h"
#include "contents/item_info_mgr.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ItemRow {
    int id;
    struct ItemTableEntry entry;
};

static struct {
    struct ItemRow *rows;
    size_t count, capacity;
    struct ItemTableEntry fallback;
    struct ItemInfoData *source;
} items;

static void *grow_array(void *data, size_t *capacity, size_t needed, size_t width)
{
    size_t limit = SIZE_MAX / width, next = *capacity;
    void *grown;
    if (needed <= next)
        return data;
    if (needed > limit)
        abort();
    if (!next)
        next = limit < 8 ? limit : 8;
    while (next < needed)
        next = next > limit / 2 ? limit : next * 2;
    grown = realloc(data, next * width);
    if (!grown)
        abort();
    *capacity = next;
    return grown;
}

static void *copy_array(const void *source, size_t count, size_t width)
{
    void *copy;
    if (!count)
        return NULL;
    if (count > SIZE_MAX / width)
        abort();
    copy = malloc(count * width);
    if (!copy)
        abort();
    memcpy(copy, source, count * width);
    return copy;
}

static void entry_clear(struct ItemTableEntry *entry)
{
    free(entry->unidDesc.data);
    free(entry->desc.data);
    *entry = (struct ItemTableEntry){0};
}

static void entry_copy(struct ItemTableEntry *dest, const struct ItemTableEntry *source)
{
    struct ItemTableEntry copy;
    if (dest == source)
        return;
    copy = *source;
    copy.desc.data = copy_array(source->desc.data, source->desc.count, sizeof *copy.desc.data);
    copy.desc.capacity = copy.desc.count;
    copy.unidDesc.data = copy_array(source->unidDesc.data, source->unidDesc.count, sizeof *copy.unidDesc.data);
    copy.unidDesc.capacity = copy.unidDesc.count;
    entry_clear(dest);
    *dest = copy;
}

static size_t lower_bound(int id)
{
    size_t low = 0, high = items.count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (items.rows[mid].id < id)
            low = mid + 1;
        else
            high = mid;
    }
    return low;
}

static struct ItemTableEntry *lookup(int id)
{
    size_t index = lower_bound(id);
    if (index < items.count && items.rows[index].id == id)
        return &items.rows[index].entry;
    return &items.fallback;
}

const struct ItemTableEntry *item_table_get(int id)
{
    return lookup(id);
}

size_t item_table_count(void)
{
    return items.count;
}

const struct ItemTableEntry *item_table_entry_at(size_t index, int *id)
{
    if (index >= items.count)
        return NULL;
    if (id)
        *id = items.rows[index].id;
    return &items.rows[index].entry;
}

struct ItemTableEntry *item_table_at(int id)
{
    size_t index = lower_bound(id);
    if (index < items.count && items.rows[index].id == id)
        return &items.rows[index].entry;
    if (items.count == SIZE_MAX / sizeof *items.rows)
        abort();
    items.rows = grow_array(items.rows, &items.capacity, items.count + 1, sizeof *items.rows);
    if (index < items.count)
        memmove(items.rows + index + 1, items.rows + index, (items.count - index) * sizeof *items.rows);
    items.rows[index] = (struct ItemRow){0};
    items.rows[index].id = id;
    items.count++;
    return &items.rows[index].entry;
}

void item_table_append_description(struct ItemTableEntry *entry, int identified, const char *line)
{
    struct ItemTextLines *lines = identified ? &entry->desc : &entry->unidDesc;
    if (lines->count == SIZE_MAX / sizeof *lines->data)
        abort();
    lines->data = grow_array(lines->data, &lines->capacity, lines->count + 1, sizeof *lines->data);
    lines->data[lines->count++] = line;
}

void item_table_apply_lua(void)
{
    size_t i, j;
    if (items.source)
        abort();
    items.source = item_info_acquire();
    for (i = 0; i < item_info_count(items.source); ++i) {
        const struct ItemDefinition *item = item_info_entry(items.source, i);
        struct ItemTableEntry *entry = item_table_at(item->id);
        entry->name = item->identified.name;
        entry->resName = item->identified.resource;
        entry->unidName = item->unidentified.name;
        entry->unidResName = item->unidentified.resource;
        for (j = 0; j < item->identified.line_count; ++j)
            item_table_append_description(entry, 1, item->identified.lines[j]);
        for (j = 0; j < item->unidentified.line_count; ++j)
            item_table_append_description(entry, 0, item->unidentified.lines[j]);
        entry->slotCount = item->slot_count;
        entry->class_num = item->class_num;
    }
}

void item_table_set_unknown(void)
{
    struct ItemTableEntry *entry = lookup(0);
    entry->name = "Unknown Item";
    entry->resName = "사과";
    item_table_append_description(entry, 1, "Unknown Item");
    entry->unidName = "Unknown Item";
    entry->unidResName = "사과";
    item_table_append_description(entry, 0, "Unknown Item");
    entry_copy(lookup(-1), entry);
}

void item_table_clear(void)
{
    size_t i;
    for (i = 0; i < items.count; ++i)
        entry_clear(&items.rows[i].entry);
    free(items.rows);
    entry_clear(&items.fallback);
    item_info_release(items.source);
    memset(&items, 0, sizeof items);
}

void item_table_init(void)
{
    atexit(item_table_clear);
}
