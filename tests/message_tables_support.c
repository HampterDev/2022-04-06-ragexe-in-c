#include "iteminfo/iteminfo.h"
#include "iteminfo/item_table_internal.h"
#include "iteminfo/item_tables_internal.h"
#include "session/state.h"
#include <stdlib.h>

struct Session g_session1;

struct ItemTableEntry *item_table_at(int id) { abort(); }
void item_table_clear(void) { abort(); }
void item_table_apply_lua(void) { abort(); }
void item_table_set_unknown(void) { abort(); }

struct CardAffix *card_affix_at(int id) { abort(); }
void card_affix_set_unknown(void) { abort(); }
void card_affix_clear(void) { abort(); }
void metal_process_append(int id, const char *line) { abort(); }
void metal_process_clear(void) { abort(); }
void item_move_info_add(int id, const int flags[8]) { abort(); }
