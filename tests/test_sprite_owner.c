#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "fileio/file.h"
#include "fileio/file_api.h"
#include "render/device.h"
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *data;
    assert(!size || count <= SIZE_MAX / size);
    data = checked_malloc(count * size);
    if (data) memset(data, 0, count * size);
    return data;
}
#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/resources/spr.c"
#include "../src/resources/palette.c"
#include "../src/render/rp_atlas.c"
#undef malloc
#undef calloc
#undef free

struct GameFileMgr g_fileMgr;
struct C3dDevice g_3dDevice;
int g_sprHalfWidth, g_sprHalfHeight;
struct Renderer *g_renderer;
unsigned g_resPurgeAgeMs = 1000;
int g_serviceType;
static unsigned atlas_uploads;
static BYTE *input;
static int input_size, file_present = 1, borrowed_buffers, fail_texture, fail_upload;
static unsigned textures, uploads, guarded_prefixes;
struct TestTexture { struct Texture texture; unsigned width, height; DWORD *pixels; };

struct Res *res_construct(struct Res *resource)
{ memset(resource, 0, sizeof *resource); resource->typeIndex = -1; return resource; }
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }
BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *name, int *size, BYTE disk)
{
    assert(mgr == &g_fileMgr && name && !disk && !borrowed_buffers);
    if (!file_present) { *size = 0; return NULL; }
    *size = input_size; ++borrowed_buffers; return input;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{ assert(mgr == &g_fileMgr && data == input && borrowed_buffers == 1); --borrowed_buffers; }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *name, BYTE disk)
{ (void)mgr; (void)name; (void)disk; return (BYTE)file_present; }

void surface_destroy(struct Surface *surface)
{
    struct TestTexture *texture = (struct TestTexture *)surface;
    assert(textures); --textures;
    free(texture->pixels);
    checked_free(texture);
}
int texture_upload_pixels(struct Texture *base, int x, int y, int width, int height,
                           const DWORD *pixels, int keyed, int pitch)
{
    struct TestTexture *texture = (struct TestTexture *)base;
    size_t size = (size_t)width * height * sizeof *pixels;
    assert(!x && !y && !keyed && !pitch && width > 0 && height > 0 && !texture->pixels);
    if (fail_upload) return 0;
    texture->pixels = malloc(size); assert(texture->pixels);
    memcpy(texture->pixels, pixels, size);
    texture->width = width; texture->height = height; ++uploads;
    return 1;
}
struct Texture *texture_create(int width, int height, int format)
{
    struct TestTexture *texture;
    assert(width > 0 && height > 0 && (format == GFX_FMT_4444 || format == GFX_FMT_1555));
    if (fail_texture) return NULL;
    texture = checked_calloc(1, sizeof *texture);
    if (!texture) return NULL;
    texture->texture.base.width = texture->texture.reqWidth = width;
    texture->texture.base.height = texture->texture.reqHeight = height;
    texture->texture.format = format;
    ++textures;
    return &texture->texture;
}
int texture_draw_sprite_frame(struct Texture *texture, int x, int y, int w, int h,
                             const struct SprImg *image, const DWORD *palette)
{
    struct TestTexture *capture = (struct TestTexture *)texture;
    size_t count = (size_t)texture->base.width * texture->base.height;
    assert(w >= image->width && h >= image->height && image->pixels && palette);
    assert(x >= 0 && y >= 0 && x + w <= texture->base.width && y + h <= texture->base.height);
    if (!capture->pixels) { capture->pixels = calloc(count, sizeof *capture->pixels); assert(capture->pixels); }
    capture->pixels[(size_t)y * texture->base.width + x] = palette[image->pixels[0]];
    ++atlas_uploads; return 1;
}
static DWORD atlas_pixel(struct Texture *texture, const struct SpriteRegion *region)
{ return ((struct TestTexture *)texture)->pixels[(size_t)region->y * texture->base.width + region->x]; }
static void set_input(const void *data, size_t size)
{ assert(size <= INT_MAX && !borrowed_buffers); input = (BYTE *)data; input_size = (int)size; }
static void clean(void) { assert(!borrowed_buffers && !outstanding && !textures); }
static void set_half(unsigned setting)
{ g_sprHalfWidth = (setting & 1) != 0; g_sprHalfHeight = (setting & 2) != 0; }
static unsigned word(const BYTE *p) { return p[0] + p[1] * 256u; }
static void put_word(BYTE *p, unsigned n) { p[0] = (BYTE)n; p[1] = (BYTE)(n >> 8); }

static const BYTE indexed_pixels[] = {1,0,0,0,2,3,4,0,5,6,7,8,9,0,10};
static const BYTE indexed_code[] = {1,0,3,2,3,4,0,1,5,6,7,8,9,0,1,10};
static const BYTE rgba_file[] = {0,1,2,3, 16,4,5,6, 32,7,8,9,
                               64,10,11,12, 128,13,14,15, 255,16,17,18};
static const DWORD rgba_pixels[] = {0x400c0b0a,0x800f0e0d,0xff121110, 0x00030201,0x10060504,0x20090807};
static size_t fixture(BYTE *data, unsigned version)
{
    size_t n = version >= 0x200 ? 8 : 6; unsigned i;
    memcpy(data, "SP", 2); put_word(data + 2, version); put_word(data + 4, 1);
    if (version >= 0x200) put_word(data + 6, 1);
    put_word(data + n, 5); put_word(data + n + 2, 3); n += 4;
    if (version >= 0x201) {
        put_word(data + n, sizeof indexed_code); n += 2;
        memcpy(data + n, indexed_code, sizeof indexed_code); n += sizeof indexed_code;
    } else { memcpy(data + n, indexed_pixels, sizeof indexed_pixels); n += sizeof indexed_pixels; }
    if (version >= 0x200) {
        put_word(data + n, 3); put_word(data + n + 2, 2); n += 4;
        memcpy(data + n, rgba_file, sizeof rgba_file); n += sizeof rgba_file;
    }
    if (version >= 0x101) {
        for (i = 0; i < 1024; ++i) data[n + i] = (BYTE)(i * 31 + i / 4);
        data[n] = data[n + 2] = 254; data[n + 1] = 3;
        n += 1024;
    }
    return n;
}
static void check_palette(const struct SprRes *sprite, const BYTE *raw)
{
    unsigned i;
    for (i = 0; i < 256; ++i) {
        DWORD expected = 0;
        if (raw) {
            const BYTE *c = raw + i * 4;
            expected = c[0] >= 254 && c[1] < 4 && c[2] >= 254 ? g_3dDevice.colorKeyPixel
                : 0x8000u | (DWORD)(c[0] >> 3) << 10 | (DWORD)(c[1] >> 3) << 5 | c[2] >> 3;
        }
        assert(sprite->palette[i] == expected);
    }
}
static void check_indexed(const struct SprImg *image, unsigned width, unsigned height, const BYTE *raw)
{
    size_t at = 0, produced = 0, count = (size_t)image->width * image->height;
    const struct Allocation *allocation = (const struct Allocation *)image->pixels - 1;
    assert(image->width == width / (g_sprHalfWidth + 1));
    assert(image->height == height / (g_sprHalfHeight + 1));
    assert(image->widthHalved == g_sprHalfWidth && image->heightHalved == g_sprHalfHeight);
    assert(!image->texture && image->pixels);
    while (produced < count) {
        unsigned value, run = 1;
        assert(at < allocation->size); value = image->pixels[at++];
        if (!value) { assert(at < allocation->size); run = image->pixels[at++]; if (!run) run = 1; }
        assert(run <= count - produced);
        while (run--) {
            size_t x = produced % image->width, y = produced / image->width;
            assert(value == raw[y * (g_sprHalfHeight + 1) * width + x * (g_sprHalfWidth + 1)]);
            ++produced;
        }
    }
    if (count) assert(at == allocation->size);
}
static void check_rgba(const struct SprImg *image, unsigned width, unsigned height, const DWORD *pixels)
{
    const struct TestTexture *texture = (const struct TestTexture *)image->texture;
    assert(image->width == width / (g_sprHalfWidth + 1) && image->height == height / (g_sprHalfHeight + 1));
    assert(image->widthHalved == g_sprHalfWidth && image->heightHalved == g_sprHalfHeight && !image->pixels);
    if (!image->width || !image->height) { assert(!texture); return; }
    assert(texture && texture->width == width && texture->height == height);
    assert(texture->texture.reqWidth == image->width && texture->texture.reqHeight == image->height);
    assert(!memcmp(texture->pixels, pixels, (size_t)width * height * 4));
}
static void test_versions(void)
{
    const unsigned versions[] = {0,0xff,0x100,0x101,0x200,0x201};
    BYTE data[1200]; unsigned v, half;
    for (v = 0; v < sizeof versions / sizeof *versions; ++v) for (half = 0; half < 4; ++half) {
        size_t size = fixture(data, versions[v]);
        struct Res *base = sprite_resource_create(); struct SprRes *sprite = (struct SprRes *)base;
        assert(base && base->ops == &sprite_resource_ops); set_half(half); set_input(data, size);
        assert(resource_load(base, "fixture.spr") && !borrowed_buffers);
        assert(sprite->images[0].count == 1 && sprite->images[1].count == (versions[v] >= 0x200));
        check_palette(sprite, versions[v] >= 0x101 ? data + size - 1024 : NULL);
        check_indexed(sprite_resource_image(sprite, 0, 0), 5, 3, indexed_pixels);
        if (versions[v] >= 0x200) check_rgba(sprite_resource_image(sprite, 1, 0), 3, 2, rgba_pixels);
        assert(!sprite_resource_image(NULL,0,0) && !sprite_resource_image(sprite,-1,0));
        assert(!sprite_resource_image(sprite,2,0) && !sprite_resource_image(sprite,0,-1));
        assert(!sprite_resource_image(sprite,0,1));
        sprite_resource_reset(sprite); sprite_resource_reset(sprite);
        assert(!sprite->images[0].items && !sprite->images[1].count && outstanding == 1 && !textures);
        assert(!sprite_resource_image(sprite,0,0)); resource_destroy(base); clean();
    }
}
static void test_rejected_replacement(void)
{
    BYTE good[1200], bad[1200]; size_t size = fixture(good, 0x201); unsigned k;
    struct Res *base = sprite_resource_create(); struct SprRes saved;
    const unsigned offsets[] = {0,2,4,6,10,12,30};
    const unsigned values[] = {0,0x202,65535,65535,0x8000,65535,0x8000};
    set_half(0); set_input(good,size); assert(resource_load(base,"old.spr")); saved = *(struct SprRes *)base;
    for (k = 0; k < sizeof offsets / sizeof *offsets; ++k) {
        memcpy(bad,good,size); put_word(bad+offsets[k],values[k]); set_input(bad,size);
        assert(!resource_load(base,"bad.spr") && !memcmp(base,&saved,sizeof saved));
        assert(outstanding == 5 && textures == 1 && !borrowed_buffers);
    }
    set_input(good,size); file_present = 0;
    assert(!resource_load(base,"missing.spr")); file_present = 1;
    assert(!resource_load(base,NULL)); input_size = -1;
    assert(!resource_load(base,"negative.spr")); set_input(good,size); fail_texture = 1;
    assert(!resource_load(base,"graphics-failure.spr")); fail_texture = 0;
    fail_upload = 1;
    assert(!resource_load(base,"upload-failure.spr")); fail_upload = 0;
    assert(!memcmp(base,&saved,sizeof saved) && outstanding == 5);
    check_indexed(saved.images[0].items,5,3,indexed_pixels); check_rgba(saved.images[1].items,3,2,rgba_pixels);
    resource_destroy(base); clean();
}
static void test_empty_frames(void)
{
    BYTE data[1200]; size_t size = fixture(data, 0x201); unsigned v;
    struct Res *base = sprite_resource_create(); struct SprRes *sprite = (struct SprRes *)base;
    set_half(0);
    put_word(data+8,0); set_input(data,size); assert(resource_load(base,"narrow.spr"));
    assert(sprite->images[0].items[0].width == 0 && sprite->images[0].items[0].height == 3);
    assert(sprite->images[0].items[0].pixels); check_rgba(sprite->images[1].items,3,2,rgba_pixels);
    size = fixture(data, 0x201); put_word(data+12,0); memmove(data+14,data+30,size-30); size -= 16;
    put_word(data+8,0); put_word(data+10,0); set_input(data,size); assert(resource_load(base,"empty-frame.spr"));
    assert(!sprite->images[0].items[0].width && !sprite->images[0].items[0].height);
    check_rgba(sprite->images[1].items,3,2,rgba_pixels); check_palette(sprite,data+size-1024);
    size = fixture(data, 0x201); put_word(data+30,0); put_word(data+32,0); set_input(data,size);
    assert(resource_load(base,"empty-rgba.spr"));
    assert(!sprite->images[1].items[0].width && !sprite->images[1].items[0].texture);
    for (v = 0x100; v <= 0x200; v += 0x100) {
        size = fixture(data, v); put_word(data+(v>=0x200?8:6),0); put_word(data+(v>=0x200?10:8),0);
        set_input(data,size); assert(resource_load(base,"raw-empty.spr"));
        assert(!sprite->images[0].items[0].width && sprite->images[0].items[0].pixels);
    }
    resource_destroy(base); clean();
}
static size_t uneven(BYTE *data, const BYTE *code, unsigned size)
{
    size_t n = 14; unsigned i;
    memcpy(data, "SP\x01\x02\x01\x00\x00\x00", 8); put_word(data + 8, 5); put_word(data + 10, 3);
    put_word(data + 12, size); memcpy(data + n, code, size); n += size;
    for (i = 0; i < 1024; ++i) data[n + i] = (BYTE)i;
    return n + 1024;
}
static void test_uneven_streams(void)
{
    static const BYTE short_code[] = {1,0,3,2}, dangling[] = {1,0,3,2,0};
    static const BYTE past_end[] = {1,0,3,2,3,4,0,1,5,6,7,8,9,0,9};
    static const BYTE long_code[] = {1,0,3,2,3,4,0,1,5,6,7,8,9,0,1,10,11,12};
    static const BYTE short_pixels[15] = {1,0,0,0,2}, cut_pixels[] = {1,0,0,0,2,3,4,0,5,6,7,8,9,0,0};
    const struct { const BYTE *code; unsigned size; const BYTE *pixels; } cases[] = {
        {short_code, sizeof short_code, short_pixels}, {dangling, sizeof dangling, short_pixels},
        {past_end, sizeof past_end, cut_pixels}, {long_code, sizeof long_code, indexed_pixels}};
    BYTE data[1100]; unsigned k, half;
    for (k = 0; k < sizeof cases / sizeof *cases; ++k) for (half = 0; half < 4; ++half) {
        struct Res *base = sprite_resource_create(); struct SprRes *sprite = (struct SprRes *)base;
        set_half(half); set_input(data, uneven(data, cases[k].code, cases[k].size));
        assert(resource_load(base, "uneven.spr") && sprite->images[0].count == 1 && !borrowed_buffers);
        check_indexed(sprite->images[0].items, 5, 3, cases[k].pixels);
        resource_destroy(base); clean();
    }
}
static void test_prefixes(void)
{
    BYTE data[1200]; SYSTEM_INFO sys; DWORD old; BYTE *pages, *guard; unsigned v;
    const unsigned versions[] = {0x100,0x101,0x200,0x201};
    GetSystemInfo(&sys); pages = VirtualAlloc(NULL,sys.dwPageSize*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    assert(pages); guard = pages + sys.dwPageSize; assert(VirtualProtect(guard,sys.dwPageSize,PAGE_NOACCESS,&old));
    set_half(0);
    for (v = 0; v < 4; ++v) {
        size_t size = fixture(data,versions[v]), n;
        struct Res *base = sprite_resource_create(); struct SprRes saved;
        set_input(data,size); assert(resource_load(base,"old.spr")); saved = *(struct SprRes *)base;
        for (n = 0; n < size; ++n) {
            memcpy(guard-n,data,n); set_input(guard-n,n);
            if (versions[v] >= 0x101 && n >= size - 1024) {
                assert(resource_load(base,"short-palette.spr") && !borrowed_buffers);
                check_palette((struct SprRes *)base, n >= 1024u + (versions[v] >= 0x200 ? 8u : 6u) ? guard - 1024 : NULL);
                check_indexed(sprite_resource_image((struct SprRes *)base,0,0),5,3,indexed_pixels);
                continue;
            }
            assert(!resource_load(base,"prefix.spr") && !memcmp(base,&saved,sizeof saved));
            assert(!borrowed_buffers); ++guarded_prefixes;
        }
        resource_destroy(base); clean();
    }
    assert(VirtualFree(pages,0,MEM_RELEASE));
}
static void test_allocation_failures(void)
{
    BYTE data[1200]; size_t size = fixture(data,0x201); unsigned half, point, count, before;
    fail_at = allocation_calls + 1; assert(!sprite_resource_create()); fail_at = 0; clean();
    for (half = 0; half < 4; ++half) {
        struct Res *base = sprite_resource_create(); struct SprRes saved;
        set_half(half); set_input(data,size); before = allocation_calls;
        assert(resource_load(base,"count.spr")); count = allocation_calls - before; resource_destroy(base); clean();
        for (point = 1; point <= count; ++point) {
            base = sprite_resource_create(); set_half(0); assert(resource_load(base,"old.spr"));
            saved = *(struct SprRes *)base; set_half(half); fail_at = allocation_calls + point;
            assert(!resource_load(base,"replacement.spr")); fail_at = 0;
            assert(!memcmp(base,&saved,sizeof saved) && outstanding == 5 && textures == 1 && !borrowed_buffers);
            resource_destroy(base); clean();
        }
    }
}
static void test_empty_and_large(void)
{
    BYTE tiny_data[16] = {'S','P',1,2,0,0,0,0};
    BYTE *large = malloc(90010), *raw = malloc(90000); unsigned i, half;
    struct Res *base = sprite_resource_create(); struct SprRes *sprite = (struct SprRes *)base;
    assert(large && raw); set_half(0); set_input(tiny_data,8); assert(resource_load(base,"empty.spr"));
    assert(!sprite->images[0].count && !sprite->images[1].count); check_palette(sprite,NULL);
    memcpy(large,"SP\x00\x01\x01\x00",6); put_word(large+6,300); put_word(large+8,300);
    for (i = 0; i < 90000; ++i) large[10+i] = raw[i] = (BYTE)(i % 2 ? 127 : 0);
    for (half = 0; half < 4; ++half) {
        set_half(half); set_input(large,90010); assert(resource_load(base,"large.spr"));
        check_indexed(sprite->images[0].items,300,300,raw);
    }
    memcpy(large,"SP\x01\x02\x01\x00\x00\x00",8);
    put_word(large+8,1); put_word(large+10,1); put_word(large+12,2);
    large[14] = large[15] = 0; memset(large+16,0,1024); raw[0] = 0;
    for (half = 0; half < 4; ++half) {
        set_half(half); set_input(large,1040); assert(resource_load(base,"one.spr"));
        check_indexed(sprite->images[0].items,1,1,raw);
        assert((sprite_resource_image(sprite,0,0) != NULL) == (half == 0));
    }
    memcpy(tiny_data,"SP\x00\x02\x00\x00\x01\x00\x01\x00\x01\x00",12);
    memcpy(tiny_data+12,rgba_file+20,4);
    for (half = 0; half < 4; ++half) {
        set_half(half); set_input(tiny_data,sizeof tiny_data); assert(resource_load(base,"rgba-only.spr"));
        check_rgba(sprite->images[1].items,1,1,rgba_pixels+2);
        assert((sprite_resource_image(sprite,1,0) != NULL) == (half == 0));
    }
    free(large); free(raw); resource_destroy(base); clean();
}

static int corpus(const char *path)
{
    FILE *file = fopen(path,"rb"); uint32_t size; unsigned files = 0, frames = 0;
    assert(file);
    while (fread(&size,sizeof size,1,file) == 1) {
        BYTE *data = malloc(size); long reference; unsigned half, counts[2], kind;
        struct Res *base = sprite_resource_create(); struct SprRes *sprite = (struct SprRes *)base;
        assert(data && size >= 8 && size <= INT_MAX && fread(data,size,1,file) == 1);
        counts[0] = word(data+4); counts[1] = word(data+6); reference = ftell(file); assert(reference >= 0);
        for (half = 0; half < 4; ++half) {
            set_half(half); set_input(data,size); assert(resource_load(base,"asset.spr"));
            check_palette(sprite,data+size-1024); assert(!fseek(file,reference,SEEK_SET));
            for (kind = 0; kind < 2; ++kind) {
                unsigned i; assert(sprite->images[kind].count == counts[kind]);
                for (i = 0; i < counts[kind]; ++i) {
                    uint32_t dimensions[2]; size_t bytes; BYTE *pixels;
                    assert(fread(dimensions,sizeof dimensions,1,file) == 1);
                    bytes = (size_t)dimensions[0]*dimensions[1]*(kind ? 4 : 1); pixels = malloc(bytes); assert(pixels);
                    assert(fread(pixels,bytes,1,file) == 1);
                    if (kind) check_rgba(sprite->images[kind].items+i,dimensions[0],dimensions[1],(DWORD *)pixels);
                    else check_indexed(sprite->images[kind].items+i,dimensions[0],dimensions[1],pixels);
                    free(pixels); if (!half) ++frames;
                }
            }
        }
        free(data); resource_destroy(base); clean(); ++files;
    }
    assert(feof(file) && !ferror(file)); fclose(file);
    printf("SPR corpus: %u files, %u images, four half-size settings match independent pixels.\n",files,frames);
    return 0;
}
static void test_atlas_resource_lifetime(void)
{
    BYTE data[1200], colors[1024]; size_t size = fixture(data, 0x201);
    struct Renderer renderer = {0}; struct Res *base = sprite_resource_create(), *pal_base;
    struct SprRes *sprite = (struct SprRes *)base; struct PaletteRes *pal;
    struct SpriteRegion old_region, new_region; struct Texture *texture;
    DWORD old_pixel; unsigned before;
    g_renderer = &renderer; set_half(0); set_input(data, size); assert(resource_load(base, "atlas.spr"));
    texture = sprite_atlas_resolve(&renderer, sprite->images[0].items, sprite->palette, &old_region);
    assert(texture); old_pixel = atlas_pixel(texture, &old_region); before = atlas_uploads;
    set_input(data, 0); assert(!resource_load(base, "truncated.spr"));
    assert(sprite_atlas_resolve(&renderer, sprite->images[0].items, sprite->palette, &new_region) == texture);
    assert(before == atlas_uploads && !memcmp(&old_region, &new_region, sizeof old_region));
    set_input(data, size); assert(resource_load(base, "reloaded.spr"));
    assert(!renderer.sprite_atlas->pages[0]->cells[0].image);
    assert(atlas_pixel(texture, &old_region) == old_pixel);
    assert(sprite_atlas_resolve(&renderer, sprite->images[0].items, sprite->palette, &new_region) == texture);
    assert(atlas_uploads == before + 1 && new_region.x != old_region.x);
    memcpy(colors, data + size - sizeof colors, sizeof colors);
    pal_base = palette_resource_create(); pal = (struct PaletteRes *)pal_base;
    set_input(colors, sizeof colors); assert(resource_load(pal_base, "palette.pal"));
    assert(sprite_atlas_resolve(&renderer, sprite->images[0].items, pal->palette, &old_region) == texture);
    old_pixel = atlas_pixel(texture, &old_region); before = atlas_uploads;
    set_input(colors, 10); assert(resource_load(pal_base, "short.pal"));
    assert(sprite_atlas_resolve(&renderer, sprite->images[0].items, pal->palette, &new_region) == texture);
    assert(atlas_uploads == before);
    memset(colors, 0x39, sizeof colors); set_input(colors, sizeof colors);
    assert(resource_load(pal_base, "replacement.pal"));
    assert(sprite_atlas_resolve(&renderer, sprite->images[0].items, pal->palette, &new_region) == texture);
    assert(atlas_uploads == before + 1 && new_region.x != old_region.x);
    assert(atlas_pixel(texture, &old_region) == old_pixel);
    resource_destroy(pal_base); resource_destroy(base);
    assert(textures == 1 && atlas_pixel(texture, &old_region) == old_pixel);
    sprite_atlas_destroy(&renderer); g_renderer = NULL; clean();
}
int main(int argc, char **argv)
{
    g_3dDevice.colorKeyPixel = 0x15ab;
    if (argc == 2) return corpus(argv[1]);
    test_versions(); test_rejected_replacement(); test_empty_frames(); test_uneven_streams(); test_prefixes();
    test_allocation_failures(); test_empty_and_large(); clean();
    test_atlas_resource_lifetime();
    printf("SPR owner: versions, pixels, %u guarded prefixes, replacement and allocation failures pass (%u uploads).\n",guarded_prefixes,uploads);
    return 0;
}
