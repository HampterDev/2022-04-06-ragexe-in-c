#ifndef CONTENTS_ITEM_INFO_MGR_H
#define CONTENTS_ITEM_INFO_MGR_H
#include <stddef.h>

struct ItemText {
    const char *name, *resource;
    const char *const *lines;
    size_t line_count;
};

struct ItemDefinition {
    int id;
    struct ItemText unidentified, identified;
    int slot_count, class_num, is_costume, effect_id;
};

struct ItemInfoData;

int item_info_load(void);
void item_info_mgr_clear(void);
int item_info_is_costume(int id);
int item_info_effect_id(int id);

struct ItemInfoData *item_info_acquire(void);
void item_info_release(struct ItemInfoData *data);
size_t item_info_count(const struct ItemInfoData *data);
const struct ItemDefinition *item_info_entry(const struct ItemInfoData *data, size_t index);

#endif
