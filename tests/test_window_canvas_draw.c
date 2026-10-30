#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render/cdc.h"
#include "../src/ui/wndmgr/render_windows.c"

DWORD g_renderCheckpointLine;
const char *g_renderCheckpointFile;
struct StateIconSheet *g_stateIconSheet;
struct Roulette *g_roulette;
static struct UIWindow window;
static struct DcBitmap bitmap;
static struct Texture texture;
static unsigned renders, colors, bitmaps, textures, flushes, icons, extras;
static unsigned box_queries, boxes, backdrops;
static BYTE box_result;

static BYTE render(struct UIWindow *w, int force)
{ assert(w == &window && !force); ++renders; return 1; }
static BYTE get_box(struct UIWindow *w, struct UIBackdropBox *box)
{
    assert(w == &window && box && renders == 1 && !colors);
    ++box_queries;
    box->x = -7; box->y = 9; box->width = 121; box->height = 52;
    box->argb = 0x6688aabb;
    return box_result;
}
static void draw_backdrop(struct UIWindow *w)
{ assert(w == &window && box_queries == 1 && box_result != 1 && !colors && !boxes); ++backdrops; }
DWORD ui_window_get_color(struct UIWindow *w)
{ assert(w == &window && renders == 1); ++colors; return 0x83abcdef; }
static void extra(struct UIWindow *w)
{ assert(w == &window && flushes == 1 && icons == 1); ++extras; }
static struct UIWindowOps window_ops = {
    .render = render, .get_backdrop_box = get_box,
    .draw_backdrop = draw_backdrop,
    .render_overlay = extra,
};
static void dimensions(int x, int y, int w, int h, DWORD tint)
{ assert(x == 17 && y == 23 && w == 310 && h == 15 && tint == 0x83abcdef && colors == 1); }
void dc_bitmap_draw(struct DcBitmap *b, int x, int y, int w, int h, DWORD tint)
{ assert(b == &bitmap); dimensions(x,y,w,h,tint); ++bitmaps; }
void texture_draw_surface(struct Texture *t, int x, int y, int w, int h, DWORD tint)
{ assert(t == &texture); dimensions(x,y,w,h,tint); ++textures; }
struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *mgr, int id)
{ (void)mgr; assert(id == WND_ID_BASIC_BUTTONS); return NULL; }
void ui_window_flush_quads(struct UIWindow *w)
{ assert(w == &window && renders == 1); ++flushes; }
void icon_sheet_flush_bars(struct StateIconSheet *sheet)
{ assert(sheet == g_stateIconSheet && flushes == 1); ++icons; }
void ui_soft_next(int on) { (void)on; assert(0); }
void draw_box_screen(int x,int y,int w,int h,DWORD tint)
{
    assert(x == -7 && y == 9 && w == 121 && h == 52 && tint == 0x6688aabb);
    assert(box_queries == 1 && !backdrops && !colors);
    ++boxes;
}
void ui_ro_map_wnd_render_overlay(struct UIRoMapWnd *w) { (void)w;assert(0); }
void ui_buttons_of_basic_wnd_render_buttons(struct UIButtonsOfBasicWnd *w) { (void)w;assert(0); }
void roulette_render(struct Roulette *r) { (void)r;assert(0); }

static void run_cases(void)
{
    struct UIWindowMgr mgr = {0};
    struct ListNode head = {0};
    struct { struct ListNode links; struct UIWindow *window; } entry = {0};
    int mode, backdrop;
    static const BYTE box_results[] = {0, 1, 0, 2, 2};
    static const BYTE custom_results[] = {0, 1, 1, 1, 0};
    head.next = head.prev = &entry.links; entry.links.next = entry.links.prev = &head; entry.window = &window;
    mgr.windows.head = &head; mgr.windows.size = 1;
    window.ops = &window_ops; window.x = 17; window.y = 23; window.width = 310; window.height = 15;
    bitmap.width = 310; bitmap.height = 15;
    for (backdrop = 0; backdrop < 5; ++backdrop) {
      box_result = box_results[backdrop]; window_ops.has_custom_backdrop = custom_results[backdrop];
      for (mode = 0; mode < 5; ++mode) {
        renders=colors=bitmaps=textures=flushes=icons=extras=0;
        box_queries=boxes=backdrops=0;
        window.visible = mode != 4; window.composites = mode == 1 || mode == 2;
        window.texture = mode == 1 ? &texture : NULL;
        window.canvas = mode == 3 ? NULL : &bitmap;
        ui_window_mgr_render_windows(&mgr);
        assert(bitmaps == (unsigned)(mode == 0) && textures == (unsigned)(mode == 1));
        assert(colors == (unsigned)(mode < 2));
        assert(renders == (unsigned)(mode != 4) && flushes == renders && icons == renders && extras == renders);
        assert(box_queries == renders);
        assert(boxes == (box_result == 1 ? renders : 0));
        assert(backdrops == (box_result != 1 && window_ops.has_custom_backdrop ? renders : 0));
      }
    }
}
int main(void)
{
    __try { run_cases(); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        fputs("Window drawing accessed an invalid legacy canvas dispatch pointer.\n",stderr);
        return 1;
    }
    puts("Window canvas drawing: actual frame loop, bitmap/texture routes, backdrop priority and exact flags, missing targets, visibility, geometry, tint and ordering passed.");
    return 0;
}
