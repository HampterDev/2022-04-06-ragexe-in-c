#ifndef ITEMINFO_ITEM_TABLE_INTERNAL_H
#define ITEMINFO_ITEM_TABLE_INTERNAL_H
#include "iteminfo/item_table.h"

struct ItemTableEntry *item_table_at(int id);
void item_table_append_description(struct ItemTableEntry *entry, int identified, const char *line);
void item_table_apply_lua(void);
void item_table_set_unknown(void);
void item_table_clear(void);
void item_table_init(void);

void table_reader_init(void);

#endif
