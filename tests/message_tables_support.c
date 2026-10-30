#include "iteminfo/iteminfo.h"
#include "iteminfo/item_table_internal.h"
#include "iteminfo/item_tables_internal.h"
#include "session/state.h"
#include <stdlib.h>

struct Session g_session1;
unsigned char file_make_file_name(const char *name, int flags) { abort(); }
void error_msg(const char *text, int flags) { abort(); }
void session_remove_underbar(const char *src, char *dst) { abort(); }

struct ItemTableEntry *item_table_at(int id) { abort(); }
void item_table_clear(void) { abort(); }
void item_table_apply_lua(void) { abort(); }
void item_table_set_unknown(void) { abort(); }
void item_table_append_description(struct ItemTableEntry *entry, int identified, const char *line) { abort(); }
void item_table_use_identified_description(struct ItemTableEntry *entry) { abort(); }
void item_table_append_params(struct ItemTableEntry *entry, int first, int second) { abort(); }

struct CardAffix *card_affix_at(int id) { abort(); }
void card_affix_set_unknown(void) { abort(); }
void card_affix_clear(void) { abort(); }
void book_item_add(int id) { abort(); }
void book_items_clear(void) { abort(); }
void metal_process_append(int id, const char *line) { abort(); }
void metal_process_clear(void) { abort(); }
void item_move_info_add(int id, const int flags[8]) { abort(); }
