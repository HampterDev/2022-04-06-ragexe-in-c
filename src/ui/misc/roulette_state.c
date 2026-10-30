#include "render/rp_pool.h"
#include "render/texture_globals.h"
#include "iteminfo/item_api.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "ui/window.h"
#include "ui/ui_items.h"
#include "ui/shops/roulette.h"
#include "game/packets_roulette.h"
#include "ui/misc/roulette_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "std/tree.h"

#define SLOT_STEP 36.0f
#define MARKER_HALF 18.0f
#define SLOTS_PER_ROW 8
#define SPIN_SLOW_ROUNDS 6
#define WND_MSG_SPIN 0x3c

#define RENDER_LIST_UI 0x201

static int prize_less(const void *a, const void *b)
{
    const struct RoulettePrize *x = a, *y = b;

    return (short)x->row < (short)y->row && (short)x->position < (short)y->position;
}

static void destroy_prize(void *value)
{
    item_info_destruct(&((struct PrizeEntry *)value)->item);
}

struct Roulette *roulette_construct(struct Roulette *self)
{
    static const struct TexDrawVertex blank = { 0, 0, 0, 0, 0, 0xff000000, 0, 0 };
    int i;

    for (i = 0; i < 6; i++) {
        self->marker[i] = blank;
        self->start[i] = blank;
        self->end[i] = blank;
    }
    tree_init(&self->prizes);
    item_info_construct(&self->slot_item);
    item_info_construct(&self->addition_item);
    self->ok = 0;
    memset(self->marker, 0, sizeof self->marker);
    memset(self->start, 0, sizeof self->start);
    memset(self->end, 0, sizeof self->end);
    self->serial = 0;
    self->frames = 0;
    self->rounds = 0;
    self->spinning = 0;
    self->step = 0;
    self->spin_column = 0;
    self->step_time = 0;
    self->interval = 0;
    self->slot = 0;
    self->addition_item_id = 0;
    self->coins[0] = self->coins[1] = self->coins[2] = 0;
    self->has_slot = 0;
    self->slot_flag = 0;
    item_info_clear(&self->slot_item);
    item_info_clear(&self->addition_item);
    self->ok = 1;
    return self;
}

void roulette_destruct(struct Roulette *self)
{
    item_info_destruct(&self->addition_item);
    item_info_destruct(&self->slot_item);
    tree_free(&self->prizes, destroy_prize);
}

static void roulette_destroy(struct Roulette *self)
{
    roulette_destruct(self);
    mem_free(self);
}

unsigned char roulette_create_instance(void)
{
    struct Roulette *r;

    if (g_roulette)
        return 0;
    r = mem_try_alloc(sizeof *r);
    if (r)
        r = roulette_construct(r);
    g_roulette = r;
    if (!r)
        return 0;
    if (r->ok)
        return 1;
    roulette_destroy(r);
    g_roulette = NULL;
    return 0;
}

void run_faq_web_page(void)
{
    if (!g_roulette)
        return;
    roulette_destroy(g_roulette);
    g_roulette = NULL;
}

void roulette_add_prize(struct Roulette *self, struct RoulettePrize prize)
{
    struct ItemInfo item;
    struct PrizeEntry entry;

    item_info_construct(&item);
    item_info_set_item_id(&item, prize.item_id);
    item.count = (short)prize.count;
    item.identified = 1;
    entry.prize = prize;
    item_info_copy(&entry.item, &item);
    tree_add_multi(&self->prizes, &entry, sizeof entry, prize_less);
    item_info_destruct(&item);
}

void roulette_reset(struct Roulette *self)
{
    memset(self->marker, 0, sizeof self->marker + sizeof self->start + sizeof self->end);
    self->frames = 0;
    self->rounds = 0;
    self->step = 0;
    self->spin_column = 0;
    self->step_time = 0;
    self->interval = 0;
    self->spinning = 0;
}

void roulette_set_addition_item(struct Roulette *self, int item_id)
{
    struct TreeNode *node;

    if (item_id == -1) {
        item_info_clear(&self->addition_item);
        return;
    }
    self->addition_item_id = item_id;
    for (node = tree_first(&self->prizes); node != self->prizes.head; node = tree_next(node)) {
        struct PrizeEntry *entry = TREE_VALUE(node, struct PrizeEntry);

        if (entry->prize.item_id == item_id) {
            item_info_assign(&self->addition_item, &entry->item);
            return;
        }
    }
}

void roulette_set_slot(struct Roulette *self, unsigned short slot, BYTE has_slot,
                                 BYTE flag)
{
    struct TreeNode *node;

    self->slot = slot;
    self->has_slot = has_slot;
    self->slot_flag = flag;
    for (node = tree_first(&self->prizes); node != self->prizes.head; node = tree_next(node)) {
        struct PrizeEntry *entry = TREE_VALUE(node, struct PrizeEntry);

        if (self->step == (short)entry->prize.row && self->slot == (short)entry->prize.position)
            item_info_assign(&self->slot_item, &entry->item);
    }
}

static void marker_vertex(struct TexDrawVertex *v)
{
    v->z = 1e-6f;
    v->rhw = 0.999999f;
    v->diffuse = 0x44ffffff;
    v->u = 0.0f;
    v->v = 0.0f;
}

void roulette_place_marker(struct Roulette *self, int x, int y, int left, int top)
{
    struct TexDrawVertex *v = self->marker;
    float l, r, t, b;
    int i;

    if (g_renderer->width == 800 && g_renderer->height == 600) {
        y += 3;
        top += 3;
    }
    b = (float)y + MARKER_HALF + (float)top + 17.0f;
    l = (float)x - MARKER_HALF + (float)left;
    t = (float)y - MARKER_HALF + (float)top + 17.0f;
    r = (float)x + MARKER_HALF + (float)left;
    v[0].y = b;
    v[1].y = b;
    v[0].x = l;
    v[3] = v[0];
    v[2].x = r;
    v[2].y = t;
    v[4] = v[2];
    v[1].x = r;
    v[5].x = l;
    v[5].y = t;
    for (i = 0; i < 6; i++)
        marker_vertex(&v[i]);
}

void roulette_begin_spin(struct Roulette *self, int x, int y, int left, int top)
{
    int i;

    roulette_place_marker(self, x, y, left, top);
    memcpy(self->start, self->marker, sizeof self->start);
    for (i = 0; i < 6; i++) {
        self->end[i] = self->marker[i];
        self->end[i].x = self->marker[i].x - (float)(SLOTS_PER_ROW - self->step) * SLOT_STEP;
    }
    self->step_time = timeGetTime();
    self->interval = 0x23;
    self->frames = 0;
    self->rounds = 0;
    self->spinning = 1;
    self->has_slot = 0;
    ui_roulette_allow_spin(g_windowMgr.roullette_wnd, 0);
    if (g_windowMgr.roullette_wnd)
        ui_window_send_msg(g_windowMgr.roullette_wnd, NULL, WND_MSG_SPIN, 0, 0, 0, 0);
}

static void marker_step(struct Roulette *self)
{
    int i;

    for (i = 0; i < 6; i++)
        if (self->marker[i].x > self->end[i].x)
            self->marker[i].x -= SLOT_STEP;
    self->frames++;
    self->step_time = timeGetTime();
}

static void marker_stop(struct Roulette *self)
{
    self->has_slot = 1;
    roulette_place_marker(self, (self->step + self->slot) * 36 + 0xad, 0x155 - self->step * 40,
               (int)((double)g_renderer->width * 0.5 - 320.0),
               (int)((double)g_renderer->height * 0.5 - 240.0));
    ui_roulette_allow_spin(g_windowMgr.roullette_wnd, 1);
    ui_window_invalidate(g_windowMgr.roullette_wnd);
    self->spinning = 0;
}

void roulette_process(struct Roulette *self)
{
    float interval;

    if (self->spinning != 1)
        return;
    if (self->rounds > SPIN_SLOW_ROUNDS) {
        if (SLOTS_PER_ROW - self->step - self->slot <= self->frames) {
            marker_stop(self);
            return;
        }
        if (self->interval > timeGetTime() - self->step_time)
            return;
        marker_step(self);
        return;
    }
    if (self->interval > timeGetTime() - self->step_time)
        return;
    if (SLOTS_PER_ROW - self->step > self->frames) {
        marker_step(self);
        return;
    }
    memcpy(self->marker, self->start, sizeof self->marker);
    interval = (float)(double)self->interval;
    self->interval = (DWORD)(long long)(interval * 0.34f + interval);
    self->rounds++;
    self->frames = 0;
}

void roulette_render(struct Roulette *self)
{
    struct RPQuad *prim = renderer_new_quad(g_renderer);

    prim->texture = &g_whiteTexture;
    prim->verts = self->marker;
    prim->nverts = 6;
    renderer_add_prim(g_renderer, prim, RENDER_LIST_UI);
    roulette_process(self);
}
