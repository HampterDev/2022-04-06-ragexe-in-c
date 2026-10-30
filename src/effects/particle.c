#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "render/gfx.h"
#include "game/context.h"

#define MATRIX_BYTES (12 * sizeof(float))

static unsigned particle_color(const struct Particle *p)
{
    unsigned argb = (unsigned)p->alpha;
    argb = (argb << 8) | (unsigned)(int)p->r;
    argb = (argb << 8) | (unsigned)(int)p->g;
    argb = (argb << 8) | (unsigned)(int)p->b;
    return argb;
}

void particle_quad_unit_corners(struct TexDrawVertex *quad)
{
    quad[0].x = -1.0f; quad[0].y =  1.0f; quad[0].z = 1.0f;
    quad[1].x =  1.0f; quad[1].y =  1.0f; quad[1].z = 1.0f;
    quad[2].x = -1.0f; quad[2].y = -1.0f; quad[2].z = 1.0f;
    quad[3].x =  1.0f; quad[3].y = -1.0f; quad[3].z = 1.0f;
}

void particle_quad_paint(struct Particle *self, struct TexDrawVertex *quad,
                                    float v_top, float v_bottom, float u_left, float u_right)
{
    unsigned color = particle_color(self);
    quad[0].diffuse = quad[1].diffuse = quad[2].diffuse = quad[3].diffuse = color;
    quad[0].v = v_top;    quad[1].v = v_top;
    quad[2].v = v_bottom; quad[3].v = v_bottom;
    quad[0].u = u_left;   quad[2].u = u_left;
    quad[1].u = u_right;  quad[3].u = u_right;
}

void particle_submit(struct TexDrawPrim *batch, int list_flags)
{
    renderer_add_prim(g_renderer, &batch->base, list_flags);
}

void particle_reset(struct Particle *self)
{
    struct TexDrawPrim fresh;
    int i;

    self->r = 0xff;
    self->g = 0xff;
    self->b = 0xff;
    self->alpha = 0xff;
    self->scale = 1.0f;
    self->texture = 0;
    self->owner_extra = 0;
    memset(self->effect_data, 0, sizeof self->effect_data);
    self->roll = 0;
    self->roll_extra = 0;
    self->life_seconds = 0;
    self->age = 0;
    self->flags = 0;
    self->offset.x = self->offset.y = self->offset.z = 0;
    self->src_blend = GFX_BLEND_SRCALPHA;
    self->dest_blend = GFX_BLEND_ONE;
    rp_quad_batch_copy_verts(&self->batch, rp_quad_batch_construct(&fresh));
    memcpy(self->matrix, g_identityMatrix.m, MATRIX_BYTES);
    self->depth_bias = 0;
    for (i = 0; i < 4; i++) {
        self->corner_points[i].x = self->corner_points[i].y = self->corner_points[i].z = 0;
    }
    particle_quad_unit_corners(self->corners);
    self->submit = particle_submit;
}

struct Particle *particle_construct(struct Particle *self, Vec3 pos, int list_flags)
{
    int i;

    self->pool_kind = PARTICLE_KIND_PLAIN;
    self->list_flags = list_flags;
    self->pos = pos;
    self->velocity.x = self->velocity.y = self->velocity.z = 0;
    self->offset.x = self->offset.y = self->offset.z = 0;
    self->second_offset.x = self->second_offset.y = self->second_offset.z = 0;
    rp_quad_batch_construct(&self->batch);
    for (i = 0; i < 4; i++) {
        memset(&self->corners[i], 0, sizeof(self->corners[i]));
        self->corners[i].specular = 0xff000000;
    }
    particle_reset(self);
    return self;
}

void particle_set_color(struct Particle *self, short r, short g, short b)
{
    if (r != PARTICLE_KEEP_CHANNEL)
        self->r = r;
    if (g != PARTICLE_KEEP_CHANNEL)
        self->g = g;
    if (b != PARTICLE_KEEP_CHANNEL)
        self->b = b;
}

void particle_set_texture(struct Particle *self, struct String name)
{
    self->texture = texture_cache_get(&g_textureCache, string_cstr(&name), 0, 0);
    string_free(&name);
}

static unsigned billboard_last_deg;
static int billboard_valid;
static float billboard_last_rot[12];

static void billboard_rotation(float turn[12])
{
    const float heading = game_camera_heading();
    unsigned deg_bits;

    memcpy(&deg_bits, &heading, sizeof deg_bits);

    if (billboard_valid && deg_bits == billboard_last_deg) {
        memcpy(turn, billboard_last_rot, MATRIX_BYTES);
        return;
    }
    {
        float deg, rad;
        memcpy(&deg, &deg_bits, sizeof(deg));
        rad = deg / g_halfTurnDegrees * g_pi;
        memcpy(turn, g_identityMatrix.m, MATRIX_BYTES);
        mat_rotate_y(turn, rad);
    }
    memcpy(billboard_last_rot, turn, MATRIX_BYTES);
    billboard_last_deg = deg_bits;
    billboard_valid = 1;
}

void particle_render(struct Particle *self, unsigned char facing_fixed,
                                  float v_top, float v_bottom, float u_left, float u_right, unsigned char with_depth)
{
    float view[12], world[12], to_screen[12];
    struct TexDrawVertex *v;
    int i, has_corner = 0;

    for (i = 0; i < 4; i++)
        if (self->corners[i].x != 0 || self->corners[i].y != 0 || self->corners[i].z != 0)
            has_corner = 1;
    if (!has_corner || !game_copy_view_matrix(view))
        return;

    particle_quad_paint(self, self->corners, v_top, v_bottom, u_left, u_right);
    memcpy(self->batch.base.verts, self->corners, sizeof(self->corners));

    if (!facing_fixed) {
        float move[12], facing[12], placed[12];
        mat_set_translation(move, self->pos.x, self->pos.y, self->pos.z);
        billboard_rotation(facing);
        mat_multiply(placed, facing, move);
        mat_multiply(world, self->matrix, placed);
        mat_multiply(to_screen, world, view);
    } else {
        mat_multiply(to_screen, self->matrix, view);
    }

    for (i = 0, v = self->batch.base.verts; i < 4; i++, v++) {
        Vec3 corner;
        corner.x = v->x;
        corner.y = v->y;
        corner.z = v->z;
        renderer_project_vertex(g_renderer, &corner, to_screen, v);
        v->rhw += self->depth_bias;
        v->z = with_depth ? renderer_project_depth(v->rhw) : 0.0f;
    }
    self->batch.base.texture = self->texture;
    self->batch.base.srcBlend = self->src_blend;
    self->batch.base.destBlend = self->dest_blend;
    renderer_add_prim(g_renderer, &self->batch.base, self->list_flags);
}

void particle_render_with(struct Particle *self, const struct ParticleRenderArgs *args)
{
    particle_render(self, args->facing_fixed, args->v_top, args->v_bottom, args->u_left, args->u_right, args->with_depth);
}
