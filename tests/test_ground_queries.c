#include "resources/attr.h"
#include "effects/rag/rag_effect.h"
#include "world/ground_api.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct C3dAttr grid;
static struct AttrCell cells[4];
static struct EffectPrim particle;
static struct RagEffect master;
static int present, attr_queries, alpha_steps, trail_steps, motion_steps;

struct C3dAttr *game_current_map_attr(void)
{
    ++attr_queries;
    return present ? &grid : NULL;
}

struct C3dGround15 *game_current_ground(void)
{
    abort();
}

void prim_update_alpha(struct EffectPrim *self)
{ assert(self == &particle); ++alpha_steps; }
void prim_shift_trail(struct EffectPrim *self)
{ assert(self == &particle); ++trail_steps; }
unsigned char prim_step_motion(struct EffectPrim *self)
{ assert(self == &particle); ++motion_steps; return 1; }
void effect_play_wave_at(const char *name, const Vec3 *position, float height)
{ (void)name; (void)position; (void)height; abort(); }

static void test_height_queries(void)
{
    RECT bounds = {0, 0, 1, 1};
    float low, high;
    int i, j;
    grid.width = grid.height = 2;
    grid.zoom = 5;
    grid.cells = cells;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            cells[i].height[j] = (float)(i * 10 + j * 2);
    assert(c3d_attr_get_height(&grid, -5.0f, -5.0f) == 0.0f);
    assert(c3d_attr_get_height(&grid, -2.5f, -2.5f) == 3.0f);
    assert(c3d_attr_get_height(&grid, 2.5f, 2.5f) == 33.0f);
    assert(c3d_attr_get_height(&grid, 20.0f, 20.0f) == 0.0f);
    c3d_attr_get_height_min_max(&grid, &bounds, &low, &high);
    assert(low == 0.0f && high == 36.0f);
    bounds.left = bounds.top = 1;
    c3d_attr_get_height_min_max(&grid, &bounds, &low, &high);
    assert(low == 30.0f && high == 36.0f);
}

static void bounce_from(float y, int has_map, float expected_speed)
{
    memset(&particle, 0, sizeof particle);
    memset(&master, 0, sizeof master);
    particle.master = &master;
    particle.matrix[0] = particle.matrix[4] = particle.matrix[8] = 1.0f;
    particle.delta.y = y;
    particle.gravity = 3.0f;
    present = has_map;
    assert(prim_bounce_process(&particle));
    assert(particle.base.pos.y == y + 3.0f);
    assert(particle.fall_speed == expected_speed);
}

int main(void)
{
    int i, j;
    test_height_queries();
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            cells[i].height[j] = 12.0f;
    bounce_from(20.0f, 1, -2.0f);
    bounce_from(0.0f, 1, 3.0f);
    bounce_from(20.0f, 0, 3.0f);
    assert(attr_queries == 3 && alpha_steps == 3 && trail_steps == 3 && motion_steps == 3);
    puts("ground queries: real grid interpolation, height bounds and particle/map type separation passed");
    return 0;
}
