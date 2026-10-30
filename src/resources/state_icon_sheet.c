#include "render/screen_prim.h"
#include "render/renderer.h"
#include "ui/cooldown.h"
#include "resources/state_icon_sheet.h"
#include "render/rp_pool.h"
#include "render/texture_globals.h"
#include <stdlib.h>
#include <string.h>

struct IconTimer {
    struct IconTimer *next;
    unsigned state;
    struct CooldownTimer timer;
};

struct PieQueue {
    struct RPQuad **items;
    size_t size, capacity;
};

struct StateIconSheet {
    struct IconTimer *timers;
    struct PieQueue bars, icons;
};

struct StateIconSheet *g_stateIconSheet;

unsigned char icon_sheet_create(void)
{
    if (g_stateIconSheet) return 0;
    g_stateIconSheet = calloc(1, sizeof *g_stateIconSheet);
    return g_stateIconSheet != NULL;
}

void icon_sheet_destroy(void)
{
    struct StateIconSheet *sheet = g_stateIconSheet;
    if (!sheet) return;
    while (sheet->timers) {
        struct IconTimer *next = sheet->timers->next;
        free(sheet->timers);
        sheet->timers = next;
    }
    free(sheet->icons.items);
    free(sheet->bars.items);
    free(sheet);
    g_stateIconSheet = NULL;
}

static struct IconTimer **timer_at(struct StateIconSheet *sheet, unsigned state)
{
    struct IconTimer **link = &sheet->timers;
    while (*link && (*link)->state != state) link = &(*link)->next;
    return link;
}

void icon_sheet_set_timer(struct StateIconSheet *sheet, unsigned state, unsigned total_ms,
    unsigned start, unsigned elapsed)
{
    struct IconTimer **link, *node;
    if (!sheet) return;
    link = timer_at(sheet, state);
    node = *link;
    if (!node) {
        node = malloc(sizeof *node);
        if (!node) return;
        node->next = NULL;
        node->state = state;
        *link = node;
    }
    cooldown_timer_init(&node->timer, 0, 0, 0, total_ms, start, elapsed, 1);
}

void icon_sheet_remove_timer(struct StateIconSheet *sheet, unsigned state)
{
    struct IconTimer **link, *node;
    if (!sheet) return;
    link = timer_at(sheet, state);
    node = *link;
    if (node) {
        *link = node->next;
        free(node);
    }
}

static int queue_reserve(struct PieQueue *queue)
{
    size_t capacity, limit = (size_t)-1 / sizeof *queue->items;
    struct RPQuad **items;
    if (queue->size < queue->capacity) return 1;
    if (queue->size == limit) return 0;
    capacity = queue->capacity ? queue->capacity + queue->capacity / 2 + 1 : 16;
    if (capacity > limit) capacity = limit;
    items = realloc(queue->items, capacity * sizeof *items);
    if (!items) return 0;
    queue->items = items;
    queue->capacity = capacity;
    return 1;
}

void icon_sheet_queue(struct StateIconSheet *sheet, int x, int y, int w, int h,
    struct CooldownTimer *timer, int limit, int dx, int dy)
{
    struct PieQueue *queue;
    struct RPQuad *quad;
    struct TexDrawVertex *vertices;
    if (!sheet || !timer) return;
    if (!cooldown_timer_layout(timer, cooldown_clock_now(), x, y, w, h, limit, dx, dy)) return;
    queue = timer->mode ? &sheet->icons : &sheet->bars;
    if (!queue_reserve(queue)) return;
    quad = renderer_new_quad(g_renderer);
    if (!quad) return;
    vertices = renderer_new_tex_draw_vertices(g_renderer, PIE_VERTICES);
    if (!vertices) return;
    SIZE_ASSERT(PieVertex, sizeof(struct TexDrawVertex));
    memcpy(vertices, timer->verts, sizeof timer->verts);
    quad->verts = vertices;
    quad->texture = &g_whiteTexture;
    quad->nverts = timer->vertex_count;
    queue->items[queue->size++] = quad;
}

void icon_sheet_queue_state(struct StateIconSheet *sheet, unsigned state, int x, int y)
{
    struct IconTimer *node;
    if (!sheet) return;
    node = *timer_at(sheet, state);
    if (!node) return;
    node->timer.x = x;
    node->timer.y = y;
    icon_sheet_queue(sheet, 0, 0, 16, 16, &node->timer, 0, 0, 0);
}

void icon_sheet_clear(struct StateIconSheet *sheet)
{
    if (sheet) sheet->icons.size = 0;
}

static void queue_flush(struct PieQueue *queue)
{
    size_t i;
    for (i = 0; i < queue->size; ++i)
        renderer_add_prim(g_renderer, queue->items[i], SCREEN_LIST);
    queue->size = 0;
}

void icon_sheet_flush(struct StateIconSheet *sheet)
{
    if (sheet) queue_flush(&sheet->icons);
}

void icon_sheet_flush_bars(struct StateIconSheet *sheet)
{
    if (sheet) queue_flush(&sheet->bars);
}
