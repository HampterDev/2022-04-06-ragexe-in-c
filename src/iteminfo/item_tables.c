#include "iteminfo/item_tables_internal.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct CardRow { int id; struct CardAffix value; };
struct MetalRow { int id; struct ItemTextLines lines; };
static struct { struct CardRow *data; size_t count, capacity; } cards;
static struct { int *data; size_t count, capacity; } books;
static struct { struct MetalRow *data; size_t count, capacity; } metal;
static struct { struct ItemMoveInfo *data; size_t count, capacity; } movement;
static struct CardAffix card_default;
static const struct ItemTextLines empty_lines;
static const struct ItemMoveInfo no_movement;

static void *table_grow(void *data, size_t *capacity, size_t count, size_t width)
{
    size_t limit = SIZE_MAX / width, next = *capacity;
    void *result;
    if (count >= limit)
        abort();
    if (count < next)
        return data;
    next = next ? (next > limit / 2 ? limit : next * 2) : (limit < 8 ? limit : 8);
    result = realloc(data, next * width);
    if (!result)
        abort();
    *capacity = next;
    return result;
}

static size_t card_position(int id)
{
    size_t low = 0, high = cards.count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (cards.data[mid].id < id) low = mid + 1; else high = mid;
    }
    return low;
}

static size_t book_position(int id)
{
    size_t low = 0, high = books.count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (books.data[mid] < id) low = mid + 1; else high = mid;
    }
    return low;
}

static size_t metal_position(int id)
{
    size_t low = 0, high = metal.count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (metal.data[mid].id < id) low = mid + 1; else high = mid;
    }
    return low;
}

static size_t movement_position(int id)
{
    size_t low = 0, high = movement.count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (movement.data[mid].item_id < id) low = mid + 1; else high = mid;
    }
    return low;
}

static struct CardAffix *card_lookup(int id)
{
    size_t at = card_position(id);
    return at < cards.count && cards.data[at].id == id ? &cards.data[at].value : &card_default;
}

const struct CardAffix *card_affix_get(int id)
{
    return card_lookup(id);
}

unsigned char card_affix_has(int id)
{
    size_t at = card_position(id);
    return at < cards.count && cards.data[at].id == id;
}

struct CardAffix *card_affix_at(int id)
{
    size_t at = card_position(id);
    if (at < cards.count && cards.data[at].id == id)
        return &cards.data[at].value;
    cards.data = table_grow(cards.data, &cards.capacity, cards.count, sizeof *cards.data);
    if (at < cards.count)
        memmove(cards.data + at + 1, cards.data + at, (cards.count - at) * sizeof *cards.data);
    cards.data[at] = (struct CardRow){0};
    cards.data[at].id = id;
    cards.count++;
    return &cards.data[at].value;
}

void card_affix_set_unknown(void)
{
    struct CardAffix *entry = card_lookup(0);
    entry->illustName = "sorry";
    entry->prefix = "";
    entry->postfix = "";
    *card_lookup(-1) = *entry;
}

void card_affix_clear(void)
{
    free(cards.data);
    memset(&cards, 0, sizeof cards);
}

unsigned char book_item_has(int id)
{
    size_t at = book_position(id);
    return at < books.count && books.data[at] == id;
}

void book_item_add(int id)
{
    size_t at = book_position(id);
    if (at < books.count && books.data[at] == id)
        return;
    books.data = table_grow(books.data, &books.capacity, books.count, sizeof *books.data);
    if (at < books.count)
        memmove(books.data + at + 1, books.data + at, (books.count - at) * sizeof *books.data);
    books.data[at] = id;
    books.count++;
}

void book_items_clear(void)
{
    free(books.data);
    memset(&books, 0, sizeof books);
}

const struct ItemTextLines *metal_process_get(int id)
{
    size_t at = metal_position(id);
    return at < metal.count && metal.data[at].id == id ? &metal.data[at].lines : &empty_lines;
}

void metal_process_append(int id, const char *line)
{
    size_t at = metal_position(id);
    struct ItemTextLines *lines;
    if (at == metal.count || metal.data[at].id != id) {
        metal.data = table_grow(metal.data, &metal.capacity, metal.count, sizeof *metal.data);
        if (at < metal.count)
            memmove(metal.data + at + 1, metal.data + at, (metal.count - at) * sizeof *metal.data);
        metal.data[at] = (struct MetalRow){0};
        metal.data[at].id = id;
        metal.count++;
    }
    lines = &metal.data[at].lines;
    lines->data = table_grow(lines->data, &lines->capacity, lines->count, sizeof *lines->data);
    lines->data[lines->count++] = line;
}

void metal_process_clear(void)
{
    size_t i;
    for (i = 0; i < metal.count; ++i)
        free(metal.data[i].lines.data);
    free(metal.data);
    memset(&metal, 0, sizeof metal);
}

const struct ItemMoveInfo *item_move_info_get(int id)
{
    size_t at = movement_position(id);
    return at < movement.count && movement.data[at].item_id == id ? &movement.data[at] : &no_movement;
}

void item_move_info_add(int id, const int flags[8])
{
    size_t at = movement_position(id), i;
    if (at < movement.count && movement.data[at].item_id == id)
        return;
    movement.data = table_grow(movement.data, &movement.capacity, movement.count, sizeof *movement.data);
    if (at < movement.count)
        memmove(movement.data + at + 1, movement.data + at, (movement.count - at) * sizeof *movement.data);
    movement.data[at].item_id = id;
    for (i = 0; i < 8; ++i)
        movement.data[at].flags[i] = flags[i] != 0;
    movement.count++;
}

void item_move_info_clear(void)
{
    free(movement.data);
    memset(&movement, 0, sizeof movement);
}

void card_affix_table_init(void) { atexit(card_affix_clear); }
void book_item_table_init(void) { atexit(book_items_clear); }
void metal_process_table_init(void) { atexit(metal_process_clear); }
void item_move_info_table_init(void) { atexit(item_move_info_clear); }
