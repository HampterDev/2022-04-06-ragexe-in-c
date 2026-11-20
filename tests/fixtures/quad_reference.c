/* The CPU code the quads' instances replaced, as the client had it at a005c851 (actor/sprite_quad.c,
   effects/particle.c, effects/sprite_particle.c, render/gfx_gpu.c), for tests/test_quad_instances.c
   to compare the GPU with: the vertices they made instead of queueing them. Compiled as the client
   is: each float operation rounded once, in the order written (MSVC's /fp:precise contracts
   nothing on x64; GCC and Clang need -ffp-contract=off for that, which the CMake builds pass). */
#include "quad_reference.h"
#include "actor/sprite_render.h"
#include "render/sprite_atlas.h"
#include "render/projection.h"
#include "render/math3d.h"
#include "render/renderer.h"
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "game/context.h"
#include "render/texture_types.h"
#include "resources/attr.h"
#include "world/ground.h"
#include "world/ground_api.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void reference_sprite_vertices(const struct SqQuad *quad, const struct SpriteRegion *region, int palette, float x,
                               float y, struct TexDrawVertex *v)
{
    struct SpriteRegion uv = *region;
    float z_a, z_b, last_y;

    z_a = renderer_project_depth(quad->depth_a);
    z_b = renderer_project_depth(quad->depth_b);
    if (quad->angle != 0.0f) {
        float c = get_cos_deg((int)quad->angle);
        float s = get_sin_deg((int)quad->angle);
        float hy = (quad->bottom - quad->top) * 0.5f;
        float hx = (quad->x1 - quad->x0) * 0.5f;
        float cy = quad->top + hy + y;
        float cx = quad->x0 + hx + x;
        float hy_s = hy * s, hx_c = hx * c, hx_s = hx * s, hy_c = hy * c;

        v[0].x = -hx_c - -hy_s + cx + 0.5f;
        v[1].y = -hy_c + hx_s + cy + 0.5f;
        v[0].y = -hy_c + -hx_s + cy + 0.5f;
        v[2].x = -hx_c - hy_s + cx + 0.5f;
        v[1].x = hx_c - -hy_s + cx + 0.5f;
        v[3].x = hx_c - hy_s + cx + 0.5f;
        v[2].y = hy_c + -hx_s + cy + 0.5f;
        last_y = hy_c + hx_s + cy;
    } else {
        v[0].x = quad->x0 + 0.5f;
        v[0].y = quad->top + 0.5f;
        v[1].x = quad->x1 + 0.5f;
        v[1].y = quad->top + 0.5f;
        v[2].x = quad->x0 + 0.5f;
        v[2].y = quad->bottom + 0.5f;
        v[3].x = quad->x1 + 0.5f;
        last_y = quad->bottom;
    }
    v[3].y = last_y + 0.5f;
    if (palette == 0xd77b)
        uv.u1 = uv.u1 - 0.002f;
    v[0].rhw = quad->depth_b;
    v[0].z = z_b;
    v[0].u = uv.u;
    v[0].v = uv.v;
    v[0].diffuse = quad->color;
    v[1].rhw = quad->depth_b;
    v[1].z = z_b;
    v[1].u = uv.u1;
    v[1].v = uv.v;
    v[1].diffuse = quad->color;
    v[2].rhw = quad->depth_b;
    v[2].z = z_a;
    v[2].u = uv.u;
    v[2].v = uv.v1;
    v[2].diffuse = quad->color;
    v[3].rhw = quad->depth_b;
    v[3].z = z_a;
    v[3].u = uv.u1;
    v[3].v = uv.v1;
    v[3].diffuse = quad->color;
}

#define MATRIX_BYTES (12 * sizeof(float))

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

int reference_particle_vertices(struct Particle *self, unsigned char facing_fixed, float v_top, float v_bottom,
                                float u_left, float u_right, unsigned char with_depth, struct TexDrawVertex *out)
{
    float view[12], world[12], to_screen[12];
    struct TexDrawVertex *v;
    int i, has_corner = 0;

    for (i = 0; i < 4; i++)
        if (self->corners[i].x != 0 || self->corners[i].y != 0 || self->corners[i].z != 0)
            has_corner = 1;
    if (!has_corner || !game_copy_view_matrix(view))
        return 0;

    particle_quad_paint(self, self->corners, v_top, v_bottom, u_left, u_right);
    memcpy(out, self->corners, sizeof(self->corners));

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

    for (i = 0, v = out; i < 4; i++, v++) {
        Vec3 corner;
        corner.x = v->x;
        corner.y = v->y;
        corner.z = v->z;
        renderer_project_vertex(g_renderer, &corner, to_screen, v);
        v->rhw += self->depth_bias;
        v->z = with_depth ? renderer_project_depth(v->rhw) : 0.0f;
    }
    return 1;
}

#define CLIP_MIRRORED 1
#define SPR_PALETTE(spr) (((struct SprRes *)(spr))->palette)

static float degrees_to_radians(float deg)
{
    return deg / g_halfTurnDegrees * g_pi;
}

static void transform(Vec3 *out, const float *m, float x, float y, float z)
{
    out->x = y * m[3] + x * m[0] + z * m[6] + m[9];
    out->y = y * m[4] + x * m[1] + z * m[7] + m[10];
    out->z = y * m[5] + x * m[2] + z * m[8] + m[11];
}

int reference_sprite_particle_vertices(struct SpriteParticle *self, struct TexDrawVertex (*out)[4], int most)
{
    float view[12];
    float screen_x, screen_y, rhw, pixel_scale;
    Vec3 pos;
    struct Motion *motion;
    int i, drawn = 0;

    if (!self->spr || !self->act || self->loop != SPRITE_STILL)
        return 0;

    if (!game_copy_view_matrix(view))
        return 0;
    pos = self->base.pos;
    renderer_project_point(g_renderer, &pos, view, &screen_x, &screen_y, &rhw);
    pixel_scale = renderer_screen_scale(g_renderer, rhw);
    motion = act_res_get_motion(self->act, self->action, self->motion);

    for (i = 0; i < motion->clip_count && drawn < most; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;
        struct Texture *tex;
        struct SpriteRegion rect;
        struct TexDrawVertex *verts;
        float u_left, v_top, u_right, v_bottom;
        float half_w, half_h, unit_x, unit_y, depth;
        float clip_scale[12], clip_turn[12], clip_move[12], clip_scale_turn[12], clip_m[12];
        float own_scale[12], own_turn[12], own_move[12], own_scale_turn[12], own_placed[12], world[12];
        Vec3 corner[4];
        int k;

        if (clip->sprIndex == -1)
            continue;
        img = sprite_resource_image(self->spr, clip->clipType, clip->sprIndex);
        if (!img) continue;
        memset(&rect, 0, sizeof(rect));
        tex = img->texture;
        if (!tex) {
            tex = sprite_atlas_resolve(g_renderer, img, SPR_PALETTE(self->spr), &rect);
            u_left = rect.u;
            v_top = rect.v;
            u_right = rect.u1;
            v_bottom = rect.v1;
        } else {
            u_left = 0;
            v_top = 0;
            u_right = (float)(unsigned)img->texture->reqWidth / (float)(unsigned)img->texture->base.width;
            v_bottom = (float)(unsigned)img->texture->reqHeight / (float)(unsigned)img->texture->base.height;
        }
        if (!tex) continue;
        if (clip->flags & CLIP_MIRRORED)
            particle_quad_paint(&self->base, self->base.corners, v_bottom, v_top, u_right, u_left);
        else
            particle_quad_paint(&self->base, self->base.corners, v_bottom, v_top, u_left, u_right);

        verts = out[drawn++];
        memcpy(verts, self->base.corners, sizeof(self->base.corners));

        if (self->base.flags & PARTICLE_TINT_BY_CLIP) {
            const float to_unit = 0.003921569f;
            unsigned alpha = (unsigned char)(int)((float)self->base.alpha * to_unit * ((float)clip->color[3] * to_unit) * 255.0f);
            unsigned color = (((alpha << 8 | clip->color[0]) << 8) | clip->color[1]) << 8 | clip->color[2];
            for (k = 0; k < 4; k++)
                verts[k].diffuse = color;
        }

        half_w = (float)img->width * 0.5f;
        half_h = (float)img->height * 0.5f;
        unit_x = clip->zoomX * pixel_scale / (g_renderer->proj[0] * rhw);
        unit_y = clip->zoomY * pixel_scale / (g_renderer->proj[1] * rhw);
        mat_set_scale(clip_scale, unit_x * half_w, unit_y * half_h, 1.0f);
        mat_set_rotate_z(clip_turn, degrees_to_radians((float)clip->angle));
        mat_set_translation(clip_move, ((float)clip->x + half_w) * unit_x, ((float)clip->y + half_h) * unit_y, 0);
        mat_multiply(clip_scale_turn, clip_scale, clip_turn);
        mat_multiply(clip_m, clip_scale_turn, clip_move);
        for (k = 0; k < 4; k++)
            transform(&corner[k], clip_m, verts[k].x, verts[k].y, verts[k].z);

        mat_set_scale(own_scale, self->base.scale, self->base.scale, 1.0f);
        mat_set_rotate_z(own_turn, degrees_to_radians(self->base.roll));
        mat_rotate_x(own_turn, degrees_to_radians(game_camera_pitch()));
        mat_rotate_y(own_turn, degrees_to_radians(game_camera_heading()));
        pos = self->base.pos;
        mat_set_translation(own_move, pos.x, pos.y, pos.z);
        mat_multiply(own_scale_turn, own_scale, own_turn);
        mat_multiply(own_placed, own_scale_turn, own_move);
        mat_multiply(world, own_placed, self->base.matrix);

        for (k = 0; k < 4; k++) {
            Vec3 placed;
            transform(&placed, world, corner[k].x, corner[k].y, corner[k].z);
            renderer_project_vertex(g_renderer, &placed, view, &verts[k]);
        }
        depth = (float)i * 1e-5f + self->base.depth_bias;
        if (depth != 0) {
            for (k = 0; k < 4; k++) {
                verts[k].rhw += depth;
                verts[k].z = renderer_project_depth(verts[k].rhw);
            }
        }
    }
    return drawn;
}

/* effect_prim_draw.c's prim_project_vertex and set_vertex */
static void prim_vertex(struct TexDrawVertex *vx, const float *matrix, const float *point, unsigned argb, float u,
                        float v)
{
    struct TexDrawVertex projected;
    Vec3 pos;

    pos.x = point[0];
    pos.y = point[1];
    pos.z = point[2];
    memset(&projected, 0, sizeof projected);
    renderer_project_vertex(g_renderer, &pos, matrix, &projected);
    *vx = projected;
    vx->diffuse = argb;
    vx->u = u;
    vx->v = v;
}

void reference_prim_triangle(const float (*p)[3], const float *u, const float *v, unsigned argb, const float *view,
                             unsigned char mode, const float *centrePoint, float scale, int second,
                             struct TexDrawVertex *out)
{
    int i;

    for (i = 0; i < 3; i++)
        prim_vertex(&out[i], view, p[i], argb, u[i], v[i]);
    /* quad_depth */
    if (mode == 2) {
        for (i = 0; i < 3; i++) {
            out[i].rhw = out[i].rhw + 4.5e-5f;
            out[i].z = renderer_project_depth(out[i].rhw);
        }
        return;
    }
    if (mode == 3) {
        struct TexDrawVertex centre;
        float half_width;

        prim_vertex(&centre, view, centrePoint, 0, 0.0f, 0.0f);
        half_width = (float)fabs((double)(out[0].x - out[1].x)) * 0.5f;
        out[0].x = centre.x + half_width;
        out[1].x = centre.x - half_width;
        out[2].x = second ? centre.x + half_width : centre.x - half_width;
        for (i = 0; i < 3; i++)
            out[i].y = (float)(-30 * g_renderer->height / 480) * scale + out[i].y;
        mode = 1;
    }
    if (mode == 1) {
        for (i = 0; i < 3; i++) {
            out[i].z = 1e-9f;
            out[i].rhw = 1.0f;
        }
    }
}

int reference_attr_quad(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour,
                        struct Texture *texture, const float *uv, float bias, struct TexDrawVertex *out)
{
    float quad[12] = {0};
    float su, sv;
    int i;

    if (!c3d_ground15_get_attr_quad(self, ax, ay, quad))
        return 0;

    for (i = 0; i < 4; i++) {
        float *p = &quad[i * 3];
        float x = p[0], z;

        if (ax & 1) {
            if (quad[3] > x)
                p[0] = (quad[3] - x) * 0.5f + x;
        } else {
            if (x > quad[0])
                p[0] = x - (x - quad[0]) * 0.5f;
        }
        z = p[2];
        if (ay & 1) {
            if (quad[8] > z)
                p[2] = (quad[8] - z) * 0.5f + z;
        } else {
            if (z > quad[2])
                p[2] = z - (z - quad[2]) * 0.5f;
        }
    }

    for (i = 0; i < 4; i++) {
        {
            Vec3 world_point = {quad[i * 3], quad[i * 3 + 1], quad[i * 3 + 2]};
            memset(&out[i], 0, sizeof out[i]);
            renderer_project_vertex(g_renderer, &world_point, view, &out[i]);
        }
        out[i].z -= bias;
    }
    su = (float)(DWORD)texture->reqWidth / (float)(DWORD)texture->base.width;
    sv = (float)(DWORD)texture->reqHeight / (float)(DWORD)texture->base.height;
    out[0].u = uv[0] * su;
    out[0].v = uv[1] * sv;
    out[1].u = uv[2] * su;
    out[1].v = uv[3] * sv;
    out[2].u = uv[4] * su;
    out[2].v = uv[5] * sv;
    out[3].u = uv[6] * su;
    out[3].v = uv[7] * sv;
    for (i = 0; i < 4; i++)
        out[i].diffuse = colour;
    return 1;
}

int reference_attr_marker(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour,
                          struct TexDrawVertex *out)
{
    int cx = (int)((float)ax * 0.5f);
    int cy = (int)((float)ay * 0.5f);
    int ayOdd = ay % 2;
    int width = self->width;
    int halfW = (int)((float)width * 0.5f);
    int halfH = (int)((float)self->height * 0.5f);
    float half = self->zoom * 0.5f;
    float quarter = half * 0.5f;
    const struct AttrCell *c;
    float quad[12], offX, offZ, x0, z0, left, right, front, back;
    int i;

    if (ax < 0 || cx >= width || ay < 0 || cy >= self->height)
        return 0;
    c = c3d_attr_get_cell(self->attr, ax, ay);
    if (!c)
        return 0;
    offX = ax % 2 == 1 ? half : 0.0f;
    offZ = 0.0f;
    x0 = (float)(cx - halfW) * self->zoom + offX;
    if (ayOdd == 1)
        offZ = half;
    z0 = (float)(cy - halfH) * self->zoom + offZ;
    left = x0 - quarter;
    right = x0 + half + quarter;
    front = z0 - quarter;
    back = z0 + half + quarter;
    quad[0] = left;
    quad[1] = c->height[0];
    quad[2] = front;
    quad[3] = right;
    quad[4] = c->height[1];
    quad[5] = front;
    quad[6] = left;
    quad[7] = c->height[2];
    quad[8] = back;
    quad[9] = right;
    quad[10] = c->height[3];
    quad[11] = back;

    for (i = 0; i < 4; i++)
        {
            Vec3 world_point = {quad[i * 3], quad[i * 3 + 1], quad[i * 3 + 2]};
            memset(&out[i], 0, sizeof out[i]);
            renderer_project_vertex(g_renderer, &world_point, view, &out[i]);
        }
    out[0].u = 0.0f;
    out[0].v = 0.0f;
    out[1].u = 1.0f;
    out[1].v = 0.0f;
    out[2].u = 0.0f;
    out[2].v = 1.0f;
    out[3].u = 1.0f;
    out[3].v = 1.0f;
    for (i = 0; i < 4; i++)
        out[i].diffuse = colour;
    return 1;
}

void reference_widen(const struct TexDrawVertex *in, struct ReferenceVertex *out)
{
    out->x = in->x;
    out->y = in->y;
    out->z = in->z;
    out->rhw = in->rhw;
    memcpy(out->bytes, &in->diffuse, 4);
    out->u0 = out->u1 = in->u;
    out->v0 = out->v1 = in->v;
}

#define QUAD_STRIDE 36

static float pixel_boundary(float v)
{
    double snapped = floor((double)v * 256.0 + 0.5) / 256.0;
    return (float)(ceil(snapped) - 0.5);
}

static float read_float(const unsigned char *p)
{
    float f;
    memcpy(&f, p, 4);
    return f;
}

#define QUAD_VALUES 9

void reference_snap_flat(struct ReferenceVertex *vertices, int nverts, const unsigned short *indices, unsigned count)
{
    unsigned char *verts = (unsigned char *)vertices, *orig, *done;
    unsigned t;
    size_t bytes = (size_t)nverts * QUAD_STRIDE;
    orig = malloc(bytes + (size_t)nverts);
    if (!orig)
        abort();
    done = orig + bytes;
    memcpy(orig, verts, bytes);
    memset(done, 0, (size_t)nverts);
    for (t = 0; t + 3 <= count; t += 3) {
        int k[3], i, j;
        float px[3], py[3], value[3][QUAD_VALUES], dx1, dy1, dx2, dy2, det;
        for (i = 0; i < 3; i++) {
            k[i] = indices ? indices[t + i] : (int)(t + i);
            if (k[i] >= nverts)
                break;
        }
        if (i < 3 || (done[k[0]] && done[k[1]] && done[k[2]]))
            continue;
        for (i = 0; i < 3; i++) {
            const unsigned char *v = orig + (size_t)k[i] * QUAD_STRIDE;
            px[i] = read_float(v);
            py[i] = read_float(v + 4);
            value[i][0] = read_float(v + 8);
            for (j = 0; j < 4; j++)
                value[i][1 + j] = v[16 + j];
            for (j = 0; j < 4; j++)
                value[i][5 + j] = read_float(v + 20 + j * 4);
        }
        dx1 = px[1] - px[0];
        dy1 = py[1] - py[0];
        dx2 = px[2] - px[0];
        dy2 = py[2] - py[0];
        det = dx1 * dy2 - dx2 * dy1;
        if (det > -1e-6f && det < 1e-6f)
            continue;
        for (i = 0; i < 3; i++) {
            unsigned char *out = verts + (size_t)k[i] * QUAD_STRIDE;
            float sx = pixel_boundary(px[i]), sy = pixel_boundary(py[i]), ox, oy;
            if (done[k[i]])
                continue;
            done[k[i]] = 1;
            if (sx == px[i] && sy == py[i])
                continue;
            ox = sx - px[0];
            oy = sy - py[0];
            memcpy(out, &sx, 4);
            memcpy(out + 4, &sy, 4);
            for (j = 0; j < QUAD_VALUES; j++) {
                float d1 = value[1][j] - value[0][j], d2 = value[2][j] - value[0][j];
                float gx = (d1 * dy2 - d2 * dy1) / det, gy = (d2 * dx1 - d1 * dx2) / det;
                float a = value[0][j] + gx * ox + gy * oy;
                if (j == 0) {
                    memcpy(out + 8, &a, 4);
                } else if (j < 5) {
                    a = floorf(a + 0.5f);
                    out[16 + j - 1] = (unsigned char)(a < 0.0f ? 0.0f : a > 255.0f ? 255.0f : a);
                } else {
                    memcpy(out + 20 + (j - 5) * 4, &a, 4);
                }
            }
        }
    }
    free(orig);
}
