#ifndef ITEMINFO_ITEM_TABLES_INTERNAL_H
#define ITEMINFO_ITEM_TABLES_INTERNAL_H
#include "iteminfo/item_tables.h"

struct CardAffix *card_affix_at(int id);
void card_affix_set_unknown(void);
void card_affix_clear(void);
void metal_process_append(int id, const char *line);
void metal_process_clear(void);
void item_move_info_add(int id, const int flags[8]);
void item_move_info_clear(void);
void card_affix_table_init(void);
void metal_process_table_init(void);
void item_move_info_table_init(void);
#endif
