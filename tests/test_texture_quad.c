#include "render/render_api.h"
#include "render/texture_cache.h"
#include "session/state.h"
#include "render/screen_prim.h"
#include "render/math3d.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static struct Renderer renderer;
static struct Texture texture;
static struct TexDrawPrim primitive;
static int allocations, submissions, projections, fail_allocation;
static float expected_rhw;
struct TexCache g_textureCache;
const struct TextureUV g_fullUV = {0, 0, 1, 1};
struct Session g_session1;

struct ResMgr *g_resMgr(void) { assert(0); return NULL; }
void *res_mgr_get(struct ResMgr *self, const char *name)
{
    (void)self; (void)name;
    assert(0);
    return NULL;
}
void screen_sprite_frame(struct Renderer *renderer, int x, int y, struct ActRes *act, struct SprRes *spr,
                         int action, int motion, float scale, float angle, DWORD colour, int flags)
{
    (void)renderer; (void)x; (void)y; (void)act; (void)spr;
    (void)action; (void)motion; (void)scale; (void)angle; (void)colour; (void)flags;
    assert(0);
}

#include "../src/render/rp_hud.c"

struct Texture *texture_cache_get(struct TexCache *cache,
                                             const char *name, int a, int b)
{
    (void)cache; (void)name; (void)a; (void)b;
    assert(0);
    return NULL;
}

struct TexDrawPrim *renderer_new_prim(struct Renderer *r)
{
    int i;
    assert(r == &renderer);
    ++allocations;
    if (fail_allocation)
        return NULL;
    memset(&primitive, 0, sizeof primitive);
    primitive.base.verts = primitive.quad;
    for (i = 0; i < 4; ++i)
        primitive.base.verts[i].specular = 0x12345678;
    return &primitive;
}

void renderer_add_prim(struct Renderer *r, struct RPQuad *p, int flags)
{
    assert(r == &renderer && p == &primitive.base);
    assert(flags == SCREEN_LIST && p->texture == &texture);
    ++submissions;
}

float renderer_project_depth(float rhw)
{
    assert(rhw == expected_rhw);
    ++projections;
    return 0.125f;
}

static void near_value(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.0001f);
}

static void check_quad(const float positions[4][2], DWORD colour, const float uv[4][2])
{
    int i;
    assert(primitive.base.texture == &texture);
    for (i = 0; i < 4; ++i) {
        const struct TexDrawVertex *v = &primitive.base.verts[i];
        near_value(v->x, positions[i][0]);
        near_value(v->y, positions[i][1]);
        assert(v->z == 0.125f && v->rhw == expected_rhw);
        assert(v->diffuse == colour && v->specular == 0x12345678);
        near_value(v->u, uv[i][0]);
        near_value(v->v, uv[i][1]);
    }
}

int main(void)
{
    RECT rect = {10, 20, 30, 60};
    const struct TextureUV crop = {0.25f, 0.5f, 0.75f, 1.0f};
    const float upright[4][2] = {{10, 20}, {30, 20}, {10, 60}, {30, 60}};
    const float clockwise[4][2] = {{40, 30}, {40, 50}, {0, 30}, {0, 50}};
    const float anticlockwise[4][2] = {{0, 50}, {0, 30}, {40, 50}, {40, 30}};
    const float cropped[4][2] = {{0.1953125f, 0.390625f}, {0.5859375f, 0.390625f},
                                  {0.1953125f, 0.78125f}, {0.5859375f, 0.78125f}};

    math_init_degree_tables();
    texture.base.width = 128;
    texture.base.height = 64;
    texture.reqWidth = 100;
    texture.reqHeight = 50;
    expected_rhw = 0.0999f;
    screen_texture_rect(&renderer, &rect, 0, expected_rhw, 0x80aabbcc, &texture, crop);
    check_quad(upright, 0x80aabbcc, cropped);
    screen_texture_rect(&renderer, &rect, 90, expected_rhw, 0xffffffff, &texture, crop);
    check_quad(clockwise, 0xffffffff, cropped);
    screen_texture_rect(&renderer, &rect, -90, expected_rhw, 0x00112233, &texture, crop);
    check_quad(anticlockwise, 0x00112233, cropped);
    assert(allocations == 3 && submissions == 3 && projections == 3);

    screen_texture_rect(NULL, &rect, 0, expected_rhw, 0, &texture, crop);
    screen_texture_rect(&renderer, NULL, 0, expected_rhw, 0, &texture, crop);
    screen_texture_rect(&renderer, &rect, 0, expected_rhw, 0, NULL, crop);
    texture.base.width = 0;
    screen_texture_rect(&renderer, &rect, 0, expected_rhw, 0, &texture, crop);
    texture.base.width = 128;
    texture.base.height = -1;
    screen_texture_rect(&renderer, &rect, 0, expected_rhw, 0, &texture, crop);
    assert(allocations == 3 && submissions == 3 && projections == 3);
    texture.base.height = 64;
    fail_allocation = 1;
    screen_texture_rect(&renderer, &rect, 0, expected_rhw, 0, &texture, crop);
    assert(allocations == 4 && submissions == 3 && projections == 3);

    puts("PASS: texture geometry and resource failures");
    return 0;
}
