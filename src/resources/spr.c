#include "resources/sprite.h"
#include "resources/resource_factories.h"
#include "resources/resource_ops.h"
#include "fileio/res_mgr.h"
#include "fileio/file.h"
#include "render/device.h"
#include "render/device_palette.h"
#include "render/texture.h"
#include "render/sprite_atlas.h"
#include "render/renderer.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ui/render/graphic_setting.h"

struct SpriteReader {
    const BYTE *data;
    size_t size, position;
};

static const BYTE *sprite_take(struct SpriteReader *reader, size_t size)
{
    const BYTE *data;
    if (size > reader->size - reader->position)
        return NULL;
    data = reader->data + reader->position;
    reader->position += size;
    return data;
}

static unsigned sprite_u16(const BYTE *data)
{
    return data[0] | (unsigned)data[1] << 8;
}

struct Res *sprite_resource_create(void)
{
    struct SprRes *sprite = calloc(1, sizeof *sprite);
    if (!sprite)
        return NULL;
    res_construct(&sprite->res);
    sprite->res.ops = &sprite_resource_ops;
    return &sprite->res;
}

void sprite_resource_reset(struct SprRes *sprite)
{
    unsigned kind;
    sprite_atlas_invalidate_images(g_renderer, sprite->images[0].items, sprite->images[0].count);
    sprite_atlas_invalidate_palette(g_renderer, sprite->palette);
    for (kind = 0; kind < 2; ++kind) {
        struct SpriteImageList *images = &sprite->images[kind];
        size_t i;
        for (i = 0; i < images->count; ++i) {
            texture_destroy(images->items[i].texture);
            free(images->items[i].pixels);
        }
        free(images->items);
        images->items = NULL;
        images->count = 0;
    }
    memset(sprite->palette, 0, sizeof sprite->palette);
}

static int sprite_decode_rle(BYTE *out, size_t pixels, const BYTE *code, size_t size)
{
    size_t input = 0, output = 0;
    while (input < size) {
        unsigned value = code[input++], count = 1;
        if (!value) {
            if (input == size)
                return 0;
            count = code[input++];
            if (!count) count = 1;
        }
        if (count > pixels - output)
            return 0;
        if (out)
            memset(out + output, value, count);
        output += count;
    }
    return output == pixels;
}

static size_t sprite_encode_pass(BYTE *out, const BYTE *raw, unsigned width, unsigned height,
                                unsigned half_width, unsigned half_height)
{
    unsigned y, x, run = 0;
    unsigned out_width = width / (half_width + 1), out_height = height / (half_height + 1);
    size_t size = 0;
    for (y = 0; y < out_height; ++y) {
        for (x = 0; x < out_width; ++x) {
            BYTE value = raw[(size_t)y * (half_height + 1) * width + x * (half_width + 1)];
            if (!value) {
                if (++run != 255) continue;
            }
            if (run) {
                if (out) { out[size] = 0; out[size + 1] = (BYTE)run; }
                size += 2;
                run = 0;
            }
            if (value) {
                if (out) out[size] = value;
                ++size;
            }
        }
    }
    if (run) {
        if (out) { out[size] = 0; out[size + 1] = (BYTE)run; }
        size += 2;
    }
    return size;
}

static BYTE *sprite_encode(const BYTE *raw, unsigned width, unsigned height, unsigned half_width, unsigned half_height)
{
    size_t size = sprite_encode_pass(NULL, raw, width, height, half_width, half_height);
    BYTE *code = malloc(size ? size : 1);
    if (code)
        sprite_encode_pass(code, raw, width, height, half_width, half_height);
    return code;
}

static int sprite_load_indexed(struct SprImg *image, struct SpriteReader *reader, unsigned version,
                               unsigned width, unsigned height, unsigned half_width, unsigned half_height)
{
    const BYTE *source;
    BYTE *decoded = NULL;
    size_t pixels = (size_t)width * height, size = pixels;
    if (version >= 0x201) {
        source = sprite_take(reader, 2);
        if (!source) return 0;
        size = sprite_u16(source);
    }
    source = sprite_take(reader, size);
    if (!source) return 0;
    if (version >= 0x201) {
        if (!sprite_decode_rle(NULL, pixels, source, size))
            return 0;
        if (!half_width && !half_height) {
            image->pixels = malloc(size);
            if (!image->pixels) return 0;
            memcpy(image->pixels, source, size);
            return 1;
        }
        decoded = malloc(pixels);
        if (!decoded) return 0;
        sprite_decode_rle(decoded, pixels, source, size);
        source = decoded;
    }
    image->pixels = sprite_encode(source, width, height, half_width, half_height);
    free(decoded);
    return image->pixels != NULL;
}

static int sprite_load_rgba(struct SprImg *image, struct SpriteReader *reader, unsigned width, unsigned height)
{
    size_t count = (size_t)width * height;
    const BYTE *source;
    DWORD *pixels;
    unsigned x, y;
    if (count > SIZE_MAX / sizeof *pixels)
        return 0;
    source = sprite_take(reader, count * sizeof *pixels);
    if (!source) return 0;
    if (!image->width || !image->height)
        return 1;
    pixels = malloc(count * sizeof *pixels);
    if (!pixels) return 0;
    for (y = 0; y < height; ++y) {
        DWORD *row = pixels + (size_t)(height - y - 1) * width;
        for (x = 0; x < width; ++x, source += 4)
            row[x] = (DWORD)source[0] << 24 | (DWORD)source[3] << 16 | (DWORD)source[2] << 8 | source[1];
    }
    image->texture = texture_create(image->width, image->height, GFX_FMT_4444);
    if (image->texture && !texture_upload(image->texture, 0, 0, width, height, pixels, 0, 0)) {
        texture_destroy(image->texture);
        image->texture = NULL;
    }
    free(pixels);
    return image->texture != NULL;
}

static int sprite_parse(struct SprRes *sprite, const BYTE *data, size_t size)
{
    struct SpriteReader reader = {data, size, 0};
    const BYTE *header = sprite_take(&reader, 6);
    unsigned version, counts[2], kind;
    unsigned half_width = g_sprHalfWidth != 0, half_height = g_sprHalfHeight != 0;
    if (!header || sprite_u16(header) != 0x5053)
        return 0;
    version = sprite_u16(header + 2);
    if (version < 0x100 || version > 0x201)
        return 0;
    counts[0] = sprite_u16(header + 4);
    counts[1] = 0;
    if (version >= 0x200) {
        header = sprite_take(&reader, 2);
        if (!header) return 0;
        counts[1] = sprite_u16(header);
    }
    if (version >= 0x101) {
        if (size >= 1024 + reader.position)
            c3d_device_convert_palette(&g_3dDevice, sprite->palette, data + size - 1024, 256);
        else if (counts[0])
            return 0;
        if (counts[0]) reader.size -= 1024;
    }
    for (kind = 0; kind < 2; ++kind) {
        unsigned i;
        if (!counts[kind]) continue;
        if (counts[kind] > (reader.size - reader.position) / 4)
            return 0;
        sprite->images[kind].items = calloc(counts[kind], sizeof *sprite->images[kind].items);
        if (!sprite->images[kind].items) return 0;
        sprite->images[kind].count = counts[kind];
        for (i = 0; i < counts[kind]; ++i) {
            struct SprImg *image = &sprite->images[kind].items[i];
            unsigned width, height;
            header = sprite_take(&reader, 4);
            if (!header) return 0;
            width = sprite_u16(header);
            height = sprite_u16(header + 2);
            if (!width || !height || width > SHRT_MAX || height > SHRT_MAX)
                return 0;
            image->width = (short)(width / (half_width + 1));
            image->height = (short)(height / (half_height + 1));
            image->widthHalved = (short)half_width;
            image->heightHalved = (short)half_height;
            if (kind == SPR_PALETTE_IMAGES) {
                if (!sprite_load_indexed(image, &reader, version, width, height, half_width, half_height))
                    return 0;
            } else if (!sprite_load_rgba(image, &reader, width, height))
                return 0;
        }
    }
    return 1;
}

static int sprite_resource_load(struct Res *resource, const char *name)
{
    struct SprRes *sprite = (struct SprRes *)resource, replacement = {0};
    struct File file;
    const BYTE *data;
    int ok = 0;
    if (!name) return 0;
    file_construct(&file);
    if (!file_open(&file, name, CFILE_MODE_READ, 1, 0) || file.size < 0)
        goto done;
    data = file_consume(&file, (size_t)file.size, 1);
    if (!data || !sprite_parse(&replacement, data, (size_t)file.size))
        goto done;
    sprite_resource_reset(sprite);
    memcpy(sprite->palette, replacement.palette, sizeof sprite->palette);
    memcpy(sprite->images, replacement.images, sizeof sprite->images);
    memset(replacement.images, 0, sizeof replacement.images);
    ok = 1;
done:
    sprite_resource_reset(&replacement);
    file_destruct(&file);
    return ok;
}

static void sprite_resource_destroy(struct Res *resource)
{
    sprite_resource_reset((struct SprRes *)resource);
    free(resource);
}

const struct ResourceOps sprite_resource_ops = {
    .destroy = sprite_resource_destroy,
    .load = sprite_resource_load,
};
