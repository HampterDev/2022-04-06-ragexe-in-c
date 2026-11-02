#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <mmsystem.h>
#include <zlib.h>
#include "owned_storage_test_allocator.h"
void *bitmap_test_malloc(size_t size) { return checked_malloc(size); }
void bitmap_test_free(void *p) { checked_free(p); }
void *bitmap_test_calloc(size_t count, size_t size)
{
    void *p;
    if (size && count > SIZE_MAX / size) return NULL;
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
void *bitmap_test_realloc(void *p, size_t size)
{
    void *next = checked_malloc(size);
    if (next && p) {
        size_t old = ((struct Allocation *)p - 1)->size;
        memcpy(next, p, old < size ? old : size);
        checked_free(p);
    }
    return next;
}
static DWORD clock_now;
static DWORD bitmap_test_now(void) { return clock_now; }
#define timeGetTime bitmap_test_now
#include "bitmap_alloc_fixture.h"
#include "../src/resources/bitmap_storage.c"
#include "../src/resources/bitmap_codecs.c"
#include "../src/resources/bitmap_gif.c"
#include "../src/resources/bitmap_png.c"
#include "../src/resources/bitmap.c"
#include "../src/resources/emblem_res.c"
#undef timeGetTime
#undef malloc
#undef calloc
#undef realloc
#undef free

static BYTE *file_bytes;
static int file_size, file_is_open, bad_emblem, bad_marks;
static struct GuildMgr guild;
struct GuildMgr *g_guildMgr = &guild;
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { (void)res; }
struct File *file_construct(struct File *file) { memset(file, 0, sizeof *file); return file; }
BYTE file_open(struct File *file, const char *name, int mode, BYTE resolve, BYTE disk)
{
    (void)name; assert(!mode && resolve && !disk && !file_is_open);
    if (!file_bytes) return 0;
    file->buffer = file_bytes; file->size = file_size; file_is_open = 1; return 1;
}
void file_close(struct File *file) { assert(file_is_open); file_is_open = 0; file->buffer = NULL; }
void file_destruct(struct File *file) { (void)file; assert(!file_is_open); }
BYTE bitmap_res_load_jpg_data(struct BitmapRes *bitmap, const BYTE *data, int size)
{ (void)bitmap; (void)data; (void)size; abort(); }
void *mem_alloc(size_t size)
{
    unsigned failing = fail_at;
    void *p;
    fail_at = 0;
    p = checked_malloc(size);
    fail_at = failing;
    --allocation_calls;
    return p;
}
void mem_free(void *memory)
{  bitmap_test_free(memory); }
void fatal_error(const char *message) { (void)message; abort(); }
char guild_mgr_is_bad_emblem(struct GuildMgr *self, struct String path)
{ assert(self == &guild); string_free(&path); return (char)bad_emblem; }
void guild_mgr_mark_bad_emblem(struct GuildMgr *self, struct String path)
{ assert(self == &guild); string_free(&path); ++bad_marks; }

static BYTE blank_bitmap(struct BitmapRes *self, int width, int height)
{
    DWORD *pixels = bitmap_allocate_pixels(width, height);
    if (!pixels) return 0;
    memset(pixels, 0xff, bitmap_pixel_count(width, height) * sizeof *pixels);
    bitmap_replace_pixels(self, pixels, width, height, 0);
    return 1;
}

struct BitmapFixture { const char *name; int kind; const BYTE *data; size_t size; int width, height, frames; const DWORD *pixels; };
#include "bitmap_format_fixtures.h"
static BYTE decode(struct GifRes *image, const struct BitmapFixture *f, const BYTE *data, int size)
{
    if (f->kind == 2) return gif_res_load_gif_data(image, data, size, 0);
    if (f->kind == 1) return bitmap_res_load_tga_data(&image->bmp, data, size);
    return bitmap_res_load_bmp_data(&image->bmp, data, size, 0);
}
static void match(const struct GifRes *image, const struct BitmapFixture *f)
{
    int i; size_t pixels = (size_t)f->width * f->height;
    assert(image->bmp.width == f->width && image->bmp.height == f->height);
    assert(f->kind != 2 || image->frames.count == (size_t)f->frames);
    for (i = 0; i < f->frames; ++i) {
        const DWORD *p = f->kind == 2 ? image->frames.data[i].pixels : image->bmp.pixels;
        size_t j;
        for (j=0;j<pixels;++j) if (p[j] != f->pixels[i*pixels+j]) {
            printf("Mismatch %s frame %d pixel %u: %08lx expected %08lx\n",f->name,i,(unsigned)j,p[j],f->pixels[i*pixels+j]);
            fflush(stdout); assert(0);
        }
    }
}
static void check_formats(void)
{
    size_t index, prefix, boundaries = 0;
    SYSTEM_INFO system; DWORD previous_protection;
    BYTE *pages;
    GetSystemInfo(&system);
    pages = VirtualAlloc(NULL, system.dwPageSize * 2, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages+system.dwPageSize,system.dwPageSize,PAGE_NOACCESS,&previous_protection));
    for (index=0;index<sizeof bitmap_fixtures/sizeof bitmap_fixtures[0];++index) {
        const struct BitmapFixture *f = &bitmap_fixtures[index];
        struct GifRes image, before;
        unsigned attempts, failure;
        gif_resource_init(&image);
        allocation_calls = 0;
        assert(decode(&image,f,f->data,(int)f->size));
        attempts = allocation_calls;
        match(&image,f);
        gif_resource_finish(&image); assert(!outstanding);
        gif_resource_init(&image);
        assert(blank_bitmap(&image.bmp,1,1));
        assert(bitmap_res_make_gray_copy(&image.bmp,0));
        before=image;
        assert(f->size<=system.dwPageSize);
        for (prefix=0;prefix<f->size;++prefix) {
            BYTE *at=pages+system.dwPageSize-prefix;
            memcpy(at,f->data,prefix);
            assert(!decode(&image,f,at,(int)prefix));
            assert(!memcmp(&image,&before,sizeof image) && outstanding==2);
            ++boundaries;
        }
        for (failure=1;failure<=attempts;++failure) {
            allocation_calls=0; fail_at=failure;
            assert(!decode(&image,f,f->data,(int)f->size));
            assert(!memcmp(&image,&before,sizeof image) && outstanding==2);
        }
        fail_at=0;
        assert(decode(&image,f,f->data,(int)f->size)); match(&image,f);
        assert(!image.bmp.grayPixels);
        gif_resource_clear(&image); gif_resource_finish(&image); assert(!outstanding);
    }
    VirtualFree(pages,0,MEM_RELEASE);
    printf("PASS: 23 real image fixtures, %u guarded prefixes and every owned/giflib allocation failure\n",(unsigned)boundaries);
}

static void check_malformed(void)
{
    struct BitmapRes image, before;
    struct GifRes gif, gif_before;
    BYTE copy[2048]; size_t position;
    unsigned i;
    bitmap_resource_init(&image); assert(blank_bitmap(&image,1,1));before=image;
    for(i=0;i<6;++i) {
        memcpy(copy,bmp8_short_data,sizeof bmp8_short_data);
        if(i==0)copy[26]=2;
        if(i==1)copy[30]=1;
        if(i==2)copy[28]=16;
        if(i==3)memset(copy+18,255,4);
        if(i==4){memset(copy+22,0,4);copy[25]=128;}
        if(i==5)memset(copy+10,255,4);
        assert(!bitmap_res_load_bmp_data(&image,copy,sizeof bmp8_short_data,0));
        assert(!memcmp(&before,&image,sizeof image));
    }
    {
        struct BitmapRes indexed;
        bitmap_resource_init(&indexed);
        memcpy(copy,bmp8_short_data,sizeof bmp8_short_data);copy[70]=4;
        assert(bitmap_res_load_bmp_data(&indexed,copy,sizeof bmp8_short_data,0));
        assert(indexed.pixels[3]==0xff000000u && indexed.pixels[4]==bmp8_short_pixels[4]);
        bitmap_resource_finish(&indexed);
    }
    memcpy(copy,tga_rle_data,sizeof tga_rle_data);copy[18]=0xff;
    assert(!bitmap_res_load_tga_data(&image,copy,sizeof tga_rle_data));
    assert(!memcmp(&before,&image,sizeof image));
    assert(!bitmap_res_load_bmp_data(&image,bmp24_1_data,sizeof bmp24_1_data,1));
    bitmap_resource_finish(&image);assert(!outstanding);
    gif_resource_init(&gif);assert(gif_res_add_blank_frame(&gif,1,1));gif_before=gif;
    memcpy(copy,gif_simple_data,sizeof gif_simple_data);
    position=25;assert(copy[position]==',');copy[position+1]=255;copy[position+2]=255;
    assert(!gif_res_load_gif_data(&gif,copy,sizeof gif_simple_data,0));
    memcpy(copy,gif_simple_data,sizeof gif_simple_data);copy[position+10]=12;
    assert(!gif_res_load_gif_data(&gif,copy,sizeof gif_simple_data,0));
    memcpy(copy,gif_simple_data,sizeof gif_simple_data);copy[10]=0x80;
    memmove(copy+19,copy+25,sizeof gif_simple_data-25);
    assert(!gif_res_load_gif_data(&gif,copy,sizeof gif_simple_data-6,0));
    assert(!gif_res_load_gif_data(&gif,gif_simple_data,sizeof gif_simple_data,1));
    assert(!memcmp(&gif,&gif_before,sizeof gif));gif_resource_finish(&gif);assert(!outstanding);
    puts("PASS: malformed dimensions, spans, palettes, RLE packets, GIF rectangles/LZW and emblem restrictions");
}

static void check_lifecycle(void)
{
    struct GifRes gif, before; struct BitmapRes bitmap;
    struct Res *res;
    unsigned fail;
    gif_resource_init(&gif);
    assert(gif_res_load_from_buffer(&gif,"image.gif",gif_disposal_data,sizeof gif_disposal_data,0));
    assert(gif.loopCount==3 && gif.frames.data[0].delay==10 && gif.animated);
    gif.lastTime=0xfffffff0u;clock_now=0x54;
    assert(gif_res_animate(&gif) && gif.curFrame==1 && gif.changed);
    assert(!gif_res_animate(&gif) && !gif.changed);
    assert(bitmap_res_make_gray_copy(&gif.bmp,1));
    clock_now+=100; assert(gif_res_animate(&gif) && !gif.bmp.grayPixels);
    before=gif;
    assert(!gif_res_load_from_buffer(&gif,"x.bmp",bmp24_1_data,4,0));
    assert(!memcmp(&gif,&before,sizeof gif));
    assert(gif_res_load_from_buffer(&gif,"x.bmp",bmp24_1_data,sizeof bmp24_1_data,0));
    assert(!gif.frames.count && !gif.animated && !gif.changed && !gif.curFrame);
    for(fail=1;fail<=2;++fail) {
        before=gif;allocation_calls=0;fail_at=fail;
        assert(!gif_res_add_blank_frame(&gif,24,24));assert(!memcmp(&gif,&before,sizeof gif));
    }
    fail_at=0; assert(gif_res_add_blank_frame(&gif,24,24)); assert(gif.frames.count==1 && gif.bmp.pixels[575]==0xffffffff);
    gif_resource_finish(&gif);assert(!outstanding);
    bitmap_resource_init(&bitmap); assert(!bitmap.width && !bitmap.height);
    assert(blank_bitmap(&bitmap,2,1));
    bitmap.pixels[0]=0x12ff00ff;bitmap.pixels[1]=0x34000000;
    assert(bitmap_res_make_gray_copy(&bitmap,0));
    assert(bitmap.grayPixels[0]==0x12ff00ff && bitmap.grayPixels[1]==0x34787878);
    assert(bitmap_res_get_pixel(&bitmap,2,0)==0xff0000);
    bitmap_resource_finish(&bitmap);assert(!outstanding);
    res=bitmap_resource_create();assert(res);resource_destroy(res);assert(!outstanding);
    res=gif_resource_create();assert(res);assert(gif_res_add_blank_frame((struct GifRes*)res,24,24));resource_destroy(res);assert(!outstanding);
    res=emblem_resource_create();assert(res);assert(gif_res_add_blank_frame(&((struct EmblemRes*)res)->picture,24,24));resource_destroy(res);assert(!outstanding);
    puts("PASS: bitmap/GIF/emblem factories, destructor cleanup, gray copies, replacement and timer wrap");
}

static void check_emblems(void)
{
    struct EmblemRes emblem, before;
    BYTE raw[54+24*24*3]={0}, packed[4096]; unsigned long packed_size=sizeof packed;
    unsigned i;
    raw[0]='B';raw[1]='M';raw[10]=54;raw[14]=40;raw[18]=raw[22]=24;raw[26]=1;raw[28]=24;
    for(i=54;i<sizeof raw;i+=3)raw[i+2]=255;
    assert(compress(packed,&packed_size,raw,sizeof raw)==Z_OK);
    guild.emblem_max_size=GUILD_EMBLEM_MAX_SIZE;
    emblem_resource_init(&emblem);
    file_bytes=packed;file_size=(int)packed_size;
    assert(emblem_res_load(&emblem,"ok.ebm") && !file_is_open && !bad_marks);
    assert(emblem.picture.bmp.width==24 && emblem.picture.bmp.pixels[0]==0xffff0000);
    before=emblem;allocation_calls=0;fail_at=1;
    assert(!emblem_res_load(&emblem,"ok.ebm") && !memcmp(&before,&emblem,sizeof emblem));fail_at=0;
    file_size=2;assert(emblem_res_load(&emblem,"bad.ebm") && bad_marks==1);
    assert(emblem.picture.bmp.pixels[0]==0xffffffff);
    bad_emblem=1; file_bytes=NULL;
    assert(emblem_res_load(&emblem,"bad.ebm"));
    before=emblem;allocation_calls=0;fail_at=1;
    assert(!emblem_res_load(&emblem,"bad.ebm") && !memcmp(&before,&emblem,sizeof emblem));fail_at=0;
    bad_emblem=0;assert(!emblem_res_load(&emblem,"missing.ebm"));
    guild.emblem_max_size=UINT_MAX;assert(!emblem_res_load(&emblem,"large.ebm"));
    emblem_resource_finish(&emblem);assert(!outstanding);
    puts("PASS: real zlib emblem loads, fallback, bounded sizes, failure preservation and cleanup");
}
int main(void)
{
    check_formats();check_malformed();check_lifecycle();check_emblems();return 0;
}
