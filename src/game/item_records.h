#ifndef GAMEMODE_ITEM_RECORDS_H
#define GAMEMODE_ITEM_RECORDS_H

#include <string.h>
#include "game/item_info.h"
#include "game/item_list_packets.h"

#define ITEM_FLAG_IDENTIFIED(f) ((f) & 1)
#define ITEM_FLAG_DAMAGED(f)    ((f) >> 1 & 1)
#define ITEM_FLAG_FAVORITE(f)   ((f) >> 2 & 1)

static __inline void item_from_equip_record(struct ItemInfo *item, const unsigned char *r)
{
    const struct EquipRecord *rec = (const struct EquipRecord *)r;

    item_info_construct(item);
    item->type = rec->type;
    item->equip_location = rec->equip_location;
    item->index = rec->index;
    item->worn_location = rec->worn_location;
    memcpy(item->slot, rec->cards, sizeof item->slot);
    item->count = 1;
    item_info_set_item_id(item, rec->item_id);
    item->identified = rec->identified;
    item->damaged = rec->damaged;
    item->refine = rec->refine;
}

static __inline void item_from_equip_record4(struct ItemInfo *item, const unsigned char *r)
{
    const struct EquipRecord4 *rec = (const struct EquipRecord4 *)r;

    item_info_construct(item);
    item->type = rec->type;
    item->equip_location = rec->equip_location;
    item->index = rec->index;
    item->worn_location = rec->worn_location;
    item->count = 1;
    memcpy(item->slot, rec->cards, sizeof item->slot);
    item_info_set_item_id(item, rec->item_id);
    item->identified = ITEM_FLAG_IDENTIFIED(rec->flags);
    item->damaged = ITEM_FLAG_DAMAGED(rec->flags);
    item->refine = rec->refine;
    item->expire_time = rec->expire_time;
    item->bind_on_equip_type = rec->bind_on_equip_type;
    item->view = rec->view;
    item->favorite = ITEM_FLAG_FAVORITE(rec->flags);
}

static __inline void item_from_v5_record(struct ItemInfo *item, const unsigned char *r)
{
    const struct ItemRecordV5 *rec = (const struct ItemRecordV5 *)r;

    item_info_construct(item);
    item->type = rec->type;
    item->index = rec->index;
    item->worn_location = rec->worn_location;
    memcpy(item->slot, rec->cards, sizeof item->slot);
    item->equip_location = rec->equip_location;
    item->count = 1;
    item_info_set_item_id(item, rec->item_id);
    item->identified = ITEM_FLAG_IDENTIFIED(rec->flags);
    item->damaged = ITEM_FLAG_DAMAGED(rec->flags);
    item->refine = rec->refine;
    item->expire_time = rec->expire_time;
    item->bind_on_equip_type = rec->bind_on_equip_type;
    item->view = rec->view;
}

static __inline void item_set_options(struct ItemInfo *item, const struct ItemOption *options)
{
    int i;

    item->option_count = 0;
    for (i = 0; i < 5; i++)
        if (options[i].id != 0)
            item->option_count++;
    for (i = 0; i < item->option_count; i++)
        item->options[i] = options[i];
}

static __inline void item_from_v5_record_full(struct ItemInfo *item, const unsigned char *r)
{
    item_from_v5_record(item, r);
    item->favorite = ITEM_FLAG_FAVORITE(((const struct ItemRecordV5 *)r)->flags);
    item->option_count = 0;
}

static __inline void item_from_v6_record(struct ItemInfo *item, const unsigned char *r)
{
    const struct ItemRecordV6 *rec = (const struct ItemRecordV6 *)r;
    struct ItemOption options[5];

    memcpy(options, rec->options, sizeof options);
    item_info_construct(item);
    item->type = rec->type;
    item->index = rec->index;
    item->worn_location = rec->worn_location;
    memcpy(item->slot, rec->cards, sizeof item->slot);
    item->equip_location = rec->equip_location;
    item->count = 1;
    item_info_set_item_id(item, rec->item_id);
    item->identified = ITEM_FLAG_IDENTIFIED(rec->flags);
    item->damaged = ITEM_FLAG_DAMAGED(rec->flags);
    item->refine = rec->refine;
    item->expire_time = rec->expire_time;
    item->bind_on_equip_type = rec->bind_on_equip_type;
    item->view = rec->view;
    item->favorite = ITEM_FLAG_FAVORITE(rec->flags);
    item_set_options(item, options);
}

static __inline void item_from_v7_record(struct ItemInfo *item, const unsigned char *r)
{
    const struct ItemRecordV7 *rec = (const struct ItemRecordV7 *)r;
    struct ItemOption options[5];

    memcpy(options, rec->options, sizeof options);
    item_info_construct(item);
    item->type = rec->type;
    item->index = rec->index;
    item->worn_location = rec->worn_location;
    memcpy(item->slot, rec->cards, sizeof item->slot);
    item->equip_location = rec->equip_location;
    item->count = 1;
    item_info_set_item_id(item, rec->item_id);
    item->identified = ITEM_FLAG_IDENTIFIED(rec->flags);
    item->damaged = ITEM_FLAG_DAMAGED(rec->flags);
    item->refine = rec->refine;
    item->expire_time = rec->expire_time;
    item->bind_on_equip_type = rec->bind_on_equip_type;
    item->view = rec->view;
    item->favorite = ITEM_FLAG_FAVORITE(rec->flags);
    item_set_options(item, options);
}

static __inline void item_from_normal_record(struct ItemInfo *item, const unsigned char *r, int v5)
{
    const struct NormalRecord *rec = (const struct NormalRecord *)r;
    const struct NormalTail *rest = (const struct NormalTail *)(r + (v5 ? 0xd : 0xb));

    item_info_construct(item);
    item->type = rec->type;
    item->index = rec->index;
    item->worn_location = v5 ? ((const struct NormalRecordV5 *)r)->worn_location : rec->worn_location;
    item->equip_location = 0;
    item->count = rec->count;
    memcpy(item->slot, rest->cards, sizeof item->slot);
    item_info_set_item_id(item, rec->item_id);
    item->damaged = 0;
    item->identified = ITEM_FLAG_IDENTIFIED(rest->flags);
    item->expire_time = rest->expire_time;
    item->favorite = rest->flags >> 1 & 1;
    item->refine = 0;
}

static __inline void item_from_added_record(struct ItemInfo *item, const unsigned char *pkt)
{
    const struct ItemAddedHead *head = (const struct ItemAddedHead *)pkt;

    item_info_construct(item);
    item->index = head->index;
    item->count = head->amount;
    item_info_set_item_id(item, head->item_id);
    item->identified = head->identified;
    item->damaged = head->damaged;
    item->type = head->type;
}

void game_mode_note_hatched_egg(const struct ItemInfo *item);
void game_mode_check_pet_auto_feed(const struct ItemInfo *item);

#endif
