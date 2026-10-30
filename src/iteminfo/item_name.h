#ifndef ITEMINFO_ITEM_NAME_H
#define ITEMINFO_ITEM_NAME_H
#include "std/string.h"
struct ItemInfo;

const char *item_info_get_name(const struct ItemInfo *self);
const char *item_info_get_unid_name(const struct ItemInfo *self);
const char *item_info_get_maker_name(const struct ItemInfo *self);
const char *item_info_get_maker_element_text(const struct ItemInfo *self);
const char *item_info_get_maker_star_text(const struct ItemInfo *self);

void item_name_append_sockets(const struct ItemInfo *self, struct String *out);
void item_name_append_refine(const struct ItemInfo *self, struct String *out);
void item_name_append_pet_prefix(const struct ItemInfo *self, struct String *out);
void item_name_append_card_prefixes(const struct ItemInfo *self, struct String *out);
void item_name_append_card_postfixes(const struct ItemInfo *self, struct String *out);

struct String *item_info_get_display_name(const struct ItemInfo *self, struct String *out);
#endif
