#ifndef ITEMINFO_ITEM_TABLE_H
#define ITEMINFO_ITEM_TABLE_H
#include <stddef.h>

struct ItemTextLines {
    const char **data;
    size_t count, capacity;
};

struct ItemTableEntry {
    const char *name;
    const char *resName;
    struct ItemTextLines desc;
    const char *unidName;
    const char *unidResName;
    struct ItemTextLines unidDesc;
    int slotCount;
    int class_num;
};

const struct ItemTableEntry *item_table_get(int item_id);
size_t item_table_count(void);
const struct ItemTableEntry *item_table_entry_at(size_t index, int *id);

static inline const char *item_table_resource_name(const struct ItemTableEntry *entry, int identified)
{
    return identified ? entry->resName : entry->unidResName;
}

struct ItemInfo;
const struct ItemTextLines *item_info_get_description(const struct ItemInfo *item);
#endif
