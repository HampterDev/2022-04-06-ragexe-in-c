#include "render/rp_pool.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS(deg) g_cosTable[deg]
#define SIN(deg) g_sinTable[deg]

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void facing_square_corners(struct EffectPrim *self, float size, Vec3 corner[4])
{
    int n;

    for (n = 0; n < 4; n++) {
        corner[n].x = COS(45 + 90 * n) * size;
        corner[n].y = 0.0f;
        corner[n].z = SIN(45 + 90 * n) * size;
    }
    prim_quad_turn(&corner[0], &corner[1], &corner[2], &corner[3], self->camera_yaw, self->camera_pitch);
}

static Vec3 vec3(float x, float y, float z)
{
    Vec3 v;

    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

static void light_box_step(struct EffectPrim *self, struct PrimLightBox *lb, int running)
{
    int frame, deg;

    if (lb->active != 1)
        return;
    frame = ++lb->frame;
    if (frame > 29000) {
        lb->frame = 29000;
        frame = 29000;
    } else if (frame <= 0) {
        return;
    }
    if (running) {
        float run = lb->run + 0.5f;
        float rim;

        lb->run = run;
        rim = lb->half_x * 8.0f;
        if (run > rim)
            lb->run = run - rim;
    }
    if (lb->style >= 20.0f && frame > 200 &&
        (self->base.pos.x != self->master->base.pos.x || self->base.pos.z != self->master->base.pos.z) &&
        lb->sink_frame > 2000)
        lb->sink_frame = (short)frame;

    if (frame > lb->sink_frame) {
        if (lb->style >= 20.0f)
            deg = (lb->sink_frame - frame + 18) * 5;
        else if (lb->style >= 10.0f)
            deg = lb->sink_frame - frame + 90;
        else
            deg = (lb->style == 0.0f ? 2505 : 2475) - frame;
        if (deg > 90)
            deg = 90;
        lb->height = SIN(deg < 0 ? 0 : deg) * lb->height_max;
    } else if (frame < 90) {
        lb->height = SIN(frame) * lb->height_max;
    } else {
        lb->height = lb->height_max;
    }
}

void prim_light_box_process(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < 2; i++)
        light_box_step(self, &self->records.light_boxes[i], 0);
    for (i = 2; i < 4; i++)
        light_box_step(self, &self->records.light_boxes[i], 1);
    self->base.pos = self->master->base.pos;
}

static void light_box_corners(struct PrimLightBox *lb)
{
    float hx = lb->half_x, hz = lb->half_z;

    lb->corner[2] = vec3(hx, -1.0f, hz);
    lb->corner[0] = vec3(hx, -1.0f, -hz);
    lb->corner[1] = vec3(-hx, -1.0f, -hz);
    lb->corner[3] = vec3(-hx, -1.0f, hz);
}

static void light_box_wall(struct EffectPrim *self, int *count, const struct PrimLightBox *lb, Vec3 from, Vec3 to,
                           float top, const float *view, unsigned char r, unsigned char g, unsigned char b)
{
    prim_draw_quad(self, count, lb->alpha, from, to, vec3(to.x, top, to.z), vec3(from.x, top, from.z), view,
                   PRIM_BLEND_ADDITIVE, 0, 0, r, g, b, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
}

static const unsigned char light_box_edge[4][2] = { { 2, 0 }, { 0, 1 }, { 1, 3 }, { 3, 2 } };

void prim_light_box_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i;

    for (i = 0; i < 2; i++) {
        struct PrimLightBox *lb = &self->records.light_boxes[i];
        unsigned char r, g, b;
        float top;
        int wall;

        if (lb->active != 1 || lb->alpha <= 0)
            continue;
        if (!(lb->style < 10.0f)) {
            r = 100; g = 100; b = 250;
        } else {
            r = 200; g = 250; b = 200;
        }
        light_box_corners(lb);
        top = -1.0f - lb->height;
        for (wall = 0; wall < 4; wall++)
            light_box_wall(self, &count, lb, lb->corner[light_box_edge[wall][0]], lb->corner[light_box_edge[wall][1]],
                           top, view, r, g, b);
    }

    for (i = 2; i < 4; i++) {
        struct PrimLightBox *lb = &self->records.light_boxes[i];
        unsigned char r, g, b;
        float hx, hz, run, run_len, over = 0.0f, top;
        int edge;

        if (lb->active != 1 || lb->alpha <= 0)
            continue;
        if (!(lb->style < 10.0f)) {
            r = 50; g = 50; b = 250;
        } else {
            r = 150; g = 250; b = 150;
        }
        light_box_corners(lb);
        hx = lb->half_x;
        hz = lb->half_z;
        run = lb->run;
        run_len = lb->run_len;
        if (hx + hx > run) {
            edge = 0;
            lb->corner[2].z = hz - run;
            lb->corner[0].z = lb->corner[2].z - run_len;
            if (-hz > lb->corner[0].z) {
                over = -hz - lb->corner[0].z;
                lb->corner[0].z = -hz;
                lb->corner[1].x = hx - over;
            }
        } else if (hx * 4.0f > run) {
            edge = 1;
            lb->corner[0].x = hx - (run - (hx + hx));
            lb->corner[1].x = lb->corner[0].x - run_len;
            if (-hx > lb->corner[1].x) {
                over = -hx - lb->corner[1].x;
                lb->corner[1].x = -hx;
                lb->corner[3].z = -hz + over;
            }
        } else if (hx * 6.0f > run) {
            edge = 2;
            lb->corner[1].z = run - hx * 4.0f - hz;
            lb->corner[3].z = lb->corner[1].z + run_len;
            if (lb->corner[3].z > hz) {
                over = lb->corner[3].z - hz;
                lb->corner[3].z = hz;
                lb->corner[2].x = -hx + over;
            }
        } else {
            edge = 3;
            lb->corner[3].x = run - hx * 6.0f - hx;
            lb->corner[2].x = lb->corner[3].x + run_len;
            if (lb->corner[2].x > hx) {
                over = lb->corner[2].x - hx;
                lb->corner[2].x = hx;
                lb->corner[0].z = hz - over;
            }
        }
        top = -1.0f - lb->height;
        light_box_wall(self, &count, lb, lb->corner[light_box_edge[edge][0]], lb->corner[light_box_edge[edge][1]], top,
                       view, r, g, b);
        if (over > 0.0f) {
            edge = (edge + 1) & 3;
            light_box_wall(self, &count, lb, lb->corner[light_box_edge[edge][0]], lb->corner[light_box_edge[edge][1]],
                           top, view, r, g, b);
        }
    }
}

static void set_vertex(struct TexDrawVertex *vx, const float *matrix, Vec3 pos, unsigned argb, float u, float v)
{
    struct TexDrawVertex tmp;

    *vx = *prim_project_vertex(&tmp, matrix, pos);
    vx->diffuse = argb;
    vx->u = u;
    vx->v = v;
}

void prim_rainbow_quad(struct EffectPrim *self, int *count, int seg, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                       const float *matrix, unsigned char blend, unsigned char r, unsigned char g, unsigned char b)
{
    const struct RagEffect *m = self->master;
    int dest = blend != 0 ? GFX_BLEND_INVSRCALPHA : GFX_BLEND_ONE;
    int cr = r, cg = g, cb = b;
    unsigned argb;
    float v0, v1;
    struct PrimTriangle *tri;

    if (m->color_r != -1)
        cr = (unsigned char)m->color_r;
    if (m->color_g != -1)
        cg = (unsigned char)m->color_g;
    if (m->color_b != -1)
        cb = (unsigned char)m->color_b;
    if (m->dest_blend != -1)
        dest = m->dest_blend;
    if (*count >= 0x1fe)
        return;

    argb = (unsigned)(int)alpha;
    argb = (argb << 8) + (unsigned char)cr;
    argb = (argb << 8) + (unsigned char)cg;
    argb = (argb << 8) + (unsigned char)cb;
    v0 = (float)seg / 21.0f;
    if (seg == 0)
        v0 = 0.0f;
    v1 = (float)(seg + 1) / 21.0f;

    p0.x = self->base.pos.x + p0.x;
    p0.y = self->base.pos.y + p0.y;
    p0.z = self->base.pos.z + p0.z;
    p1.x = self->base.pos.x + p1.x;
    p1.y = self->base.pos.y + p1.y;
    p1.z = self->base.pos.z + p1.z;
    p2.x = self->base.pos.x + p2.x;
    p2.y = self->base.pos.y + p2.y;
    p2.z = self->base.pos.z + p2.z;
    p3.x = self->base.pos.x + p3.x;
    p3.y = self->base.pos.y + p3.y;
    p3.z = self->base.pos.z + p3.z;

    tri = &self->triangles[*count];
    tri->src_blend = GFX_BLEND_SRCALPHA;
    tri->dest_blend = dest;
    set_vertex(&tri->v[0], matrix, p0, argb, 1.0f, v0);
    set_vertex(&tri->v[1], matrix, p1, argb, 1.0f, v1);
    set_vertex(&tri->v[2], matrix, p2, argb, 0.0f, v1);
    tri->texture = self->textures[0];
    renderer_add_triangle(g_renderer, tri, self->list_flags);
    ++*count;

    tri = &self->triangles[*count];
    tri->src_blend = GFX_BLEND_SRCALPHA;
    tri->dest_blend = dest;
    set_vertex(&tri->v[0], matrix, p3, argb, 0.0f, v0);
    set_vertex(&tri->v[1], matrix, p2, argb, 0.0f, v1);
    set_vertex(&tri->v[2], matrix, p0, argb, 1.0f, v0);
    tri->texture = self->textures[0];
    renderer_add_triangle(g_renderer, tri, self->list_flags);
    ++*count;
}

static const unsigned char rainbow_rgb[7][3] = {
    { 0xff, 0x00, 0x00 }, { 0xff, 0x7e, 0x00 }, { 0xff, 0xff, 0x00 }, { 0x00, 0xff, 0x00 },
    { 0x00, 0x00, 0xff }, { 0x73, 0x32, 0xc8 }, { 0xff, 0x00, 0xff },
};

static Vec3 rainbow_point(const struct PrimRainbow *rb, int deg, double scale)
{
    double across = (double)(COS(deg) * rb->radius);
    double up = scale * (double)(SIN(deg) * rb->radius);
    float out = (float)(scale * across);

    return vec3(out * COS(rb->yaw), (float)(up + up), out * SIN(rb->yaw));
}

void prim_rainbow_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, band;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimRainbow *rb = &self->records.rainbows[i];
        int step;

        if (rb->active != 1 || rb->alpha <= 0)
            continue;
        step = rb->arc / 20;
        for (band = 0; band < 7; band++) {
            double outer = 1.0 - (double)((float)band * 0.05f);
            double inner = 1.0 - (double)((float)(band + 1) * 0.05f);
            int seg = 0;
            int arc_pos;

            for (arc_pos = 0; arc_pos <= rb->arc; arc_pos += step) {
                int near_deg = arc_pos + rb->start;
                int far_deg = near_deg + step;
                Vec3 p0, p1, p2, p3;

                if (near_deg >= 360)
                    near_deg -= 360;
                if (far_deg >= 360)
                    far_deg -= 360;
                p3 = rainbow_point(rb, near_deg, outer);
                p2 = rainbow_point(rb, near_deg, inner);
                p0 = rainbow_point(rb, far_deg, outer);
                p1 = rainbow_point(rb, far_deg, inner);
                prim_rainbow_quad(self, &count, seg, rb->alpha, p0, p1, p2, p3, view, 0,
                                  rainbow_rgb[band][0], rainbow_rgb[band][1], rainbow_rgb[band][2]);
                if (++seg >= 21)
                    seg = 20;
            }
        }
    }
}

static void drift_square_points(struct EffectPrim *self, const struct PrimDriftSquare *sq, Vec3 p[4])
{
    Vec3 corner[4];
    int k;

    facing_square_corners(self, sq->size, corner);
    for (k = 0; k < 4; k++) {
        p[k].x = sq->centre.x + corner[k].x;
        p[k].y = sq->centre.y + corner[k].y;
        p[k].z = sq->centre.z + corner[k].z;
    }
}

void prim_glow_squares_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimDriftSquare *sq = &self->records.drift_squares[i];
        Vec3 p[4];

        if (sq->active != 1)
            continue;
        drift_square_points(self, sq, p);
        prim_draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, 1  , 0, 0, 0xff, 0xff,
                       0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    }
}

static void drift_turn(float *angle, float *target)
{
    float cur = *angle;

    if (*target > cur) {
        cur = cur + 3.0f;
        if (cur > 359.0f)
            cur = 359.0f;
        *angle = cur;
        if (cur > *target) {
            int whole = (int)cur;
            *target = (float)(whole == 0 ? 0 : rand_below(whole));
        }
    } else {
        cur = cur - 2.0f;
        if (0.0f > cur)
            cur = 0.0f;
        *angle = cur;
        if (*target >= cur) {
            int whole = (int)cur;
            *target = (float)(359 - (whole == 0 ? 0 : rand_below(whole)));
        }
    }
}

unsigned char prim_drift_squares_process(struct EffectPrim *self)
{
    short pitch;
    int i;

    self->camera_yaw = (short)(360 - (short)(int)game_camera_heading());
    pitch = (short)(450 - (short)(int)game_camera_pitch());
    self->camera_pitch = pitch;
    if (pitch >= 360)
        self->camera_pitch = (short)(pitch - 360);

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimDriftSquare *sq = &self->records.drift_squares[i];
        float y;
        int frame;

        if (sq->active != 1)
            continue;
        frame = ++sq->frame;
        if (frame <= 0)
            continue;
        sq->alpha = (short)(sq->alpha + (frame >= 30 ? -2 : 5));
        if (sq->alpha < 0)
            sq->alpha = 0;
        drift_turn(&sq->drift_x, &sq->drift_x_to);
        drift_turn(&sq->drift_z, &sq->drift_z_to);
        y = sq->centre.y;
        if (0.0f > y) {
            sq->centre.x = SIN((int)sq->drift_x) * 0.03f + sq->centre.x;
            sq->centre.z = SIN((int)sq->drift_z) * 0.03f + sq->centre.z;
        }
        sq->centre.y = y - sq->fall;
    }
    return 1;
}

void prim_drift_squares_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimDriftSquare *sq = &self->records.drift_squares[i];
        unsigned char r, g, b;
        Vec3 p[4];

        if (sq->active != 1)
            continue;
        drift_square_points(self, sq, p);
        if (!(0.0f >= p[0].y))
            continue;
        if (sq->tint == 2) {
            r = 0x69; g = 0x69; b = 0xe1;
        } else if (sq->tint == 3) {
            r = 0x37; g = 0x37; b = 0x37;
        } else {
            r = 0xff; g = 0xff; b = 0xff;
        }
        prim_draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0, r, g, b,
                       0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    }
}

static Vec3 upright_corner(const struct PrimUprightSquare *sq, int deg)
{
    float across = COS(deg) * sq->size;

    return vec3(COS(sq->yaw) * across + sq->centre.x, SIN(deg) * sq->size + sq->centre.y,
                SIN(sq->yaw) * across + sq->centre.z);
}

void prim_upright_square_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimUprightSquare *sq = &self->records.upright_squares[i];

        if (sq->active != 1 || sq->alpha <= 0)
            continue;
        prim_draw_quad(self, &count, sq->alpha, upright_corner(sq, 90), upright_corner(sq, 180),
                       upright_corner(sq, 270), upright_corner(sq, 0), view, PRIM_BLEND_ADDITIVE, 0, 1, 0x3c, 0x3c,
                       0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    }
}

static const unsigned char glow_spot[4] = { 2, 0, 3, 1 };

static const struct {
    float scale;
    unsigned char r, g, b;
} glow_layer[3][3] = {
    { { 1.0f, 100, 100, 255 }, { 1.2f, 0, 0, 120 }, { 2.0f, 0, 0, 120 } },
    { { 1.0f, 90, 90, 80 }, { 1.2f, 60, 60, 50 }, { 2.0f, 30, 30, 20 } },
    { { 1.0f, 255, 100, 100 }, { 1.2f, 120, 0, 0 }, { 2.0f, 120, 0, 0 } },
};

void prim_orbit_glows_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, layer, q;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSparkGroup *og = &self->records.spark_groups[i];
        int tint;

        if (og->active != 1)
            continue;
        tint = og->tint == 0 ? 0 : og->tint == 1 ? 1 : 2;
        for (layer = 0; layer < 3; layer++) {
            float size = og->size * glow_layer[tint][layer].scale;
            Vec3 corner[4];

            for (q = 0; q < 4; q++) {
                Vec3 *spot = &og->point[glow_spot[q]];
                spot->x = COS((int)og->spark[q].angle) * 4.0f;
                spot->z = SIN((int)og->spark[q].angle) * 4.0f;
            }
            facing_square_corners(self, size, corner);

            for (q = 0; q < 4; q++) {
                const Vec3 *spot = &og->point[glow_spot[q]];
                float tex = og->spark[q].texture;
                int off_yaw = (int)(og->spark[q].angle - (float)self->camera_yaw);
                int front = (off_yaw < 70 && off_yaw > -80) || (unsigned)(off_yaw + 270) > 540;
                int flip = tex == 0.0f || tex == 2.0f;
                int alpha = (int)og->spark[q].alpha;
                Vec3 p[4];
                int k;

                for (k = 0; k < 4; k++) {
                    p[k].x = spot->x + corner[k].x;
                    p[k].y = spot->y + corner[k].y - og->lift;
                    p[k].z = spot->z + corner[k].z;
                }
                if (front != flip) {
                    Vec3 swap = p[0];
                    p[0] = p[1];
                    p[1] = swap;
                    swap = p[2];
                    p[2] = p[3];
                    p[3] = swap;
                }
                prim_draw_quad(self, &count,
                               (short)(alpha <= og->frames ? (unsigned short)alpha : (unsigned short)og->frames),
                               p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, (char)(unsigned char)(int)tex, 0,
                               glow_layer[tint][layer].r, glow_layer[tint][layer].g, glow_layer[tint][layer].b, 0.0f,
                               1.0f, 0.0f, 1.0f, PRIM_QUAD_DEPTH_BIAS);
            }
        }
    }
}

static int heading_of(float nx, float nz)
{
    unsigned short deg;

    for (deg = 0; (short)deg <= 180; deg++)
        if (nx >= COS((short)deg))
            return !(nz >= 0.0f) ? (unsigned short)(360 - deg) : deg;
    return 180;
}

static int tube_across(float dx, float dz)
{
    float len = float_sqrt(dx * dx + dz * dz);
    short deg = (short)(heading_of(dx / len, dz / len) + 90);

    return deg < 360 ? deg : (short)(deg - 360);
}

static float tube_out(const struct PrimArcTube *tube, float arc)
{
    float out = 0.0f;

    if (arc > 0.0f)
        out = arc / 90.0f * tube->reach + 0.0f;
    return out;
}

static int arc_tube_color(short style, int seg, unsigned char rgb[3])
{
    unsigned char grey;

    switch (style) {
    case 0:
        rgb[0] = 0xff; rgb[1] = 0xc8; rgb[2] = (unsigned char)(seg * 30);
        return 1;
    case 1:
        grey = (unsigned char)(seg * 22 + 10);
        rgb[0] = grey; rgb[1] = grey; rgb[2] = 0xff;
        return 1;
    case 2:
        rgb[0] = 0xff; rgb[1] = 0xc8; rgb[2] = (unsigned char)(seg * 50);
        return 1;
    default:
        if (style < 3)
            return 0;
        grey = (unsigned char)(seg * 30 + 0x69);
        rgb[0] = grey; rgb[1] = grey; rgb[2] = grey;
        return 1;
    }
}

void prim_arc_tube_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev0 = { 0.0f, 0.0f, 0.0f }, prev1 = { 0.0f, 0.0f, 0.0f };
    int i, seg;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimArcTube *tube = &self->records.arc_tubes[i];

        if (tube->active != 1 || tube->alpha <= 0 || tube->frame <= 0 || !(tube->points > 0.0f))
            continue;
        for (seg = 0; tube->points > (float)seg; seg++) {
            float arc0 = tube->arc[seg], arc1 = tube->arc[seg + 1];
            float yaw_cos = COS(tube->yaw), yaw_sin = SIN(tube->yaw);
            float out0 = tube_out(tube, arc0), out1;
            float y0 = -(SIN((short)(int)arc0) * tube->reach * tube->height);
            float x0 = yaw_cos * out0, z0 = yaw_sin * out0;
            float x1, y1, z1;
            int across, deg, ring;

            out1 = tube_out(tube, arc1);
            x1 = yaw_cos * out1;
            z1 = yaw_sin * out1;
            y1 = -(SIN((short)(int)arc1) * tube->reach * tube->height);
            across = tube_across(x0 - x1, z0 - z1);
            x0 = x0 + tube->centre.x;
            y0 = tube->centre.y + y0;
            y1 = tube->centre.y + y1;
            z0 = tube->centre.z + z0;
            z1 = tube->centre.z + z1;
            x1 = x1 + tube->centre.x;

            for (ring = 0, deg = 0; deg <= 360; deg += 120, ring++) {
                float out = COS(deg) * tube->radius;
                float up = SIN(deg) * tube->radius - 5.0f;
                float ox = COS(across) * out, oz = SIN(across) * out;
                Vec3 q0 = vec3(x0 + ox, up + y0, z0 + oz);
                Vec3 q1 = vec3(x1 + ox, up + y1, z1 + oz);
                unsigned char rgb[3];
                int alpha = tube->alpha - seg * 10;

                if (ring > 0 && alpha > 0 && arc_tube_color(tube->style, seg, rgb))
                    prim_draw_quad(self, &count, (short)alpha, q0, q1, prev1, prev0, view, PRIM_BLEND_ADDITIVE,
                                   0, 1, rgb[0], rgb[1], rgb[2], 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
                prev0 = q0;
                prev1 = q1;
            }
            if (tube->style < 3)
                continue;
            for (ring = 0, deg = 0; deg <= 360; deg += 120, ring++) {
                float wide = tube->radius * 4.0f;
                float out = COS(deg) * wide;
                float up = SIN(deg) * wide - 5.0f;
                float ox = COS(across) * out, oz = SIN(across) * out;
                Vec3 q0 = vec3(x0 + ox, up + y0, z0 + oz);
                Vec3 q1 = vec3(x1 + ox, up + y1, z1 + oz);

                if (ring > 0) {
                    int alpha = (int)((float)(tube->alpha - seg * 10) * 0.5f);
                    if (alpha > 0 && tube->style == 3)
                        prim_draw_quad(self, &count, (short)alpha, q0, q1, prev1, prev0, view,
                                       PRIM_BLEND_ADDITIVE, 0, 1, 0, 0, 0xff, 0.0f, 1.0f, 0.0f, 1.0f,
                                       PRIM_QUAD_PROJECTED);
                }
                prev0 = q0;
                prev1 = q1;
            }
        }
    }
}

void prim_jagged_tube_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev0 = { 0.0f, 0.0f, 0.0f }, prev1 = { 0.0f, 0.0f, 0.0f };
    float shake_y[21], shake_x[21], shake_z[21];
    int i, seg;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimArcTube *tube = &self->records.arc_tubes[i];

        for (seg = 0; tube->points > (float)seg; seg++) {
            shake_y[seg] = (float)rand_below(7) * 0.1f - 0.3f;
            shake_x[seg] = (float)rand_below(7) * 0.1f - 0.3f;
            shake_z[seg] = (float)rand_below(7) * 0.1f - 0.3f;
        }
        if (tube->active != 1 || tube->alpha <= 0 || tube->frame <= 0)
            continue;
        for (seg = 0; tube->points > (float)seg; seg++) {
            float arc0 = tube->arc[seg], arc1 = tube->arc[seg + 1];
            float yaw_cos = COS(tube->yaw), yaw_sin = SIN(tube->yaw);
            float out0 = tube_out(tube, arc0), out1;
            float x0 = yaw_cos * out0 + shake_x[seg];
            float y0 = shake_y[seg] - 2.0f - SIN((short)(int)arc0) * tube->reach * tube->height;
            float z0 = yaw_sin * out0 + shake_z[seg];
            float x1, y1, z1;
            int across, deg, ring;

            out1 = tube_out(tube, arc1);
            y1 = shake_y[seg + 1] - 2.0f - SIN((short)(int)arc1) * tube->reach * tube->height;
            x1 = yaw_cos * out1 + shake_x[seg + 1];
            z1 = yaw_sin * out1 + shake_z[seg + 1];
            across = tube_across(x0 - x1, z0 - z1);
            x0 = tube->centre.x + x0;
            y0 = y0 + tube->centre.y;
            z0 = z0 + tube->centre.z;
            x1 = tube->centre.x + x1;
            y1 = y1 + tube->centre.y;
            z1 = z1 + tube->centre.z;
            if ((unsigned)(seg - 1) > 17)
                continue;

            for (ring = 0, deg = 0; deg <= 360; deg += 120, ring++) {
                float out = COS(deg) * tube->radius;
                float up = SIN(deg) * tube->radius;
                float ox = COS(across) * out, oz = out * SIN(across);
                Vec3 q0 = vec3(x0 + ox, y0 + up, z0 + oz);
                Vec3 q1 = vec3(x1 + ox, y1 + up, z1 + oz);
                int alpha = tube->alpha - seg * 5;

                if (ring > 0 && alpha > 0) {
                    unsigned char r, g;

                    if (tube->style_jagged == 3) {
                        g = (unsigned char)rand_below(100);
                        r = (unsigned char)((unsigned char)rand_below(50) - 0x38);
                    } else {
                        r = g = (unsigned char)(seg * 6);
                    }
                    prim_draw_quad(self, &count, (short)alpha, q0, q1, prev1, prev0, view, PRIM_BLEND_ADDITIVE,
                                   0, 1, r, g, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
                }
                prev0 = q0;
                prev1 = q1;
            }
        }
    }
}

void prim_rainbow_process(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRainbow *rb = &self->records.rainbows[i];

        if ((signed char)rb->active <= 0)
            continue;
        if (rb->age++ < 180) {
            rb->alpha++;
            rb->start++;
        } else if (rb->age <= 360) {
            rb->alpha--;
            rb->start++;
        }
        if (rb->alpha < 0)
            rb->alpha = 0;
        else if (rb->alpha >= 250)
            rb->alpha = 250;
        if (rb->yaw >= 360)
            rb->yaw -= 360;
    }
}

void prim_glow_squares_process(struct EffectPrim *self)
{
    short pitch;
    int i;

    self->camera_yaw = (short)(360 - (short)(int)game_camera_heading());
    pitch = (short)(450 - (short)(int)game_camera_pitch());
    self->camera_pitch = pitch;
    if (pitch >= 360)
        self->camera_pitch = (short)(pitch - 360);

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimDriftSquare *sq = &self->records.drift_squares[i];
        int frame;

        if (sq->active != 1)
            continue;
        frame = ++sq->frame;
        if (frame <= 0)
            continue;
        if (frame <= 90) {
            int hop = frame * 2 < 360 ? frame * 2 : frame * 2 - 360;

            sq->centre.x = COS(sq->heading) * sq->fall + sq->centre.x;
            sq->centre.z = SIN(sq->heading) * sq->fall + sq->centre.z;
            sq->centre.y = sq->base_y - SIN(hop) * sq->fall * 50.0f;
        }
        if (sq->frame <= 25)
            sq->alpha += 2;
        if (sq->frame > 70)
            sq->alpha--;
        if (sq->alpha < 0)
            sq->alpha = 0;
    }
}

void prim_upright_square_process(struct EffectPrim *self)
{
    struct PrimUprightSquare *sq = &self->records.upright_squares[0];

    if (sq->active != 1)
        return;
    sq->age++;
    if (sq->age < 50)
        sq->size = sq->size + 0.1f;
    if (sq->age <= 5) {
        sq->alpha += 50;
    } else {
        sq->alpha -= 3;
        if (sq->alpha < 0)
            sq->alpha = 0;
    }
}

static void arc_tube_shift(struct PrimArcTube *tube)
{
    int k;

    for (k = (int)tube->points; k >= 1; k--)
        tube->arc[k] = tube->arc[k - 1];
}

void prim_arc_tube_process(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimArcTube *tube = &self->records.arc_tubes[i];

        if (tube->active != 1)
            continue;
        tube->frame++;
        if (tube->frame <= 0)
            continue;
        if (tube->frame > 35)
            tube->alpha -= 5;
        if (tube->alpha < 0)
            tube->alpha = 0;
        tube->arc[0] = tube->arc[0] + 6.0f;
        if (tube->arc[0] > 180.0f)
            tube->arc[0] = 180.0f;
        arc_tube_shift(tube);
        tube->centre = self->master->base.pos;
    }
}

void prim_jagged_tube_process(struct EffectPrim *self)
{
    int i, step;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimArcTube *tube = &self->records.arc_tubes[i];

        if (tube->active != 1)
            continue;
        tube->frame++;
        if (tube->frame <= 0)
            continue;
        tube->reach = tube->reach + 0.4f;
        if (tube->reach > 15.0f)
            tube->alpha -= 2;
        if (tube->frame <= 25)
            tube->alpha = (short)(tube->alpha + (short)(rand_below(4) + 5));
        if (tube->alpha < 0)
            tube->alpha = 0;
        if (tube->frame != 1)
            continue;
        for (step = 21; step > 0; step--) {
            tube->arc[0] = tube->arc[0] + 9.0f;
            if (tube->arc[0] > 180.0f)
                tube->arc[0] = 180.0f;
            else
                arc_tube_shift(tube);
        }
    }
}
