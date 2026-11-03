#include "contents/player_names.h"
#include "core/msg_str.h"
#include "core/client.h"
#include "iteminfo/iteminfo.h"

#include <string.h>
#include "platform/language.h"

#define MSG_PET_NAMED_PREFIX    0x2f4
#define MSG_CARD_TIMES_2        0x1be
#define MSG_CARD_TIMES_3        0x1bf
#define MSG_CARD_TIMES_4        0x1c0
#define MSG_MAKER_UNKNOWN       0x245

static const struct ItemTableEntry *table_entry(const struct ItemInfo *item)
{
    return item_table_get(item_id(item));
}

const char *item_info_get_name(const struct ItemInfo *self)
{
    const char *name = table_entry(self)->name;
    return name ? name : " ";
}

const char *item_info_get_unid_name(const struct ItemInfo *self)
{
    const char *name = table_entry(self)->unidName;
    return name ? name : " ";
}

const struct ItemTextLines *item_info_get_description(const struct ItemInfo *self)
{
    if (self->identified == 1)
        return &table_entry(self)->desc;
    return &table_entry(self)->unidDesc;
}

const char *item_info_get_res_name(const struct ItemInfo *self)
{
    if (self->identified == 1)
        return table_entry(self)->resName;
    return table_entry(self)->unidResName;
}

int item_info_get_slot_count(const struct ItemInfo *self)
{
    return table_entry(self)->slotCount;
}

void item_name_append_sockets(const struct ItemInfo *self, struct String *out)
{
    int count = table_entry(self)->slotCount;
    if (count) string_append_format(out, " [%d]", count);
}

void item_name_append_refine(const struct ItemInfo *self, struct String *out)
{
    string_append_format(out, "+%d ", self->refine);
}

void item_name_append_pet_prefix(const struct ItemInfo *self, struct String *out)
{
    const char *p = msg_str(MSG_PET_NAMED_PREFIX), *start = p;
    (void)self;
    while (*p) {
        if (p[0] == '%' && p[1] == '%') {
            string_append(out, start, (size_t)(p - start + 1));
            p += 2;
            start = p;
        } else ++p;
    }
    string_append(out, start, (size_t)(p - start));
}

unsigned char item_info_has_card(const struct ItemInfo *self)
{
    int key = self->slot[0];
    unsigned char firstSlotIsCard;
    int id;

    firstSlotIsCard = card_affix_has(key);
    id = item_id(self);
    if ((unsigned)(id - 9001) <= 0x1f2 || (unsigned)(id - 10000) <= 999)
        return firstSlotIsCard;
    if (self->slot[0] == ITEM_SLOT_NAMED || self->slot[0] == ITEM_SLOT_FORGED)
        return firstSlotIsCard;
    if (item_info_is_equipment_type(self) != 1)
        return firstSlotIsCard;
    if (firstSlotIsCard)
        return 1;
    if (card_affix_has(self->slot[1]) || card_affix_has(self->slot[2]) || card_affix_has(self->slot[3]))
        return 1;
    return 0;
}

static int take_duplicates(int *slots, int i)
{
    int count = 1, j;

    for (j = i + 1; j < ITEM_SLOT_COUNT; j++) {
        if (slots[j] == slots[i]) {
            slots[j] = 0;
            count++;
        }
    }
    return count;
}

static int times_message(int count)
{
    switch (count) {
    case 2: return MSG_CARD_TIMES_2;
    case 3: return MSG_CARD_TIMES_3;
    case 4: return MSG_CARD_TIMES_4;
    default: return 0;
    }
}

void item_name_append_card_prefixes(const struct ItemInfo *self, struct String *out)
{
    int slots[ITEM_SLOT_COUNT];
    int i;

    slots[0] = self->slot[0];
    slots[1] = self->slot[1];
    slots[2] = self->slot[2];
    slots[3] = self->slot[3];
    for (i = 0; i < ITEM_SLOT_COUNT; i++) {
        const char *prefix;
        int count;

        if (slots[i] <= 0)
            continue;
        prefix = card_affix_get(slots[i])->prefix;
        if (!prefix || !*prefix)
            continue;
        count = take_duplicates(slots, i);
        if (times_message(count))
            string_append_format(out, "%s ", msg_str(times_message(count)));
        string_append_format(out, "%s ", prefix);
    }
}

void item_name_append_card_postfixes(const struct ItemInfo *self, struct String *out)
{
    int slots[ITEM_SLOT_COUNT];
    int i;

    slots[0] = self->slot[0];
    slots[1] = self->slot[1];
    slots[2] = self->slot[2];
    slots[3] = self->slot[3];
    for (i = 0; i < ITEM_SLOT_COUNT; i++) {
        const char *postfix;
        int count;

        if (slots[i] <= 0)
            continue;
        postfix = card_affix_get(slots[i])->postfix;
        if (!postfix || !*postfix)
            continue;
        count = take_duplicates(slots, i);
        string_append_format(out, " %s", postfix);
        if (times_message(count))
            string_append_format(out, " %s", msg_str(times_message(count)));
    }
}

COLORREF item_info_get_name_color(const struct ItemInfo *self)
{
    COLORREF colors[5];
    int count = 0, slotCount, i;

    colors[0] = 0;
    colors[1] = 0xadff2f;
    colors[2] = 0xbfff;
    colors[3] = 0xffa500;
    colors[4] = 0x9400d3;
    if (item_info_has_card(self) != 1)
        return 0;
    slotCount = table_entry(self)->slotCount;
    for (i = 0; i < ITEM_SLOT_COUNT; i++) {
        int key;

        if (i < slotCount)
            continue;
        key = self->slot[i];
        if (card_affix_has(key))
            count++;
    }
    return colors[count];
}

const char *item_info_get_maker_element_text(const struct ItemInfo *self)
{
    switch ((unsigned)self->slot[1] & 0xff) {
    case 0: {
        int id = item_id(self);
        return msg_str(id == 0xa4a || id == 0xa4b ? 0xba0 : 0x1c2);
    }
    case 1: return msg_str(0x1c4);
    case 2: return msg_str(0x1c6);
    case 3: return msg_str(0x1c3);
    case 4: return msg_str(0x1c5);
    default: return "";
    }
}

const char *item_info_get_maker_star_text(const struct ItemInfo *self)
{
    switch (((unsigned)self->slot[1] >> 8) & 0xff) {
    case 5: return msg_str(0x1cb);
    case 10: return msg_str(0x1cc);
    case 15: return msg_str(0x1cd);
    default: return "";
    }
}

const char *item_info_get_maker_name(const struct ItemInfo *self)
{
    struct TextView name = player_name_lookup(((unsigned)self->slot[3] << 16) + (unsigned)self->slot[2]);
    return name.size ? name.data : msg_str(MSG_MAKER_UNKNOWN);
}

struct String *item_info_get_display_name(const struct ItemInfo *self,
                                                     struct String *out)
{
    int id;

    string_clear(out);

    if (self->identified != 1) {
        string_append_cstr(out, item_info_get_unid_name(self));
        return out;
    }

    id = item_id(self);
    if (((unsigned)(id - 9001) <= 0x1f2 || (unsigned)(id - 10000) <= 999) && self->slot[3] == 1) {
        item_name_append_pet_prefix(self, out);
        string_append_cstr(out, item_info_get_name(self));
        return out;
    }

    if (self->slot[0] == ITEM_SLOT_NAMED && self->slot[1] == 0) {
        if (self->refine > 0)
            item_name_append_refine(self, out);
        string_append_cstr(out, item_info_get_maker_name(self));
        string_append_cstr(out, item_info_get_maker_element_text(self));
        string_append_cstr(out, item_info_get_name(self));
        return out;
    }

    if (!item_info_is_equipment_type(self)) {
        string_append_cstr(out, item_info_get_name(self));
        return out;
    }

    if (self->refine > 0)
        item_name_append_refine(self, out);
    if (item_info_has_card(self) == 1) {
        item_name_append_card_prefixes(self, out);
        string_append_cstr(out, item_info_get_name(self));
        item_name_append_card_postfixes(self, out);
        item_name_append_sockets(self, out);
        return out;
    }
    if (self->slot[0] != ITEM_SLOT_NAMED && self->slot[0] != ITEM_SLOT_FORGED) {
        string_append_cstr(out, item_info_get_name(self));
        item_name_append_sockets(self, out);
        return out;
    }
    string_append_cstr(out, item_info_get_maker_star_text(self));
    string_append_cstr(out, item_info_get_maker_name(self));
    string_append_cstr(out, item_info_get_maker_element_text(self));
    string_append_cstr(out, item_info_get_name(self));
    return out;
}
