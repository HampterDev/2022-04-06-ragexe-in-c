#include "contents/repute_mgr.h"

void repute_info_entry_destroy(void *value)
{
    struct ReputeInfoEntry *entry = value;

    string_free(&entry->info.name);
}

void repute_group_entry_destroy(void *value)
{
    struct ReputeGroupEntry *entry = value;

    vec_free(&entry->group.ids);
    string_free(&entry->group.text);
}
