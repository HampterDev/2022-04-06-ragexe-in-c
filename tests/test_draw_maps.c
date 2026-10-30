#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <contents/body_layer.c>
#include "std/std.h"
#include "draw_maps_unreached.h"
#ifdef DRAW_OWNERS_REFERENCE
#define DESTROY_BODY FuncRenderBodyLayer_Destruct
const void *CSpriteResPair_ops[1];
#else
#define DESTROY_BODY body_layer_destroy
#endif

static struct {
    unsigned char *p;
    size_t n;
    unsigned id;
} blocks[256];
static unsigned allocation_id;
static unsigned live_blocks, trace[32768], trace_count;
static int spr, act;
static void note(unsigned value)
{
    assert(trace_count < 32768);
    trace[trace_count++] = value;
}
void *mem_alloc(size_t n)
{
    unsigned i;
    unsigned char *p = malloc(n + 16);
    assert(p);
    memset(p, 0xa5, n + 16);
    for (i = 0; i < 256 && blocks[i].p; ++i) {
    }
    assert(i < 256);
    blocks[i].p = p;
    blocks[i].n = n;
    blocks[i].id = ++allocation_id;
    note(800);
    note(blocks[i].id);
    ++live_blocks;
    return p;
}
void mem_free(void *p)
{
    unsigned i, j;
    if (!p)
        return;
    for (i = 0; i < 256 && blocks[i].p != p; ++i) {
    }
    assert(i < 256);
    for (j = 0; j < 16; ++j)
        assert(blocks[i].p[blocks[i].n + j] == 0xa5);
    note(801);
    note(blocks[i].id);
    free(p);
    blocks[i].p = NULL;
    --live_blocks;
}
__declspec(noreturn) void fatal_error(const char *message)
{
    (void)message;
    abort();
}

static void body_maps(void)
{
    struct BodyLayerDraw draw;
    struct FoundSprites found;
    struct QueuedLayer queued = {0};
    int i;
    memset(&draw, 0, sizeof draw);
    tree_init(&draw.sprites);
    tree_init(&draw.views);
    tree_init(&draw.queued);
    for (i = 8; i >= 0; --i) {
        struct LayerSprite *node = body_layer_sprite_slot(&draw, i);
        assert(!node->pair.spr && !node->pair.act);
        node->pair.spr = (struct SprRes *)&spr;
        node->pair.act = (struct ActRes *)&act;
        assert(body_layer_sprite_slot(&draw, i) == node);
        body_layer_find_sprites(&draw, &found, i);
        assert(found.found && found.pair.spr == node->pair.spr && found.pair.act == node->pair.act);
        *body_layer_view_slot(&draw, i) = 100 + i;
        queued.priority = i % 3;
        queued.flags = i;
        tree_add(&draw.queued, &queued, sizeof queued, tree_less_int, NULL);
        note(*body_layer_view_slot(&draw, i));
    }
    body_layer_find_sprites(&draw, &found, 99);
    assert(!found.found && !found.pair.spr && !found.pair.act);
    assert(draw.sprites.size == 9 && draw.views.size == 9 && draw.queued.size == 3);
    for (i = 0; i < 3; ++i) {
        struct QueuedLayer *entry = tree_find(&draw.queued, &i, tree_less_int);
        assert(entry->flags == 6 + i);
        note(entry->flags);
    }
    DESTROY_BODY(&draw);
    assert(!live_blocks);
}

int main(int argc, char **argv)
{
    unsigned i;
    for (i = 0; i < 32; ++i) {
        body_maps();
    }
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb");
        assert(f);
        assert(fwrite(trace, sizeof *trace, trace_count, f) == trace_count);
        assert(!fclose(f));
    }
    puts("Draw maps: 32 lifecycle rounds, borrowed pairs, duplicate priorities and "
         "guarded cleanup passed.");
    return 0;
}
