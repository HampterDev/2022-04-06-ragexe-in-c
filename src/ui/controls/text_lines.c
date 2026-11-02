#include "ui/draw.h"
#include "core/text_format.h"
#include "core/utf8.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "ui/controls/item_id_tag.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void text_lines_push_range(struct TextArray *lines, const char *first, const char *last)
{
    if (!text_array_append(lines, text_view(first, last - first))) abort();
}

static size_t next_character(const char *text, size_t pos, size_t end)
{
    return pos < end ? pos + utf8_char_size(text + pos, end - pos) : pos + 1;
}

void text_split_lines(const char *text, struct TextArray *lines, int width)
{
    size_t end, start = 0, pos = 0, brk = 0;
    uint64_t limit;
    if (!text || !*text) return;
    end = strlen(text);
    if (width < 1) width = 1;
    limit = (unsigned)width;
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
}

void text_wrap_bytes(const char *text, struct TextArray *lines, int width, int skip_cr)
{
    size_t end, start = 0, pos = 0, brk = 0;
    uint64_t limit;
    if (!text || !*text) return;
    end = strlen(text);
    if (width < 1) width = 1;
    limit = (unsigned)width;
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
            const char *close = strstr(text + pos, "]");
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
}
