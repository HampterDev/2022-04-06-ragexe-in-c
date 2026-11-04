#include "iteminfo/iteminfo.h"

#include <string.h>
#include "game/drag_info.h"
#include "std/vec.h"

void item_info_clear(struct ItemInfo *self)
{
    self->type = 0;
    self->index = 0;
    self->equip_location = 0;
    self->worn_location = 0;
    self->count = 0;
    self->price = 0;
    self->charged_price = 0;
    memset(self->slot, 0, sizeof self->slot);
    string_clear(&self->id_text);
    string_clear(&self->name);
    self->identified = 0;
    self->damaged = 0;
    self->bind_on_equip_type = 0;
    self->expire_in_hours = 0;
    self->expire_time = 0;
    self->weight = 0;
    self->view = 0;
    self->favorite = 0;
    self->is_new = 0;
    self->move_pending = 0;
    self->option_count = 0;
    self->refine = 0;
    memset(self->options, 0, sizeof self->options);
    self->by_index = 0;
    self->sale_kind = 0;
    self->sale_start_time = 0;
    self->sale_end_time = 0;
    self->sale_remaining = 0;
}

struct ItemInfo *item_info_construct(struct ItemInfo *self)
{
    string_init(&self->id_text);
    string_init(&self->name);
    item_info_clear(self);
    return self;
}

struct ItemInfo *item_info_construct_with_arg(struct ItemInfo *self)
{
    string_init(&self->id_text);
    string_init(&self->name);
    item_info_clear(self);
    return self;
}

struct ItemInfo *item_info_assign(struct ItemInfo *self, const struct ItemInfo *src)
{
    self->type = src->type;
    self->index = src->index;
    self->equip_location = src->equip_location;
    self->worn_location = src->worn_location;
    self->count = src->count;
    self->price = src->price;
    self->charged_price = src->charged_price;
    memmove(self->slot, src->slot, sizeof self->slot);
    string_assign(&self->id_text, string_cstr(&src->id_text), src->id_text.size);
    string_assign(&self->name, string_cstr(&src->name), src->name.size);
    self->identified = src->identified;
    self->damaged = src->damaged;
    self->refine = src->refine;
    self->bind_on_equip_type = src->bind_on_equip_type;
    self->expire_in_hours = src->expire_in_hours;
    self->expire_time = src->expire_time;
    self->weight = src->weight;
    self->view = src->view;
    self->favorite = src->favorite;
    self->is_new = src->is_new;
    self->move_pending = src->move_pending;
    self->option_count = src->option_count;
    memmove(self->options, src->options, sizeof self->options);
    self->by_index = src->by_index;
    self->sale_kind = src->sale_kind;
    self->sale_remaining = src->sale_remaining;
    self->sale_start_time = src->sale_start_time;
    self->sale_end_time = src->sale_end_time;
    return self;
}

void item_record_a_clear(struct ItemRecordA *record)
{
    memset(record, 0, sizeof *record);
}

struct ItemInfo *item_info_assign_from_record_a(struct ItemInfo *self,
                                                       const struct ItemRecordA *src)
{
    self->index = src->index;
    self->price = src->price;
    self->charged_price = src->charged_price;
    self->type = src->type;
    self->identified = src->identified;
    string_assign_cstr(&self->id_text, src->id_text);
    self->view = src->view;
    self->equip_location = src->equip_location;
    self->by_index = src->index != -1;
    return self;
}

struct ItemInfo *item_info_assign_from_drag(struct ItemInfo *self, const struct DragInfo *drag)
{
    int count, i;

    self->type = drag->type;
    self->index = drag->index;
    self->count = drag->drop_count;
    self->price = drag->price;
    self->charged_price = drag->charged_price;
    self->slot[0] = drag->slots[0];
    self->slot[1] = drag->slots[1];
    self->slot[2] = drag->slots[2];
    self->slot[3] = drag->slots[3];
    string_assign(&self->id_text, string_cstr(&drag->name), drag->name.size);
    self->identified = drag->identified;
    self->refine = drag->refine;
    count = drag->option_count;
    self->option_count = count <= ITEM_OPTION_COUNT ? count : 0;
    for (i = 0; i < self->option_count; i++)
        memcpy(&self->options[i], &drag->options[i], sizeof self->options[i]);
    self->by_index = drag->by_index;
    self->equip_location = drag->equip_location;
    self->view = (int)drag->view;
    return self;
}

int item_info_less_by_name(const struct ItemInfo *self, const struct ItemInfo *other)
{
    const unsigned char *a = (const unsigned char *)string_cstr(&self->name);
    const unsigned char *b = (const unsigned char *)string_cstr(&other->name);
    size_t na = self->name.size, nb = other->name.size;
    size_t n = nb < na ? nb : na, i;

    for (i = 0; i < n; i++)
        if (a[i] != b[i])
            return a[i] < b[i];
    return na < nb;
}

unsigned char item_info_set_name(struct ItemInfo *self, const char *text)
{
    string_assign_cstr(&self->name, text);
    return self->name.size == 0;
}

void item_info_set_item_id(struct ItemInfo *self, int itemId)
{
    char text[16] = {0};

    snprintf(text, sizeof text, "%d", itemId);
    string_assign_cstr(&self->id_text, text);
}

unsigned char item_info_is_equipment_type(const struct ItemInfo *self)
{
    switch (self->type) {
    case 4: case 5: case 8: case 9: case 11: case 12: case 13: case 14: case 15: case 21:
        return 1;
    default:
        return 0;
    }
}

unsigned char item_info_has_maker_slot(const struct ItemInfo *self)
{
    return self->slot[0] == ITEM_SLOT_NAMED || self->slot[0] == ITEM_SLOT_FORGED;
}

unsigned char item_info_is_pet_range_id(const struct ItemInfo *self)
{
    int id = item_id(self);

    return (unsigned)(id - 9001) <= 0x1f2 || (unsigned)(id - 10000) <= 999;
}

unsigned char item_info_is_element_stone_or_star_crumb(const struct ItemInfo *self)
{
    if (item_id(self) == 1000)
        return 1;
    return (unsigned)(item_id(self) - 994) <= 3;
}

unsigned char item_info_is_element_stone(const struct ItemInfo *self)
{
    return (unsigned)(item_id(self) - 994) <= 3;
}

unsigned char item_info_id_above600(const struct ItemInfo *self)
{
    if (item_id(self) <= 500)
        return 0;
    if (item_id(self) <= 600)
        return 0;
    return 1;
}

int item_info_get_item_id(struct ItemInfo *self)
{
    return atoi(string_cstr(&self->id_text));
}

struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *src)
{
    self->type = src->type;
    self->index = src->index;
    self->equip_location = src->equip_location;
    self->worn_location = src->worn_location;
    self->count = src->count;
    self->price = src->price;
    self->charged_price = src->charged_price;
    memcpy(self->slot, src->slot, sizeof self->slot);
    string_copy(&self->id_text, &src->id_text);
    string_copy(&self->name, &src->name);
    self->identified = src->identified;
    self->damaged = src->damaged;
    self->refine = src->refine;
    self->bind_on_equip_type = src->bind_on_equip_type;
    self->expire_in_hours = src->expire_in_hours;
    self->expire_time = src->expire_time;
    self->weight = src->weight;
    self->view = src->view;
    self->favorite = src->favorite;
    self->is_new = src->is_new;
    self->move_pending = src->move_pending;
    self->by_index = src->by_index;
    self->sale_kind = src->sale_kind;
    self->sale_start_time = src->sale_start_time;
    self->sale_end_time = src->sale_end_time;
    self->sale_remaining = src->sale_remaining;
    self->option_count = src->option_count;
    memcpy(self->options, src->options, sizeof self->options);
    self->guild_log = src->guild_log;
    return self;
}

void item_info_vec_clear(struct Vec *v)
{
    struct ItemInfo *p;

    for (p = v->first; p != v->last; p++)
        item_info_destruct(p);
    vec_clear(v);
}

void item_info_vec_free(struct Vec *v)
{
    item_info_vec_clear(v);
    vec_free(v);
}
