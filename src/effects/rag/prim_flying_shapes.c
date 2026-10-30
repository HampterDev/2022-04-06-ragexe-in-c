#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define PATTERN_RING_FULL_WIDTH   0x001
#define PATTERN_PYRAMID_STOP_AT   0x001
#define PATTERN_PYRAMID_LATE_TURN 0x002
#define PATTERN_BAND_CLOSE_TOP    0x001

void prim_pyramid_process(struct EffectPrim *self)
{
    float speed;

    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    if (self->frame == self->change_frame) {
        const float *next = &self->next_speed;

        self->accel = self->next_accel;
        speed = *next;
    } else {
        speed = self->speed;
    }
    self->speed = self->accel + speed;
    self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->speed);
    self->delta.x += self->velocity.x;
    self->delta.y += self->velocity.y;
    self->delta.z += self->velocity.z;
    self->base.pos.x = self->delta.x + self->master_pos.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;

    if ((self->pattern & PATTERN_PYRAMID_STOP_AT) && self->frame == self->pattern_arg) {
        self->accel = 0.0f;
        self->speed = 0.0f;
    }
    if (self->pattern & PATTERN_PYRAMID_LATE_TURN) {
        if (self->frame == self->duration - 30) {
            self->speed = self->next_velocity.y;
            self->accel = self->next_velocity_accel.y;
        }
        return;
    }
    prim_update_alpha(self);
}

void prim_pyramid_render(struct EffectPrim *self, const float *view)
{
    float s = self->scale;
    float h = self->size[2].value;
    float ns = -s;
    Vec3 corner[12];
    float u = 0.0f;
    int face, k;

    corner[0].x = ns; corner[0].y = ns; corner[0].z = 0.0f;
    corner[1].x = 0.0f; corner[1].y = 0.0f; corner[1].z = h;
    corner[2].x = s; corner[2].y = ns; corner[2].z = 0.0f;
    corner[3].x = ns; corner[3].y = ns; corner[3].z = 0.0f;
    corner[4].x = 0.0f; corner[4].y = 0.0f; corner[4].z = h;
    corner[5].x = ns; corner[5].y = s; corner[5].z = 0.0f;
    corner[6].x = ns; corner[6].y = s; corner[6].z = 0.0f;
    corner[7].x = 0.0f; corner[7].y = 0.0f; corner[7].z = h;
    corner[8].x = s; corner[8].y = s; corner[8].z = 0.0f;
    corner[9].x = s; corner[9].y = s; corner[9].z = 0.0f;
    corner[10].x = 0.0f; corner[10].y = 0.0f; corner[10].z = h;
    corner[11].x = s; corner[11].y = ns; corner[11].z = 0.0f;

    if (self->records.pyramid.style == PYRAMID_STYLE_FLICKER) {
        unsigned red = (unsigned)(rand() % 11);
        unsigned green = (unsigned)(rand() % 11);
        self->argb = 0xff2828ffu + (red << 16) + (green << 8);
    }

    for (face = 0; face < 4; face++) {
        struct PrimTriangle *tri = &self->triangles[face];
        Vec3 p[3];

        for (k = 0; k < 3; k++) {
            const Vec3 *c = &corner[face * 3 + k];
            p[k] = prim_mat_apply(self->matrix2, c->x, c->y, c->z);
            p[k].x = self->base.pos.x + p[k].x;
            p[k].y = self->base.pos.y + p[k].y;
            p[k].z = p[k].z + self->base.pos.z;
        }
        prim_set_vertex(&tri->v[0], view, p[0], self->argb, u, 1.0f);
        prim_set_vertex(&tri->v[1], view, p[1], self->argb, u, 0.0f);
        u = u + 0.2f;
        prim_set_vertex(&tri->v[2], view, p[2], self->argb, u, 1.0f);
        tri->texture = self->textures[0];
        if (self->records.pyramid.style == PYRAMID_STYLE_FLICKER) {
            tri->src_blend = GFX_BLEND_SRCALPHA;
            tri->dest_blend = GFX_BLEND_ONE;
            renderer_add_triangle(g_renderer, tri, 5);
        } else {
            renderer_add_triangle(g_renderer, tri, 1);
        }
    }
}

void prim_corner_trail_render(struct EffectPrim *self, const float *view)
{
    int i;

    for (i = 0; i < self->particle_count; i++) {
        const struct PrimParticle *part = &self->particles[i];
        const Vec3 *corner = part->corners;
        struct PrimTriangle *tri = &self->triangles[i * 2];

        prim_set_vertex(&tri->v[0], view, corner[0], part->argb, 0.0f, 1.0f);
        prim_set_vertex(&tri->v[1], view, corner[1], part->argb, 1.0f, 1.0f);
        prim_set_vertex(&tri->v[2], view, corner[2], part->argb, 0.0f, 0.0f);
        tri->texture = self->textures[0];
        renderer_add_triangle(g_renderer, tri, 1);

        tri++;
        prim_set_vertex(&tri->v[0], view, corner[3], part->argb, 1.0f, 0.0f);
        prim_set_vertex(&tri->v[1], view, corner[2], part->argb, 0.0f, 0.0f);
        prim_set_vertex(&tri->v[2], view, corner[1], part->argb, 1.0f, 1.0f);
        tri->texture = self->textures[0];
        renderer_add_triangle(g_renderer, tri, 1);
    }
}

void prim_ring_process(struct EffectPrim *self)
{
    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    if (self->change_frame == self->frame) {
        const float *next = &self->next_speed;

        self->size[0].accel = self->next_accel;
        self->size[0].speed = *next;
    }
    self->size[0].speed = self->size[0].accel + self->size[0].speed;
    self->speed += self->accel;
    self->angle_speed[0] += self->angle_accel[0];
    self->radius_speed[0] += self->radius_accel[0];
    self->size[0].value += self->size[0].speed;
    self->angle[0] += self->angle_speed[0];
    self->radius[0] += self->radius_speed[0];

    mat_set_rotate_x(self->matrix, deg_to_rad((int)self->angle[1]));
    mat_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
    self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->speed);
    self->delta.x += self->velocity.x;
    self->delta.y += self->velocity.y;
    self->delta.z += self->velocity.z;
    self->base.pos.x = self->delta.x + self->master_pos.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;

    if ((self->pattern & PATTERN_RING_FULL_WIDTH) || self->radius[0] > self->size[0].value)
        self->scale = self->size[0].value;
    else
        self->scale = self->radius[0];
    prim_update_alpha(self);
    prim_shift_trail(self);
}

static Vec3 shape_point(const float *m, float x, float y, float z, const Vec3 *at)
{
    Vec3 p = prim_mat_apply(m, x, y, z);

    p.x += at->x;
    p.y += at->y;
    p.z += at->z;
    return p;
}

void prim_ring_render(struct EffectPrim *self, const float *view)
{
    float limit = 360.0f - self->arc_degrees;
    float u = 0.0f;
    int count = 0;
    int i;

    for (i = 0; i < self->particle_count; i++) {
        const struct PrimParticle *part = &self->particles[i];
        float outer = part->size;
        float inner = outer - self->scale;
        float a = 0.0f;

        if (!(limit >= 0.0f))
            continue;
        do {
            struct PrimTriangle *tri = &self->triangles[count];
            float b = self->arc_degrees + a;
            float c0 = g_cosTable[(int)a], s0 = g_sinTable[(int)a];
            float c1 = g_cosTable[(int)b], s1 = g_sinTable[(int)b];
            Vec3 in_a = shape_point(self->matrix, c0 * inner, s0 * inner, 0.0f, &part->pos);
            Vec3 in_b = shape_point(self->matrix, c1 * inner, s1 * inner, 0.0f, &part->pos);
            Vec3 out_a = shape_point(self->matrix, outer * c0, outer * s0, 0.0f, &part->pos);
            Vec3 out_b = shape_point(self->matrix, outer * c1, outer * s1, 0.0f, &part->pos);
            float u1 = u + 0.25f;

            prim_set_vertex(&tri->v[0], view, in_a, part->argb, u, 1.0f);
            prim_set_vertex(&tri->v[1], view, in_b, part->argb, u1, 1.0f);
            prim_set_vertex(&tri->v[2], view, out_a, part->argb, u, 0.0f);
            tri->texture = self->textures[0];
            renderer_add_triangle(g_renderer, tri, self->list_flags);

            tri++;
            prim_set_vertex(&tri->v[0], view, out_b, part->argb, u1, 0.0f);
            prim_set_vertex(&tri->v[1], view, out_a, part->argb, u, 0.0f);
            prim_set_vertex(&tri->v[2], view, in_b, part->argb, u1, 1.0f);
            tri->texture = self->textures[0];
            renderer_add_triangle(g_renderer, tri, self->list_flags);

            count += 2;
            u = u1 >= 1.0f ? 0.0f : u1;
            a = self->arc_degrees + a;
        } while (limit >= a);
    }
}

unsigned char prim_band_process(struct EffectPrim *self)
{
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->speed += self->accel;
    self->angle_speed[1] += self->angle_accel[1];
    self->size[2].speed += self->size[2].accel;
    self->radius_speed[1] += self->radius_accel[1];
    self->radius_speed[0] += self->radius_accel[0];
    self->angle[1] += self->angle_speed[1];
    self->angle_speed[0] += self->angle_accel[0];
    self->angle[0] += self->angle_speed[0];

    mat_set_rotate_x(self->matrix, deg_to_rad((int)self->angle[1]));
    mat_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
    self->velocity = prim_mat_apply(self->matrix, 0.0f, -self->speed, 0.0f);
    self->delta.x += self->velocity.x;
    self->delta.y += self->velocity.y;
    self->delta.z += self->velocity.z;
    self->base.pos.x = self->delta.x + self->master_pos.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;

    self->angle[0] += self->angle_speed[0];
    self->radius[1] += self->radius_speed[1];
    self->radius[0] += self->radius_speed[0];
    self->size[2].value += self->size[2].speed;
    if (self->size[2].value > self->size2_max)
        self->size[2].value = self->size2_max;
    if ((self->pattern & PATTERN_BAND_CLOSE_TOP) && self->frame == self->fade_frame)
        self->radius_speed[1] = -(self->radius[1] / (float)(self->duration - self->frame + 1));

    prim_update_alpha(self);
    return prim_step_motion_counted(self);
}

void prim_band_render(struct EffectPrim *self, const float *view)
{
    struct PrimTriangle *tri = self->triangles;
    float limit = 360.0f - self->arc_degrees;
    float u = 0.0f;
    float a = 0.0f;

    if (!(limit >= 0.0f))
        return;
    do {
        float b = self->arc_degrees + a;
        float c0 = g_cosTable[(int)a], s0 = g_sinTable[(int)a];
        float c1 = g_cosTable[(int)b], s1 = g_sinTable[(int)b];
        float r0 = self->radius[0];
        float r1 = self->radius[1];
        float top = -self->size[2].value;
        Vec3 bottom_a = shape_point(self->matrix, r0 * c0, 0.0f, r0 * s0, &self->base.pos);
        Vec3 bottom_b = shape_point(self->matrix, r0 * c1, 0.0f, r0 * s1, &self->base.pos);
        Vec3 top_a = shape_point(self->matrix, r1 * c0, top, r1 * s0, &self->base.pos);
        Vec3 top_b = shape_point(self->matrix, r1 * c1, top, r1 * s1, &self->base.pos);
        float u1 = u + 0.25f;

        prim_set_vertex(&tri->v[0], view, bottom_a, self->argb, u, 1.0f);
        prim_set_vertex(&tri->v[1], view, top_a, self->argb, u, 0.0f);
        prim_set_vertex(&tri->v[2], view, bottom_b, self->argb, u1, 1.0f);
        tri->texture = self->textures[self->base.motion];
        renderer_add_triangle(g_renderer, tri, 1);
        tri++;

        prim_set_vertex(&tri->v[0], view, bottom_b, self->argb, u1, 1.0f);
        prim_set_vertex(&tri->v[1], view, top_a, self->argb, u, 0.0f);
        prim_set_vertex(&tri->v[2], view, top_b, self->argb, u1, 0.0f);
        tri->texture = self->textures[self->base.motion];
        renderer_add_triangle(g_renderer, tri, 1);
        tri++;

        u = u1 >= 1.0f ? 0.0f : u1;
        a = self->arc_degrees + a;
    } while (limit >= a);
}

void prim_corner_trail_process(struct EffectPrim *self)
{
    Vec3 *corner = self->corners;
    float near_x, near_z, far_x, far_z, y;
    float r;
    int far_angle;

    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    r = self->size[0].value;
    near_x = get_sin_deg((int)self->angle[0]) * r + self->master_pos.x;
    near_z = (float)((double)self->master_pos.z - (double)get_cos_deg((int)self->angle[0]) * r);
    far_angle = (int)(self->angle[0] + self->angle_speed[0]);
    far_x = get_sin_deg(far_angle) * r + self->master_pos.x;
    far_z = (float)((double)self->master_pos.z - (double)get_cos_deg(far_angle) * self->size[0].value);
    y = self->master_pos.y;

    corner[0].x = near_x;
    corner[0].y = y;
    corner[0].z = near_z;
    corner[1].x = far_x;
    corner[1].y = self->size[2].speed + y;
    corner[1].z = far_z;
    corner[2].x = near_x;
    corner[2].y = y - self->scale;
    corner[2].z = near_z;
    corner[3].x = far_x;
    corner[3].y = self->size[2].speed + y - self->scale;
    corner[3].z = far_z;
    prim_update_alpha(self);
    prim_shift_trail(self);
    self->angle[0] += self->angle_speed[0];
    self->delta2.y = self->size[2].speed + self->delta2.y;
}

void prim_pyramid_surge_process(struct EffectPrim *self)
{
    int frame = self->frame;

    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    if (frame < 550) {
        if (frame % 6 == 2) {
            self->speed = self->next_speed;
            self->accel = self->next_accel;
        } else if (frame % 6 == 5) {
            self->speed = 1.18f;
            self->accel = 0.01f;
        }
    } else if (frame == 550) {
        self->speed = self->next_speed;
        self->accel = self->next_accel;
    }
    self->speed = self->accel + self->speed;
    self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->speed);
    self->delta.x += self->velocity.x;
    self->delta.y += self->velocity.y;
    self->delta.z += self->velocity.z;
    self->base.pos.x = self->delta.x + self->master_pos.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;
    prim_update_alpha(self);
}
