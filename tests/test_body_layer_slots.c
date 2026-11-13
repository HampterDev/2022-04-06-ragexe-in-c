#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <contents/body_layer.c>
#include "std/std.h"
#include "body_layer_slots_unreached.h"

/* One player's layers in a frame live in fixed arrays: the sprite slots, the worn layers' views and
   their quads queued by priority, which the flush draws in priority order on top of each other. */

static unsigned allocations;
void *mem_alloc(size_t n)
{
    ++allocations;
    return malloc(n);
}
void mem_free(void *p)
{
    free(p);
}
__declspec(noreturn) void fatal_error(const char *message)
{
    (void)message;
    abort();
}

static struct SqQuad drawn[BODY_LAYER_COUNT + 1];
static void *drawn_palette[BODY_LAYER_COUNT + 1];
static int drawn_flags[BODY_LAYER_COUNT + 1], drawn_count, flashes;
static int spr[BODY_LAYER_COUNT], act[BODY_LAYER_COUNT];
static int replacing_view = 77;

BYTE session_is_screen_flipped(struct Session *session)
{
    assert(session == &g_session1);
    return 0;
}
void render_object_submit_spr_quad(struct RenderObject *object, struct SqQuad *quad, const void *palette,
                                   int flags, float x, float y)
{
    assert(object && x == 0.0f && y == 0.0f && drawn_count <= BODY_LAYER_COUNT);
    drawn[drawn_count] = *quad;
    drawn_palette[drawn_count] = (void *)palette;
    drawn_flags[drawn_count++] = flags;
}
void render_object_draw_flashes(struct RenderObject *object, struct SqQuad quad, void *palette)
{
    (void)object; (void)quad; (void)palette;
    ++flashes;
}
struct SprRes *pc_layer_spr(struct Pc *pc, int layer)
{
    assert(pc && (unsigned)layer < BODY_LAYER_COUNT);
    return layer == 6 ? NULL : (struct SprRes *)&spr[layer];
}
struct ActRes *pc_layer_act(struct Pc *pc, int layer)
{
    assert(pc && (unsigned)layer < BODY_LAYER_COUNT);
    return (struct ActRes *)&act[layer];
}
BYTE effect_state_bit11(DWORD state)
{
    return (BYTE)((state >> 11) & 1);
}
BYTE session_is_head_replacing_view(struct Session *session, int view)
{
    assert(session == &g_session1);
    return view == replacing_view;
}

static struct SqQuad quad_at(float depth_a, float depth_b)
{
    struct SqQuad quad;
    memset(&quad, 0, sizeof quad);
    quad.depth_a = depth_a;
    quad.depth_b = depth_b;
    return quad;
}

static void slots(void)
{
    struct BodyLayerDraw draw;
    struct FoundSprites found;
    struct LayerSprite *slot;
    int layer;
    memset(&draw, 0, sizeof draw);
    for (layer = 0; layer < BODY_LAYER_COUNT; ++layer) {
        body_layer_find_sprites(&draw, &found, layer);
        assert(!found.found && !found.pair.spr && !found.pair.act);
        slot = body_layer_sprite_slot(&draw, layer);
        assert(slot->present && !slot->pair.spr && !slot->pair.act);
        slot->pair.spr = (struct SprRes *)&spr[layer];
        slot->pair.act = (struct ActRes *)&act[layer];
        assert(body_layer_sprite_slot(&draw, layer) == slot && slot->pair.spr == (struct SprRes *)&spr[layer]);
        body_layer_find_sprites(&draw, &found, layer);
        assert(found.found && found.pair.spr == slot->pair.spr && found.pair.act == slot->pair.act);
    }
    body_layer_find_sprites(&draw, &found, 99);
    assert(!found.found && !found.pair.spr && !found.pair.act);
    body_layer_find_sprites(&draw, &found, -1);
    assert(!found.found);
}

/* The worn layer whose view replaces the head gives the head its sprites and is drawn no more; the
   views are the accessories' and the robe's, and nothing is left queued from a frame before. */
static void reset(void)
{
    struct BodyLayerDraw draw;
    struct FoundSprites found;
    struct Pc pc;
    int layer;
    memset(&draw, 0xcd, sizeof draw);
    memset(&pc, 0, sizeof pc);
    pc.accessory[0] = 11;
    pc.accessory[1] = replacing_view;
    pc.accessory[2] = 33;
    pc.robe = 44;
    draw.pc = &pc;
    body_layer_reset(&draw);
    assert(!draw.layers_drawn && !draw.head_drawn && !draw.any_drawn && !draw.queued_count);
    assert(draw.views[2] == 11 && draw.views[3] == replacing_view && draw.views[4] == 33 && draw.views[8] == 44);
    for (layer = 0; layer < BODY_LAYER_COUNT; ++layer) {
        body_layer_find_sprites(&draw, &found, layer);
        if (layer == 3 || layer == 6) {
            assert(!found.found);
        } else if (layer == BODY_LAYER_HEAD) {
            assert(found.found && found.pair.spr == (struct SprRes *)&spr[3] && found.pair.act == (struct ActRes *)&act[3]);
        } else {
            assert(found.found && found.pair.spr == (struct SprRes *)&spr[layer]);
            assert(found.pair.act == (struct ActRes *)&act[layer]);
        }
        if (layer != 2 && layer != 3 && layer != 4 && layer != 8)
            assert(!draw.views[layer]);
    }
    pc.accessory[1] = 12;
    body_layer_reset(&draw);
    body_layer_find_sprites(&draw, &found, BODY_LAYER_HEAD);
    assert(found.found && found.pair.spr == (struct SprRes *)&spr[1]);
    body_layer_find_sprites(&draw, &found, 3);
    assert(found.found && found.pair.spr == (struct SprRes *)&spr[3]);
}

/* Queued by priority, the first of equal priorities kept; drawn in that order, each at the
   deepest depth of them all plus a millionth for each drawn before it. */
static void queue_and_flush(void)
{
    static const int priorities[] = {300, 200, 300, -5, 100, 200, 0};
    static const float depths[][2] = {{0.25f, 0.5f}, {0.75f, 0.125f}, {0.9f, 0.9f}, {0.5f, 0.625f},
                                      {0.375f, 0.0f}, {0.1f, 0.1f}, {0.0f, 0.3f}};
    static const int kept[] = {3, 6, 4, 1, 0};
    struct BodyLayerDraw draw;
    struct Pc pc;
    float step = 0.0f;
    int i;
    memset(&draw, 0, sizeof draw);
    memset(&pc, 0, sizeof pc);
    draw.pc = &pc;
    for (i = 0; i < 7; ++i) {
        struct SqQuad quad = quad_at(depths[i][0], depths[i][1]);
        body_layer_queue(&draw, priorities[i], &quad, &drawn_palette[i], i);
    }
    assert(draw.queued_count == 5);
    for (i = 0; i < 5; ++i)
        assert(draw.queued[i].priority == priorities[kept[i]] && draw.queued[i].flags == kept[i]);
    drawn_count = flashes = 0;
    body_layer_flush(&draw);
    assert(drawn_count == 5 && flashes == 5);
    for (i = 0; i < 5; ++i) {
        assert(drawn_flags[i] == kept[i] && drawn_palette[i] == &drawn_palette[kept[i]]);
        assert(drawn[i].depth_a == step + 0.75f && drawn[i].depth_b == step + 0.625f);
        step += 1.0e-6f;
    }
    memset(&draw, 0, sizeof draw);
    draw.pc = &pc;
    for (i = 0; i < BODY_LAYER_COUNT + 3; ++i) {
        struct SqQuad quad = quad_at((float)i, -(float)i);
        body_layer_queue(&draw, 50 - i, &quad, NULL, i);
    }
    assert(draw.queued_count == BODY_LAYER_COUNT && draw.queued[0].priority == 50 - (BODY_LAYER_COUNT - 1));
    drawn_count = 0;
    body_layer_flush(&draw);
    assert(drawn_count == BODY_LAYER_COUNT && drawn[0].depth_a == (float)(BODY_LAYER_COUNT - 1)
           && drawn[0].depth_b == 0.0f);
    memset(&draw, 0, sizeof draw);
    draw.pc = &pc;
    drawn_count = 0;
    body_layer_flush(&draw);
    assert(!drawn_count);
}

int main(void)
{
    slots();
    reset();
    queue_and_flush();
    assert(!allocations);
    puts("Body layer slots: sprite slots, the head swapped for a replacing view, views, a queue by "
         "priority keeping the first of equal ones, bounded, the flush's order and depths, and no "
         "allocation passed.");
    return 0;
}
