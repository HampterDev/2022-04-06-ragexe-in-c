#include "font_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLES 10
#define MAX_CHARS 512
#define MAX_TABLE 8192

struct Bytes {
    unsigned char data[MAX_TABLE];
    size_t length;
};

static void u8(struct Bytes *b, unsigned value)
{
    if (b->length < sizeof b->data)
        b->data[b->length++] = (unsigned char)value;
}

static void u16(struct Bytes *b, unsigned value)
{
    u8(b, value >> 8 & 0xff);
    u8(b, value & 0xff);
}

static void u32(struct Bytes *b, unsigned long value)
{
    u16(b, (unsigned)(value >> 16 & 0xffff));
    u16(b, (unsigned)(value & 0xffff));
}

static void zeros(struct Bytes *b, size_t count)
{
    while (count--)
        u8(b, 0);
}

/* UTF-8 to UTF-16BE, surrogate pairs for characters past the BMP. */
static void utf16(struct Bytes *b, const char *text)
{
    const unsigned char *s = (const unsigned char *)text;
    while (*s) {
        unsigned long code = *s++;
        int extra = code >= 0xf0 ? 3 : code >= 0xe0 ? 2 : code >= 0xc0 ? 1 : 0;
        code &= extra == 3 ? 0x07 : extra == 2 ? 0x0f : extra == 1 ? 0x1f : 0x7f;
        while (extra-- && *s)
            code = code << 6 | (*s++ & 0x3f);
        if (code >= 0x10000) {
            code -= 0x10000;
            u16(b, 0xd800 + (unsigned)(code >> 10));
            u16(b, 0xdc00 + (unsigned)(code & 0x3ff));
        } else {
            u16(b, (unsigned)code);
        }
    }
}

static int chars_of(const struct FixtureFont *font, unsigned *out)
{
    int count = 0, i, j;
    if (font->chars) {
        for (i = 0; font->chars[i] && count < MAX_CHARS; ++i)
            if (font->chars[i] < 0xffff)
                out[count++] = font->chars[i];
    } else {
        for (i = ' '; i <= '~'; ++i)
            out[count++] = (unsigned)i;
    }
    for (i = 1; i < count; ++i)
        for (j = i; j > 0 && out[j - 1] > out[j]; --j) {
            unsigned t = out[j];
            out[j] = out[j - 1];
            out[j - 1] = t;
        }
    for (i = j = 0; i < count; ++i)
        if (!j || out[j - 1] != out[i])
            out[j++] = out[i];
    return j;
}

struct Name {
    unsigned language, id;
    struct Bytes text;
};

static void name_table(struct Bytes *b, const struct FixtureFont *font)
{
    static struct Name names[5];
    int count = 0, i;
    size_t offset = 0;
    const char *subfamily = font->weight >= 600 ? (font->italic ? "Bold Italic" : "Bold")
                          : font->italic ? "Italic" : "Regular";
    memset(names, 0, sizeof names);
    if (font->family || font->raw_family) {
        names[count].language = 0x0409;
        names[count].id = 1;
        if (font->raw_family) {
            memcpy(names[count].text.data, font->raw_family, font->raw_length);
            names[count].text.length = font->raw_length;
        } else {
            utf16(&names[count].text, font->family);
        }
        ++count;
        names[count].language = 0x0409;
        names[count].id = 2;
        utf16(&names[count++].text, subfamily);
    }
    if (font->local_family) {
        names[count].language = font->local_language;
        names[count].id = 1;
        utf16(&names[count++].text, font->local_family);
    }
    u16(b, 0);
    u16(b, (unsigned)count);
    u16(b, 6 + 12 * (unsigned)count);
    for (i = 0; i < count; ++i) {
        u16(b, 3);
        u16(b, 1);
        u16(b, names[i].language);
        u16(b, names[i].id);
        u16(b, (unsigned)names[i].text.length);
        u16(b, (unsigned)offset);
        offset += names[i].text.length;
    }
    for (i = 0; i < count; ++i) {
        memcpy(b->data + b->length, names[i].text.data, names[i].text.length);
        b->length += names[i].text.length;
    }
}

static void cmap_table(struct Bytes *b, const unsigned *chars, int count)
{
    int segments = count + 1, i, power = 1, log = 0;
    while (power * 2 <= segments) {
        power *= 2;
        ++log;
    }
    u16(b, 0);
    u16(b, 1);
    u16(b, 3);
    u16(b, 1);
    u32(b, 12);
    u16(b, 4);
    u16(b, 16 + 8 * (unsigned)segments);
    u16(b, 0);
    u16(b, 2 * (unsigned)segments);
    u16(b, 2 * (unsigned)power);
    u16(b, (unsigned)log);
    u16(b, 2 * (unsigned)(segments - power));
    for (i = 0; i < count; ++i)
        u16(b, chars[i]);
    u16(b, 0xffff);
    u16(b, 0);
    for (i = 0; i < count; ++i)
        u16(b, chars[i]);
    u16(b, 0xffff);
    for (i = 0; i < count; ++i)
        u16(b, (1u - chars[i]) & 0xffff);
    u16(b, 1);
    zeros(b, 2 * (size_t)segments);
}

/* One font's tables, in tag order. */
static void font_tables(const struct FixtureFont *font, struct Bytes *tables, unsigned long *tags)
{
    static const unsigned long tag_list[TABLES] = {
        0x4f532f32, 0x636d6170, 0x676c7966, 0x68656164, 0x68686561,
        0x686d7478, 0x6c6f6361, 0x6d617870, 0x6e616d65, 0x706f7374,
    };
    unsigned chars[MAX_CHARS];
    int count = chars_of(font, chars), i;
    unsigned em = font->em ? (unsigned)font->em : 1000;
    unsigned ascent = font->win_ascent < 0 ? 0 : font->win_ascent ? (unsigned)font->win_ascent : em * 4 / 5;
    unsigned descent = font->win_descent < 0 ? 0 : font->win_descent ? (unsigned)font->win_descent : em / 5;
    unsigned weight = font->weight ? (unsigned)font->weight : 400, advance = em / 2;
    unsigned box_left = em / 10, box_right = em * 2 / 5, box_top = em * 7 / 10;
    struct Bytes *b;
    for (i = 0; i < TABLES; ++i) {
        tags[i] = tag_list[i];
        tables[i].length = 0;
    }
    b = &tables[0]; /* OS/2, version 4 */
    u16(b, 4);
    u16(b, advance);
    u16(b, weight);
    u16(b, 5);
    u16(b, 0);
    zeros(b, 20 + 2 + 10 + 16);
    u8(b, 'T'), u8(b, 'E'), u8(b, 'S'), u8(b, 'T');
    u16(b, font->italic ? 0x01 : weight >= 600 ? 0x20 : 0x40);
    u16(b, count ? chars[0] : 0);
    u16(b, count ? chars[count - 1] : 0);
    u16(b, ascent);
    u16(b, (0x10000 - descent) & 0xffff);
    u16(b, 0);
    u16(b, ascent);
    u16(b, descent);
    u32(b, 1);
    u32(b, 0);
    u16(b, em / 2);
    u16(b, box_top);
    u16(b, 0);
    u16(b, ' ');
    u16(b, 1);
    cmap_table(&tables[1], chars, count);
    b = &tables[2]; /* glyf: glyph 1, a box */
    u16(b, 1);
    u16(b, box_left);
    u16(b, 0);
    u16(b, box_right);
    u16(b, box_top);
    u16(b, 3);
    u16(b, 0);
    u8(b, 1), u8(b, 1), u8(b, 1), u8(b, 1);
    u16(b, box_left), u16(b, 0), u16(b, box_right - box_left), u16(b, 0);
    u16(b, 0), u16(b, box_top), u16(b, 0), u16(b, (0x10000 - box_top) & 0xffff);
    b = &tables[3]; /* head */
    u32(b, 0x00010000);
    u32(b, 0x00010000);
    u32(b, 0);
    u32(b, 0x5f0f3cf5);
    u16(b, 0x000b);
    u16(b, em);
    zeros(b, 16);
    u16(b, 0);
    u16(b, 0);
    u16(b, box_right);
    u16(b, box_top);
    u16(b, (font->weight >= 600 ? 1 : 0) | (font->italic ? 2 : 0));
    u16(b, 8);
    u16(b, 2);
    u16(b, 0);
    u16(b, 0);
    b = &tables[4]; /* hhea */
    u32(b, 0x00010000);
    u16(b, ascent);
    u16(b, (0x10000 - descent) & 0xffff);
    u16(b, 0);
    u16(b, advance);
    u16(b, 0);
    u16(b, 0);
    u16(b, box_right);
    u16(b, 1);
    u16(b, 0);
    zeros(b, 2 + 8);
    u16(b, 0);
    u16(b, 2);
    b = &tables[5]; /* hmtx */
    u16(b, advance), u16(b, 0), u16(b, advance), u16(b, box_left);
    b = &tables[6]; /* loca, short: glyph 0 empty, glyph 1 the box */
    u16(b, 0), u16(b, 0), u16(b, (unsigned)(tables[2].length + 1) / 2);
    if (tables[2].length % 2)
        u8(&tables[2], 0);
    b = &tables[7]; /* maxp 1.0 */
    u32(b, 0x00010000);
    u16(b, 2);
    u16(b, 4);
    u16(b, 1);
    zeros(b, 4);
    u16(b, 2);
    zeros(b, 18);
    name_table(&tables[8], font);
    b = &tables[9]; /* post 3.0 */
    u32(b, 0x00030000);
    u32(b, 0);
    u16(b, 0);
    u16(b, 0);
    u32(b, font->fixed_pitch ? 1 : 0);
    zeros(b, 16);
}

static int put(unsigned char *out, size_t size, size_t *at, const void *bytes, size_t length)
{
    if (*at + length > size)
        return 0;
    memcpy(out + *at, bytes, length);
    *at += length;
    return 1;
}

static void put_u32(unsigned char *at, unsigned long value)
{
    at[0] = (unsigned char)(value >> 24);
    at[1] = (unsigned char)(value >> 16);
    at[2] = (unsigned char)(value >> 8);
    at[3] = (unsigned char)value;
}

size_t font_fixture_build(const struct FixtureFont *fonts, int count, unsigned char *out, size_t size)
{
    static struct Bytes tables[TABLES];
    size_t at = 0, header = count > 1 ? 12 + 4 * (size_t)count : 0;
    unsigned long tags[TABLES];
    int f, i;
    if (count < 1 || header > size)
        return 0;
    if (count > 1) {
        put_u32(out, 0x74746366);
        put_u32(out + 4, 0x00010000);
        put_u32(out + 8, (unsigned long)count);
        at = header;
    }
    for (f = 0; f < count; ++f) {
        size_t directory = at, offset = at + 12 + 16 * TABLES;
        struct Bytes head = {{0}, 0};
        if (count > 1)
            put_u32(out + 12 + 4 * f, (unsigned long)directory);
        font_tables(&fonts[f], tables, tags);
        u32(&head, 0x00010000);
        u16(&head, TABLES), u16(&head, 128), u16(&head, 3), u16(&head, 16 * TABLES - 128);
        for (i = 0; i < TABLES; ++i) {
            u32(&head, tags[i]);
            u32(&head, 0);
            u32(&head, (unsigned long)offset);
            u32(&head, (unsigned long)tables[i].length);
            offset += (tables[i].length + 3) & ~(size_t)3;
        }
        if (!put(out, size, &at, head.data, head.length))
            return 0;
        for (i = 0; i < TABLES; ++i) {
            static const unsigned char pad[3];
            if (!put(out, size, &at, tables[i].data, tables[i].length)
                || !put(out, size, &at, pad, ((tables[i].length + 3) & ~(size_t)3) - tables[i].length))
                return 0;
        }
    }
    return at;
}

int font_fixture_write(const char *path, const unsigned char *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    int ok;
    if (!file)
        return 0;
    ok = fwrite(bytes, 1, size, file) == size;
    return fclose(file) == 0 && ok;
}

int font_fixture_save(const char *path, const struct FixtureFont *fonts, int count)
{
    static unsigned char bytes[1 << 17];
    size_t size = font_fixture_build(fonts, count, bytes, sizeof bytes);
    return size && font_fixture_write(path, bytes, size);
}
