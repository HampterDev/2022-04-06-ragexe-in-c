#include "ui/draw.h"
#include "core/text_format.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "ui/controls/item_id_tag.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include <mbstring.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void text_lines_push_range(struct TextArray *lines, const char *first, const char *last)
{
    if (!text_array_append(lines, text_view(first, last - first))) abort();
}

static size_t next_character(const char *text, size_t pos, size_t end)
{
    const char *next = language_char_next(text + pos);
    return next > text + pos && next <= text + end ? (size_t)(next - text) : pos + 1;
}

static void carry_colour(struct TextArray *lines, size_t first, const char *colour, int skip_rgb)
{
    char tag[3];
    size_t i;
    if (!colour) return;
    memcpy(tag, colour, sizeof tag);
    for (i = first + 1; i < lines->size; ++i) {
        struct OwnedText *line = &lines->items[i];
        const char *text = owned_text_cstr(line);
        size_t at = skip_rgb && text_is_colour_code(text, text + line->size) ? 7 : 0;
        char *copy;
        if (line->size > SIZE_MAX - 4) abort();
        copy = malloc(line->size + 4);
        if (!copy) abort();
        memcpy(copy, text, at);
        memcpy(copy + at, tag, 3);
        memcpy(copy + at + 3, text + at, line->size - at + 1);
        free(line->data);
        line->data = copy;
        line->size += 3;
    }
}

void text_split_lines(const char *text, struct TextArray *lines, int width)
{
    size_t end, start = 0, pos = 0, brk = 0, charset, first = lines->size;
    uint64_t limit;
    if (!text || !*text) return;
    end = strlen(text);
    if (width < 1) width = 1;
    limit = (unsigned)width;
    charset = text_find_charset_tag(text, end, g_useCharsetTag);
    while (pos < end) {
        pos = next_character(text, pos, end);
        if (text[pos] == ' ') {
            do brk = ++pos; while (text[pos] == ' ');
        }
        if (pos >= limit || text[pos] == '\n') {
            size_t cut;
            if (text[pos] == '\n') brk = pos++;
            cut = brk > start ? brk : pos;
            text_lines_push_range(lines, text + start, text + cut);
            start = cut;
            limit = (uint64_t)cut + (unsigned)width;
        }
    }
    if (start < pos) text_lines_push_range(lines, text + start, text + pos);
    carry_colour(lines, first, charset < end ? text + charset : NULL, 0);
}

void text_wrap_bytes(const char *text, struct TextArray *lines, int width, int skip_cr)
{
    size_t end, start = 0, pos = 0, brk = 0, charset, first = lines->size;
    uint64_t limit;
    if (!text || !*text) return;
    end = strlen(text);
    if (width < 1) width = 1;
    limit = (unsigned)width;
    charset = text_find_charset_tag(text, end, g_useCharsetTag);
    while (pos < end) {
        size_t before = pos;
        int len, item_id;
        if (skip_cr && text[pos] == '\r') {
            text_lines_push_range(lines, text + start, text + pos);
            for (++pos; pos < end && text[pos] == '\n'; ++pos) {}
            start = pos;
            limit = (uint64_t)pos + (unsigned)width;
            continue;
        }
        if (text_is_colour_code(text + pos, text + end)) {
            pos += 7;
            limit += 7;
            brk = pos;
        } else if (parse_item_id_tag(text + pos, text + end, &len, &item_id)) {
            text_lines_push_range(lines, text + start, text + pos);
            pos += len;
            limit += len;
            start = pos;
            if (item_id) {
                struct ItemInfo item;
                const char *name;
                item_info_construct(&item);
                item_info_set_item_id(&item, item_id);
                name = item_info_get_name(&item);
                text_lines_push_range(lines, name, name + strlen(name));
                item_info_destruct(&item);
            }
        } else {
            pos = next_character(text, pos, end);
        }
        if (text[pos] == ' ') {
            do ++pos; while (text[pos] == ' ');
            brk = pos;
        }
        while (text[pos] == ']') brk = ++pos;
        while (text[pos] == '[') {
            const char *close = (const char *)_mbsstr((const unsigned char *)text + pos,
                                                     (const unsigned char *)"]");
            size_t count;
            if (!close) {
                ++pos;
                continue;
            }
            count = (size_t)(close - text) - pos + 1;
            if ((uint64_t)pos + count < limit) {
                ++pos;
            } else if (count >= (unsigned)width) {
                break;
            } else {
                limit = count > limit ? 0 : limit - count;
                {
                    size_t previous = before, next = before;
                    while (next < pos) {
                        previous = next;
                        next = next_character(text, next, end);
                    }
                    if (previous > start) pos = previous;
                }
                break;
            }
        }
        if (pos >= limit || text[pos] == '\n') {
            if (text[pos] == '\n') brk = ++pos;
            if (brk > start) {
                text_lines_push_range(lines, text + start, text + brk);
                start = brk;
            } else if (pos > start) {
                text_lines_push_range(lines, text + start, text + pos);
                start = pos;
            }
            limit = (uint64_t)start + (unsigned)width;
        }
    }
    if (start < pos) text_lines_push_range(lines, text + start, text + pos);
    carry_colour(lines, first, charset < end ? text + charset : NULL, 1);
}
