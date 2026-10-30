#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
static unsigned now;
static DWORD test_time(void) { return now; }
static int fail_path, path_allocations;
static void *path_malloc(size_t size) {
    void *p;
    if (fail_path) return NULL;
    p = malloc(size); assert(p); ++path_allocations; return p;
}
static void path_free(void *p) { if (p) { --path_allocations; free(p); } }
#define malloc path_malloc
#define free path_free
#define timeGetTime test_time
#include "../src/contents/map_name_display.c"
#undef malloc
#undef free
#undef timeGetTime

struct TexCache g_textureCache;
struct C3dDevice g_3dDevice;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
int g_serviceType;
static struct Texture ornament;
static struct DcBitmap canvas;
static int missing_texture, missing_canvas, canvas_calls, clears, releases, texts, draws;

struct Texture *texture_cache_get(struct TexCache *cache,const char *name,int keyed,int full)
{ assert(cache==&g_textureCache && !strcmp(name, "유저인터페이스\\display_mapname\\test.png") && !keyed && !full); return missing_texture ? NULL : &ornament; }
struct DcBitmap *dc_bitmap_create(int w,int h)
{
    assert(w==800 && h==120); ++canvas_calls;
    return missing_canvas ? NULL : &canvas;
}
void dc_bitmap_fill(struct DcBitmap *b,const RECT *r,DWORD color)
{ assert(b==&canvas && !r && !color); ++clears; }
void dc_bitmap_destroy(struct DcBitmap *b)
{ assert(b==&canvas); ++releases; }
void dc_bitmap_draw(struct DcBitmap *b,int x,int y,int w,int h,DWORD color)
{ assert(b==&canvas && x==0 && y==72 && w==800 && h==120 && color==((now <= 5000 ? 255u : 5255u-now)<<24 | 0xffffff)); ++draws; }
void texture_draw(struct Texture *t,int x,int y,int w,int h,DWORD color)
{ assert(t==&ornament && x==280 && y==72 && w==240 && h==120 && color==((now <= 5000 ? 255u : 5255u-now)<<24 | 0xffffff)); }
void dc_bitmap_alpha_from_brightness(struct DcBitmap *b,BYTE level)
{ assert(b==&canvas && level==255); }
struct TextDC *text_dc_construct(struct TextDC *dc,struct DcBitmap *target)
{ assert(target==&canvas); memset(dc,0,sizeof *dc); return dc; }
void text_dc_destruct(struct TextDC *dc) { (void)dc; }
void text_dc_select_font(struct TextDC *dc,int a,int b,int c,int d)
{ (void)dc;(void)a;(void)b;(void)c;(void)d; }
void text_dc_text_extent(struct TextDC *dc,const char *s,int n,SIZE *out)
{ (void)dc;assert(s && n==4);out->cx=40;out->cy=20; }
COLORREF text_dc_set_text_color(struct TextDC *dc,COLORREF color)
{ (void)dc;assert(color==0xffffff);return 0; }
void text_dc_text_out(struct TextDC *dc,int x,int y,const char *s,int n)
{ (void)dc;(void)y;assert(x==380 && s && n==4); ++texts; }
static void construct(struct MapNameDisplay *banner)
{
    assert(map_name_display_construct(banner,"test","test","test"));
    assert(!path_allocations);
}
int main(void)
{
    struct MapNameDisplay banner;
    renderer.width=800; renderer.height=600; ornament.reqWidth=200; ornament.reqHeight=100;
    fail_path=1;
    assert(!map_name_display_construct(&banner,"test","test","test"));
    assert(!banner.band && !banner.ornament && !path_allocations);
    map_name_display_destruct(&banner); fail_path=0;
    missing_texture=1; memset(&banner,0xcc,sizeof banner); construct(&banner);
    assert(!banner.band && !canvas_calls && !map_name_display_render(&banner));
    map_name_display_destruct(&banner); assert(!releases);
    missing_texture=0; missing_canvas=1; construct(&banner);
    assert(!banner.band && canvas_calls==1 && !texts && !clears && !map_name_display_render(&banner));
    map_name_display_destruct(&banner); assert(!releases);
    missing_canvas=0; construct(&banner);
    assert(banner.band==&canvas && canvas.singleTexture && clears==1 && texts==2);
    assert(map_name_display_render(&banner) && draws==1);
    now=5000; assert(map_name_display_render(&banner));
    now=5001; assert(map_name_display_render(&banner));
    now=5254; assert(map_name_display_render(&banner));
    now=5255; assert(!map_name_display_render(&banner));
    map_name_display_destruct(&banner); map_name_display_destruct(&banner); assert(releases==1 && !path_allocations);
    puts("Map banner: canonical screen dimensions and canvas calls, missing ornament/canvas, drawing and cleanup passed.");
    return 0;
}
