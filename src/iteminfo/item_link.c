#include "iteminfo/item_link.h"
#include "iteminfo/item_api.h"
#include "iteminfo/item_table.h"

#include <stdint.h>
#include <string.h>

enum {
    LINK_LOCATION, LINK_EQUIPMENT, LINK_ID, LINK_REFINE, LINK_VIEW, LINK_GRADE,
    LINK_CARD_COUNT, LINK_FIRST_CARD,
    LINK_FIELDS = LINK_FIRST_CARD + ITEM_CARD_SLOTS + 2 + ITEM_OPTION_SLOTS * 3
};

static uint32_t base62_digit(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 36;
    return 0;
}

static int signed_value(uint32_t value)
{
    return value <= INT32_MAX ? (int)value : -1 - (int)(UINT32_MAX - value);
}

static int base62_value(struct TextView text)
{
    uint32_t value = 0;
    size_t i;
    for (i = 0; i < text.size; ++i)
        value = value * 62u + base62_digit((unsigned char)text.data[i]);
    return signed_value(value);
}

static int read_values(struct TextView text, size_t *at, char a, char b,
                       int *values, size_t capacity)
{
    uint32_t value = 0;
    size_t count = 0;

    ++*at;
    for (;;) {
        unsigned char c = *at < text.size ? (unsigned char)text.data[*at] : 0;
        if (c >= '0' && c < 128) {
            value = value * 62u + base62_digit(c);
        } else {
            if (count == capacity) return -1;
            values[count++] = signed_value(value);
            value = 0;
            if (c != a && c != b) return (int)count;
        }
        ++*at;
    }
}

static int decode_fields(struct TextView text, int fields[LINK_FIELDS])
{
    static const char separators[] = "!#$%&'()*+,-/";
    int options[ITEM_OPTION_SLOTS * 3];
    size_t at = 6, accumulated = 0, option_values = 0;
    int current = LINK_ID, card_count = 0, option_count = 0;
    uint32_t value = 0;

    if (text.size < at || memchr(text.data, 0, text.size)) return 0;
    fields[LINK_LOCATION] = base62_value(text_view(text.data, 5));
    fields[LINK_EQUIPMENT] = base62_value(text_view(text.data + 5, 1));
    fields[LINK_CARD_COUNT] = -1;
    while (at <= text.size) {
        unsigned char c = at < text.size ? (unsigned char)text.data[at] : 0;
        const char *separator;

        if (c >= '0' && c < 128) {
            value = value * 62u + base62_digit(c);
            ++accumulated;
            ++at;
            continue;
        }
        separator = strchr(separators, c);
        if (!separator) { ++at; continue; }
        if (accumulated) fields[current] = signed_value(value);
        value = 0;
        accumulated = 0;
        current = (int)(separator - separators);
        if (c == ')') {
            card_count = read_values(text, &at, ')', ')', fields + LINK_FIRST_CARD, ITEM_CARD_SLOTS);
            if (card_count < 0) return 0;
            fields[LINK_CARD_COUNT] = card_count;
        } else if (c == '+') {
            int count;
            if (option_count == ITEM_OPTION_SLOTS) return 0;
            count = read_values(text, &at, ',', '-', options + option_values, 3);
            if (count != 3) return 0;
            option_values += 3;
            ++option_count;
        } else {
            ++at;
        }
    }
    if (option_count) {
        fields[card_count + 8] = option_count;
        memcpy(fields + card_count + 9, options, option_values * sizeof *options);
    }
    return 1;
}

static int unknown_item_name(const char *name)
{
    static const char unknown[] = "unknown item";
    size_t i;
    int trail = 0;

    for (; *name; ++name) {
        for (i = 0; i < sizeof unknown - 1; ++i) {
            unsigned char c = (unsigned char)name[i];
            if ((i || !trail) && c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (c != unknown[i]) break;
        }
        if (i == sizeof unknown - 1) return 1;
        trail = !trail && (signed char)*name < 0;
    }
    return 0;
}

int item_link_read(struct ItemInfo *item, struct TextView text)
{
    int fields[LINK_FIELDS] = {0}, cards, count, i;
    const struct ItemTableEntry *entry;
    const int *options;

    if (!decode_fields(text, fields)) return 0;
    cards = fields[LINK_CARD_COUNT];
    if (cards < -1 || cards > ITEM_CARD_SLOTS) return 0;
    count = fields[cards + 8];
    if (count < 0 || count > ITEM_OPTION_SLOTS) return 0;
    options = fields + cards + 9;
    entry = item_table_get(fields[LINK_ID]);
    if (!entry || !entry->name || unknown_item_name(entry->name)) return 0;

    item_info_set_name(item, entry->name);
    item_info_set_item_id(item, fields[LINK_ID]);
    item->equip_location = fields[LINK_LOCATION];
    if (fields[LINK_EQUIPMENT]) item->type = 4;
    item->refine = fields[LINK_REFINE];
    item->view = fields[LINK_VIEW];
    item->grade = (short)fields[LINK_GRADE];
    for (i = 0; i < cards; ++i) item->slot[i] = fields[LINK_FIRST_CARD + i];
    item->option_count = count;
    for (i = 0; i < count; ++i, options += 3) {
        item->options[i].id = (unsigned short)options[0];
        item->options[i].param = (unsigned char)options[1];
        item->options[i].value = (short)options[2];
    }
    item->identified = 1;
    return 1;
}

int item_link_next(struct TextView text, size_t *cursor, struct ItemLinkSpan *span)
{
    const struct TextView open = {"<ITEML>", 7}, close = {"</ITEML>", 8};

    while (*cursor < text.size) {
        size_t end = text_view_find(text, close, *cursor);
        size_t begin, next;
        if (end == SIZE_MAX) break;
        begin = text_view_find(text_view(text.data, end), open, *cursor);
        *cursor = end + close.size;
        if (begin == SIZE_MAX) continue;
        while ((next = text_view_find(text_view(text.data, end), open, begin + open.size)) != SIZE_MAX)
            begin = next;
        span->begin = begin;
        span->end = *cursor;
        span->payload = text_view(text.data + begin + open.size, end - begin - open.size);
        return 1;
    }
    *cursor = text.size;
    return 0;
}
