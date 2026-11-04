#include "resources/bitmap_internal.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>

#define GIF_LZW_CODES 4096

struct GifInput { const BYTE *data; size_t size, at; };
struct GifImage { unsigned left, top, width, height; BYTE flags; };
struct LzwEntry { BYTE value; int prefix, length; };
struct Lzw { struct LzwEntry *table; unsigned size; int clear; };

struct GifDecoder {
    struct GifInput in;
    struct GifFrames frames;
    struct Lzw lzw;
    DWORD global[256];
    unsigned width, global_count;
    size_t count;
    BYTE background, transparent, transparent_index;
    unsigned short delay, loops;
};

static BYTE gif_byte(const struct GifInput *in, size_t at)
{ return at < in->size ? in->data[at] : 0; }

static BYTE gif_next(struct GifInput *in)
{ return gif_byte(in, in->at++); }

static unsigned gif_u16(const struct GifInput *in, size_t at)
{ return gif_byte(in, at) | (unsigned)gif_byte(in, at + 1) << 8; }

static unsigned gif_read_palette(struct GifInput *in, DWORD *palette, BYTE flags)
{
    unsigned i, count = 2u << (flags & 7);
    memset(palette, 0, 256 * sizeof *palette);
    for (i = 0; i < count; ++i, in->at += 3)
        palette[i] = 0xff000000u | (DWORD)gif_byte(in, in->at) << 16 |
                     (DWORD)gif_byte(in, in->at + 1) << 8 | gif_byte(in, in->at + 2);
    return count;
}

static struct LzwEntry lzw_entry(const struct Lzw *lzw, int code)
{
    struct LzwEntry entry = {0, 0, 0};
    if (code < 0 || (unsigned)code >= lzw->size) return entry;
    if (code < GIF_LZW_CODES) return lzw->table[code];
    if (code < lzw->clear) {
        entry.value = (BYTE)code;
        entry.prefix = -1;
        entry.length = 1;
    }
    return entry;
}

static int lzw_reset(struct Lzw *lzw)
{
    int i;
    for (i = 0; i < lzw->clear && i < GIF_LZW_CODES; ++i) {
        lzw->table[i].value = (BYTE)i;
        lzw->table[i].prefix = -1;
        lzw->table[i].length = 1;
    }
    return (lzw->clear > 0 ? lzw->clear : 0) + 2;
}

static int lzw_add(struct Lzw *lzw, int code, int prev, int next)
{
    struct LzwEntry entry;
    int root = code == next ? prev : code, steps = 0;
    while ((entry = lzw_entry(lzw, root)).prefix != -1) {
        if (++steps > GIF_LZW_CODES) return 0;
        root = entry.prefix;
    }
    if ((unsigned)next < lzw->size) {
        lzw->table[next].value = entry.value;
        lzw->table[next].prefix = prev;
        lzw->table[next].length = lzw_entry(lzw, prev).length + 1;
    }
    return 1;
}

static int lzw_output(const struct Lzw *lzw, int code, BYTE *out, size_t pos, size_t count)
{
    int steps = 0;
    for (;;) {
        struct LzwEntry entry = lzw_entry(lzw, code);
        size_t end = pos + (size_t)entry.length;
        if (end && end - 1 < count) out[end - 1] = entry.value;
        if (entry.prefix == code || ++steps > GIF_LZW_CODES) return 0;
        if (entry.prefix == -1) return 1;
        code = entry.prefix;
    }
}

static int gif_lzw_decode(struct Lzw *lzw, BYTE min_size, const BYTE *src, size_t count,
                          BYTE *out, size_t out_count)
{
    unsigned initial = 1u << ((min_size + 1) & 31), mask = 1;
    int code_size = min_size, prev = -1, last = 0, next;
    size_t at = 0, pos = 0, remaining = count;
    lzw->clear = (int)(1u << (min_size & 31));
    lzw->size = initial;
    memset(lzw->table, 0, GIF_LZW_CODES * sizeof *lzw->table);
    next = lzw_reset(lzw);
    while (remaining) {
        int bits = code_size + 1, code, k;
        unsigned value = 0;
        for (k = 0; k < bits; ++k) {
            if ((at < count ? src[at] : 0) & mask) value |= 1u << (k & 31);
            mask <<= 1;
            if (mask == 0x100) {
                mask = 1;
                ++at;
                if (remaining) --remaining;
            }
        }
        code = (int)value;
        if (code == lzw->clear) {
            code_size = min_size;
            lzw->size = initial;
            next = lzw_reset(lzw);
            prev = -1;
            continue;
        }
        if (code == lzw->clear + 1) return remaining <= 1 ? last : -1;
        if (prev > -1 && code_size < 12) {
            if (code > next || !lzw_add(lzw, code, prev, next)) return -1;
            if (next <= GIF_LZW_CODES) ++next;
            if (code_size < 11 && (unsigned)next == 1u << bits) {
                unsigned grown = 1u << (bits + 1);
                memset(lzw->table + lzw->size, 0, (grown - lzw->size) * sizeof *lzw->table);
                lzw->size = grown;
                code_size = bits;
            }
        }
        prev = code;
        last = lzw_entry(lzw, code).length;
        if (code != -1 && !lzw_output(lzw, code, out, pos, out_count)) return -1;
        pos += (size_t)last;
        if (pos > out_count) pos = out_count + 1;
    }
    return last;
}

static BYTE *gif_join_blocks(struct GifInput *in, size_t *total)
{
    size_t at = in->at, length, sum = 0;
    BYTE *joined, *cursor;
    while ((length = gif_byte(in, at)) != 0) {
        sum += length;
        at += length + 1;
    }
    joined = malloc(sum ? sum : 1);
    if (!joined) return NULL;
    for (cursor = joined; (length = gif_next(in)) != 0; cursor += length, in->at += length) {
        size_t have = in->at < in->size ? in->size - in->at : 0;
        if (have > length) have = length;
        if (have) memcpy(cursor, in->data + in->at, have);
        memset(cursor + have, 0, length - have);
    }
    *total = sum;
    return joined;
}

static BYTE gif_add_frame(struct GifDecoder *gif, const struct GifImage *image, const BYTE *indices,
                          const DWORD *palette, unsigned colors)
{
    struct GifFrame frame;
    size_t bytes = gif->count * sizeof(DWORD);
    int keyed = gif->frames.count && gif->transparent == 1;
    DWORD key = palette[gif->transparent_index < colors ? gif->transparent_index : colors];
    unsigned x, y;
    if (bytes && gif->frames.count >= BITMAP_MAX_BYTES / bytes) return 0;
    frame.pixels = malloc(bytes ? bytes : sizeof(DWORD));
    if (!frame.pixels) return 0;
    if (gif->frames.count)
        memcpy(frame.pixels, gif->frames.data[gif->frames.count - 1].pixels, bytes);
    else
        memset(frame.pixels, (int)palette[gif->background], bytes);
    for (y = 0; y < image->height; ++y, indices += image->width)
        for (x = 0; x < image->width; ++x) {
            DWORD color = palette[indices[x] < colors ? indices[x] : colors];
            unsigned at = (image->top + y) * gif->width + x + image->left;
            if (!(keyed && color == key) && at < gif->count) frame.pixels[at] = color;
        }
    frame.delay = gif->delay;
    if (!gif_frames_append(&gif->frames, frame)) {
        free(frame.pixels);
        return 0;
    }
    return 1;
}

static BYTE gif_image(struct GifDecoder *gif)
{
    struct GifInput *in = &gif->in;
    struct GifImage image;
    DWORD local[256];
    const DWORD *palette = gif->global;
    unsigned colors = gif->global_count;
    size_t total, count;
    BYTE min_size, *joined, *indices, ok = 1;
    image.left = gif_u16(in, in->at);
    image.top = gif_u16(in, in->at + 2);
    image.width = gif_u16(in, in->at + 4);
    image.height = gif_u16(in, in->at + 6);
    image.flags = gif_byte(in, in->at + 8);
    in->at += 9;
    if (image.flags & 0x80) {
        colors = gif_read_palette(in, local, image.flags);
        palette = local;
    }
    min_size = gif_next(in);
    joined = gif_join_blocks(in, &total);
    if (!joined) return 0;
    count = (size_t)image.width * image.height;
    indices = count <= BITMAP_MAX_BYTES ? calloc(count ? count : 1, 1) : NULL;
    if (indices && gif_lzw_decode(&gif->lzw, min_size, joined, total, indices, count) > 0)
        ok = gif_add_frame(gif, &image, indices, palette, colors);
    else if (!indices)
        ok = 0;
    free(joined);
    free(indices);
    return ok;
}

static BYTE gif_has_netscape(const struct GifInput *in, size_t at, size_t length)
{
    size_t i, j;
    for (i = 0; i + 8 <= length; ++i) {
        for (j = 0; j < 8; ++j)
            if (gif_byte(in, at + i + j) != (BYTE)"NETSCAPE"[j]) break;
        if (j == 8) return 1;
    }
    return 0;
}

static void gif_extension(struct GifDecoder *gif)
{
    struct GifInput *in = &gif->in;
    BYTE label = gif_next(in);
    if (label == 0xf9) {
        unsigned delay = gif_u16(in, in->at + 2);
        gif->transparent = (BYTE)(gif_byte(in, in->at + 1) & 1);
        gif->transparent_index = gif_byte(in, in->at + 4);
        gif->delay = (unsigned short)(delay < 10 ? 10 : delay);
        in->at += 5;
    } else if (label == 0xff) {
        size_t length = gif_next(in), name = in->at;
        in->at += length;
        if (gif_has_netscape(in, name, length)) {
            unsigned repeat = gif_u16(in, in->at + 2);
            gif->loops = (unsigned short)(repeat ? repeat + 1 : 0);
            in->at += 4;
        }
    }
    while (gif_byte(in, in->at)) ++in->at;
}

BYTE gif_res_load_gif_data(struct GifRes *self, const BYTE *data, int size, BYTE need24)
{
    struct GifDecoder gif;
    unsigned height;
    BYTE flags, ok = 0;
    if (!self || !data || size < 0) return 0;
    memset(&gif, 0, sizeof gif);
    gif.in.data = data;
    gif.in.size = (size_t)size;
    if (gif_byte(&gif.in, 0) != 'G' || gif_byte(&gif.in, 1) != 'I' || gif_byte(&gif.in, 2) != 'F')
        return 0;
    gif.width = gif_u16(&gif.in, 6);
    height = gif_u16(&gif.in, 8);
    flags = gif_byte(&gif.in, 10);
    gif.background = gif_byte(&gif.in, 11);
    gif.in.at = 13;
    if (need24 == 1 && (gif.width != 24 || height != 24)) return 0;
    gif.count = (size_t)gif.width * height;
    if (gif.count > BITMAP_MAX_BYTES / sizeof(DWORD)) return 0;
    gif.global_count = flags & 0x80 ? gif_read_palette(&gif.in, gif.global, flags) : 256;
    gif.lzw.table = malloc(GIF_LZW_CODES * sizeof *gif.lzw.table);
    if (!gif.lzw.table) return 0;
    while (gif.in.at < gif.in.size) {
        BYTE block = gif_next(&gif.in);
        if (block == ',' && !gif_image(&gif)) break;
        if (block == '!') gif_extension(&gif);
        if (block == ';') {
            ok = gif.frames.count != 0;
            break;
        }
    }
    free(gif.lzw.table);
    if (!ok) {
        gif_frames_destroy(&gif.frames);
        return 0;
    }
    gif_resource_clear(self);
    self->frames = gif.frames;
    self->bmp.width = (int)gif.width;
    self->bmp.height = (int)height;
    self->bmp.pixels = gif.frames.data[0].pixels;
    self->loopCount = gif.loops;
    self->lastTime = timeGetTime();
    self->animated = 1;
    return 1;
}
