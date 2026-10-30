#include "render/texture_types.h"
#include "ui/cooldown.h"
#include "render/rp_pool.h"
#include "render/texture_globals.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void *checked_realloc(void *old, size_t size)
{
    void *p = checked_malloc(size);
    if (!p) return NULL;
    if (old) {
        size_t old_size = ((struct Allocation *)old - 1)->size;
        memcpy(p, old, old_size < size ? old_size : size);
        checked_free(old);
    }
    return p;
}

#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#include "../src/resources/state_icon_sheet.c"
#undef free
#undef realloc
#undef calloc
#undef malloc

static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
struct Texture g_whiteTexture;
static struct RPQuad quads[512];
static struct RPQuad *submitted[512];
static struct TexDrawVertex buffer[512 * PIE_VERTICES];
static unsigned quad_count, vertex_count, submitted_count, ticks = 100;

unsigned cooldown_clock_now(void) { return ticks; }
struct RPQuad *renderer_new_quad(struct Renderer *r)
{
    assert(r == &renderer && quad_count < 512);
    return &quads[quad_count++];
}
struct TexDrawVertex *renderer_new_tex_draw_vertices(struct Renderer *r, int count)
{
    assert(r == &renderer && count == PIE_VERTICES);
    assert(vertex_count + count <= sizeof buffer / sizeof *buffer);
    vertex_count += count;
    return buffer + vertex_count - count;
}
void renderer_add_prim(struct Renderer *r, struct RPQuad *p, int flags)
{
    assert(r == &renderer && flags == SCREEN_LIST && submitted_count < 512);
    submitted[submitted_count++] = p;
}

int main(void)
{
    struct CooldownTimer bar;
    struct PieVertex expected[PIE_VERTICES];
    struct StateIconSheet *sheet;
    unsigned i, before, live;
    fail_at = allocation_calls + 1;
    assert(!icon_sheet_create() && !g_stateIconSheet); fail_at = 0;
    assert(icon_sheet_create()); sheet = g_stateIconSheet;
    assert(!icon_sheet_create());
    icon_sheet_set_timer(sheet, 7, 8000, 100, 0);
    assert(sheet->timers && sheet->timers->state == 7);
    fail_at = allocation_calls + 1;
    icon_sheet_set_timer(sheet, 9, 8000, 100, 0); fail_at = 0;
    assert(!*timer_at(sheet, 9));
    before = allocation_calls;
    icon_sheet_set_timer(sheet, 7, 9000, 100, 1);
    assert(allocation_calls == before && sheet->timers->timer.total_ms == 9000);
    fail_at = allocation_calls + 1;
    icon_sheet_queue_state(sheet, 7, 20, 30); fail_at = 0;
    assert(!sheet->icons.size && !quad_count);
    icon_sheet_queue_state(sheet, 7, 20, 30);
    assert(sheet->icons.size == 1 && quad_count == 1);
    assert((const void *)quads[0].verts != sheet->timers->timer.verts);
    memcpy(expected, quads[0].verts, sizeof expected);
    icon_sheet_remove_timer(sheet, 7);
    assert(!sheet->timers && !memcmp(quads[0].verts, expected, sizeof expected));
    cooldown_timer_init(&bar, 2, 12, 4, 8000, 100, 0, 0);
    for (i = 0; i < 40; ++i) {
        ticks = 101 + i;
        icon_sheet_queue(sheet, 0, 0, 12, 12, &bar, 6, 0, 0);
        assert(!memcmp(quads[0].verts, expected, sizeof expected));
    }
    assert(sheet->bars.size == 40);
    while (sheet->bars.size < sheet->bars.capacity)
        icon_sheet_queue(sheet, 0, 0, 12, 12, &bar, 6, 0, 0);
    before = (unsigned)sheet->bars.size; live = outstanding;
    fail_at = allocation_calls + 1;
    icon_sheet_queue(sheet, 0, 0, 12, 12, &bar, 6, 0, 0); fail_at = 0;
    assert(sheet->bars.size == before && outstanding == live);
    icon_sheet_flush_bars(sheet);
    assert(!sheet->bars.size && submitted_count == before);
    icon_sheet_flush(sheet);
    assert(submitted_count == before + 1 && submitted[before] == &quads[0]);
    assert(!memcmp(submitted[before]->verts, expected, sizeof expected));
    icon_sheet_set_timer(sheet, 10, 8000, 100, 0);
    icon_sheet_queue_state(sheet, 10, 20, 30);
    icon_sheet_clear(sheet);
    icon_sheet_flush(sheet);
    assert(submitted_count == before + 1);
    icon_sheet_destroy(); icon_sheet_destroy();
    assert(!g_stateIconSheet && !outstanding);
    assert(!memcmp(submitted[before]->verts, expected, sizeof expected));
    puts("Cooldown sheet: C-owned timers/queues, failed allocations, replacement/removal, stable queued copies through renderer growth, queue order and cleanup passed");
    return 0;
}
