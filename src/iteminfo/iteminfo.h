#include "iteminfo/item_api.h"
#ifndef ITEMINFO_H
#define ITEMINFO_H

#include "iteminfo/item_types.h"
#include "base.h"
#include "std/std.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

#define ITEM_SLOT_COUNT     4
#define ITEM_OPTION_COUNT   5

#define ITEM_SLOT_NAMED     0xfe
#define ITEM_SLOT_FORGED    0xff

static __inline int item_id(const struct ItemInfo *item)
{
    return atoi(string_cstr(&item->id_text));
}

#include "iteminfo/item_table.h"
#include "iteminfo/item_tables.h"

unsigned char item_info_has_card(const struct ItemInfo *self);
COLORREF item_info_get_name_color(const struct ItemInfo *self);

void item_tables_load(void);
unsigned char read_int_file(struct Vec *out, const char *name);
unsigned char table_token_is_number(const char *text);

struct List *item_search_by_name(struct List *out, struct String query, char cardsOnly, char fuzzy);

#endif
