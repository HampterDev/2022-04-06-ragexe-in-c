#ifndef ITEMINFO_ITEM_API_H
#define ITEMINFO_ITEM_API_H

#include "iteminfo/item_types.h"
#include "iteminfo/item_name.h"

struct ItemRecordA;
struct DragInfo;

struct ItemInfo *item_info_construct(struct ItemInfo *self);
struct ItemInfo *item_info_construct_with_arg(struct ItemInfo *self);
void item_info_clear(struct ItemInfo *self);
struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *src);
struct ItemInfo *item_info_assign(struct ItemInfo *self, const struct ItemInfo *src);
void item_record_a_clear(struct ItemRecordA *record);
struct ItemInfo *item_info_assign_from_record_a(struct ItemInfo *self,
                                                       const struct ItemRecordA *src);
struct ItemInfo *item_info_assign_from_drag(struct ItemInfo *self, const struct DragInfo *drag);
void item_info_set_item_id(struct ItemInfo *self, int itemId);
int item_info_get_slot_count(const struct ItemInfo *self);
const char *item_info_get_res_name(const struct ItemInfo *self);
unsigned char item_info_set_name(struct ItemInfo *self, const char *text);
unsigned char item_info_is_equipment_type(const struct ItemInfo *self);
unsigned char item_info_has_maker_slot(const struct ItemInfo *self);
unsigned char item_info_is_pet_range_id(const struct ItemInfo *self);
unsigned char item_info_is_element_stone_or_star_crumb(const struct ItemInfo *self);
unsigned char item_info_is_element_stone(const struct ItemInfo *self);
unsigned char item_info_id_above600(const struct ItemInfo *self);
void item_info_set_option(struct ItemInfo *self, int idAndValue, BYTE param,
                                   const int *index);
void item_info_set_slot(struct ItemInfo *self, const int *cardId, const int *index);
int item_info_less_by_name(const struct ItemInfo *self, const struct ItemInfo *other);
int item_info_get_item_id(struct ItemInfo *self);

void item_info_vec_clear(struct Vec *v);
void item_info_vec_free(struct Vec *v);

static __inline void item_info_destruct(struct ItemInfo *self)
{
    string_free(&self->name);
    string_free(&self->id_text);
}

#endif
