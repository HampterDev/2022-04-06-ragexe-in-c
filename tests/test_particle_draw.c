#include "render/rp_pool.h"
#include "effects/particle.h"
#include "render/render_api.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
struct Session g_session1;
const double k_strAnglePerDegree = 1024.0 / 360.0;
static struct Ez2StrParticle particle;
static struct StrAnim animation;
static struct TexDrawVertex vertices[4];
static struct TexDrawPrim batch;
static struct Texture texture;
static int draws, projections, scale_queries;
static int flipped;

struct TexDrawPrim *renderer_new_prim(struct Renderer *r)
{
    assert(r == &renderer);
    batch.base.verts = vertices;
    return &batch;
}

void renderer_project_point(struct Renderer *r, const Vec3 *pos, const float *view,
                            float *x, float *y, float *rhw)
{
    (void)pos;
    assert(r == &renderer && view);
    ++projections;
    *x = 100.0f;
    *y = 200.0f;
    *rhw = 0.5f;
}

float renderer_screen_scale(struct Renderer *r, float rhw)
{
    assert(r == &renderer && rhw == 0.5f);
    ++scale_queries;
    return 2.0f;
}

unsigned char session_is_screen_flipped(void *session)
{
    assert(session == &g_session1);
    return (unsigned char)flipped;
}

void *effect_find_actor(int aid) { (void)aid; return NULL; }
void render_object_apply_shrink_scale(void *actor, float *scale)
{
    (void)actor; (void)scale;
    assert(!"no actor in this scene");
}

void *str_layer_get_texture(struct StrLayer *layer, int frame)
{
    assert(layer == &animation.layers[1] && frame == 0);
    return &texture;
}

void *str_layer_get_set_texture(struct StrLayer *layer, int frame, const char *set)
{
    assert(set != NULL);
    return str_layer_get_texture(layer, frame);
}

static void submit(struct TexDrawPrim *value, int flags)
{
    assert(value == &batch && value->base.texture == &texture);
    assert(flags == (particle.base.list_flags | 0x20));
    ++draws;
}

int main(void)
{
    float view[12] = { 0 };
    struct StrKeyState *state = &particle.state[1];
    animation.layer_count = 2;
    particle.anim = &animation;
    particle.visible = particle.drawn[1] = 1;
    particle.zoom = particle.alpha_scale = 1.0f;
    particle.base.submit = submit;
    particle.base.list_flags = PARTICLE_LIST_UI_SCREEN;
    particle.screen_pos.x = 328.0f;
    particle.screen_pos.y = 198.0f;
    state->x = STR_SCREEN_CX;
    state->y = STR_SCREEN_CY;
    state->corner_x[0] = state->corner_x[3] = -10.0f;
    state->corner_x[1] = state->corner_x[2] = 10.0f;
    state->corner_y[0] = state->corner_y[1] = -5.0f;
    state->corner_y[2] = state->corner_y[3] = 5.0f;
    state->us = state->vs = 1.0f;
    state->color[0] = state->color[1] = state->color[2] = state->color[3] = 255.0f;
    texture.base.width = texture.base.height = 64;
    texture.reqWidth = texture.reqHeight = 32;

    ez2_str_particle_render(&particle, NULL);
    assert(draws == 1 && projections == 0 && scale_queries == 0);
    assert(vertices[0].x == 318.0f && vertices[0].y == 203.0f);
    assert(vertices[2].x == 338.0f && vertices[1].y == 193.0f);
    assert(vertices[2].u == 0.5f && vertices[0].diffuse == 0xffffffffu);
    assert(vertices[0].rhw > 0.9999f && vertices[0].rhw < 1.0f);

    particle.zoom = 2.0f;
    ez2_str_particle_render(&particle, NULL);
    assert(draws == 2 && vertices[0].x == 308.0f && vertices[0].y == 208.0f);
    particle.zoom = 1.0f;
    flipped = 1;
    ez2_str_particle_render(&particle, NULL);
    assert(draws == 3 && vertices[0].y == 193.0f);
    flipped = 0;

    ez2_str_particle_render(&particle, view);
    assert(draws == 4 && projections == 1 && scale_queries == 1);
    assert(vertices[0].x == 308.0f && vertices[0].y == 208.0f);
    particle.visible = 0;
    ez2_str_particle_render(&particle, NULL);
    assert(draws == 4);
    particle.visible = 1;
    particle.base.list_flags = 5;
    ez2_str_particle_render(&particle, NULL);
    assert(draws == 4);
    puts("particle drawing: UI pixels, zoom, flipping, visibility and active camera projection passed");
    return 0;
}
