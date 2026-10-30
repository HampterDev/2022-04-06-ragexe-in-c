#include "session/session_items_priv.h"
#include "session/session_items.h"

#include <stdlib.h>
#include <string.h>
#include "iteminfo/iteminfo.h"

struct ItemInfo *session_get_worn_counterpart(struct Session *self,
                                                    const struct ItemInfo *item)
{
    struct ItemInfo *slot;

    if (!item)
        return NULL;
    if (item->worn_location != 0) {
        if (item->worn_location != ITEM_LOCATION_AMMO || self->wornArrowIndex == item->index)
            return NULL;
        return find_by_index(&self->itemList, self->wornArrowIndex);
    }
    switch ((unsigned)item->equip_location) {
    case 0x1:       slot = &self->equipSlots[0]; break;
    case 0x2:
    case 0x22:      slot = &self->equipSlots[1]; break;
    case 0x4:       slot = &self->equipSlots[2]; break;
    case 0x88:      slot = &self->equipSlots[3]; break;
    case 0x10:      slot = &self->equipSlots[4]; break;
    case 0x20:      slot = &self->equipSlots[5]; break;
    case 0x40:      slot = &self->equipSlots[6]; break;
    case 0x100: case 0x101:
    case 0x300: case 0x301:
                    slot = &self->equipSlots[8]; break;
    case 0x200: case 0x201:
                    slot = &self->equipSlots[9]; break;
    case 0x1000:    slot = &self->costumeSlots[0]; break;
    case 0x20000:   slot = &self->costumeSlots[1]; break;
    case 0x2000:    slot = &self->costumeSlots[2]; break;
    case 0x100000:  slot = &self->costumeSlots[3]; break;
    case 0x10000:   slot = &self->costumeSlots[4]; break;
    case 0x40000:   slot = &self->costumeSlots[5]; break;
    case 0x80000:   slot = &self->costumeSlots[6]; break;
    case 0x200000:  slot = &self->costumeSlots[7]; break;
    case 0x400: case 0xc00: case 0x1400: case 0x1c00:
                    slot = &self->costumeSlots[8]; break;
    case 0x800: case 0x1800:
                    slot = &self->costumeSlots[9]; break;
    default:
        return NULL;
    }
    return slot->count != 0 ? slot : NULL;
}

static struct ItemInfo *copy_slot_by_index(struct ItemInfo *out, struct ItemInfo *slots,
                                           int index, int *slotOut)
{
    int i;
    for (i = 0; i < 10; i++) {
        if (slots[i].index == index) {
            *slotOut = i;
            return item_info_copy(out, &slots[i]);
        }
    }
    return item_info_construct_with_arg(out);
}

struct ItemInfo *session_find_costume_item(struct Session *self,
                                                        struct ItemInfo *out, int index, int *slotOut)
{
    copy_slot_by_index(out, self->costumeSlots, index, slotOut);
    return out;
}

struct ItemInfo *session_find_equipped_item(struct Session *self,
                                                         struct ItemInfo *out, int index, int *slotOut)
{
    copy_slot_by_index(out, self->equipSlots, index, slotOut);
    return out;
}

struct ItemInfo *session_get_equipped_item(struct Session *self,
                                                  struct ItemInfo *out, unsigned slot)
{
    if (slot == 0xf)
        return session_get_inventory_item(self, out, self->wornArrowIndex);
    if (slot <= 9)
        item_info_copy(out, &self->equipSlots[slot]);
    else
        item_info_construct_with_arg(out);
    return out;
}

struct ItemInfo *session_get_my_trade_item(struct Session *self,
                                                      struct ItemInfo *out, int i)
{
    item_info_copy(out, &self->exchangeA[i]);
    return out;
}

struct ItemInfo *session_get_inventory_item_at(struct Session *self,
                                                        struct ItemInfo *out, int n)
{
    copy_nth(out, &self->itemList, n);
    return out;
}

struct PartyMember *session_party_member_at(struct Session *self,
                                                     struct PartyMember *out, int n)
{
    struct List *list = &self->partyMembers;
    struct ListNode *node;
    int i = 0;

    for (node = list->head->next; node != list->head; node = node->next, i++)
        if (i == n) {
            party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
            return out;
        }
    out->found = 0;
    string_init(&out->name);
    string_init(&out->map_name);
    out->dead = 0;
    return out;
}

struct PartyMember *session_get_friend_by_index(struct Session *self,
                                                            struct PartyMember *out, int n)
{
    struct List *list = &self->friendList;
    struct ListNode *node;
    int i = 0;

    for (node = list->head->next; node != list->head; node = node->next, i++)
        if (i == n)
            return party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
    out->found = 0;
    string_init(&out->name);
    string_init(&out->map_name);
    out->dead = 0;
    return out;
}

WORD session_get_inventory_index_by_id(struct Session *self, int itemId)
{
    int count = (int)self->itemList.size;
    int i;

    for (i = 0; i < count; i++) {
        struct ItemInfo item;
        int found;

        session_get_inventory_item_at(self, &item, i);
        found = item_id(&item) == itemId;
        string_free(&item.name);
        string_free(&item.id_text);
        if (found)
            return (WORD)item.index;
    }
    return 0xffff;
}

struct Vec *session_inventory_indices(struct Session *self,
                                                              struct Vec *out, int itemId, BYTE onlyIdentified)
{
    struct ListNode *node;

    out->first = out->last = out->end = NULL;
    for (node = self->itemList.head->next; node != self->itemList.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        int index;

        if (item_id(item) != itemId)
            continue;
        if (onlyIdentified == 1 && item->identified != 1)
            continue;
        index = item->index;
        vec_push(out, &index, sizeof(int));
    }
    return out;
}

struct ItemInfo *session_get_inventory_item_by_id(struct Session *self,
                                                          struct ItemInfo *out, int itemId, BYTE requireIdentified)
{
    struct ListNode *node;

    if (self->itemList.size == 0)
        return item_info_construct_with_arg(out);
    for (node = self->itemList.head->next; node != self->itemList.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        if (item_id(item) == itemId && (!requireIdentified || item->identified != 0))
            return item_info_copy(out, item);
    }
    return item_info_construct_with_arg(out);
}

int session_inventory_count(struct Session *self, int itemId)
{
    struct ListNode *node;
    int total = 0;

    for (node = self->itemList.head->next; node != self->itemList.head; node = node->next)
        if (item_id(LIST_VALUE(node, struct ItemInfo)) == itemId)
            total += LIST_VALUE(node, struct ItemInfo)->count;
    return total;
}

DWORD session_get_item_dis_color(const struct ItemInfo *item)
{
    DWORD color;

    if (item->identified == 0)
        return 0xffaaaaaa;
    color = item_info_get_name_color(item);
    if (color != 0)
        return color;
    return item->bind_on_equip_type == 2 ? 0xffffff00 : 0xffffffff;
}
