#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"

#define BLEND_ALPHA 1

void prim_face_camera(struct EffectPrim *self)
{
    short pitch;

    self->camera_yaw = (short)(360 - (short)(int)game_camera_heading());
    pitch = (short)(450 - (short)(int)game_camera_pitch());
    self->camera_pitch = pitch;
    if (pitch >= 360)
        self->camera_pitch = (short)(pitch - 360);
}

static int quarter_turn(int deg)
{
    return deg < 270 ? deg + 90 : deg - 270;
}

void prim_spiral_column_process(struct EffectPrim *self)
{
    struct PrimSpiralColumn *col = self->records.spiral_columns;
    int i;

    prim_face_camera(self);
    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, col++) {
        int frame;

        if (col->active != 1)
            continue;
        frame = ++col->frame;
        if (frame <= 0)
            continue;
        col->angle -= 10;
        if (col->angle < 0)
            col->angle += 360;
        if (frame == 1) {
            rand();
        } else if (frame <= 10) {
            col->alpha += 10;
        } else if (frame > 100) {
            col->alpha += -1;
            if (col->alpha < 0) {
                col->alpha = 0;
                col->active = 0;
            }
        }
    }
}

void prim_spiral_column_draw(struct EffectPrim *self, const float *view, int *count,
                                        float scale, short index, short blend, unsigned char r, unsigned char g,
                                        unsigned char b)
{
    struct PrimSpiralColumn *col = &self->records.spiral_columns[index];
    short yaw = self->camera_yaw;
    short pitch = self->camera_pitch;
    float shimmer = col->shimmer, size = col->size, square_size;
    Vec3 corner[4];
    int deg, k, level;

    if (shimmer >= 360.0f || 0.0f > shimmer) {
        col->shimmer = 0.0f;
        shimmer = 0.0f;
    }
    square_size = g_sinTable[(int)shimmer] * size * 0.05f + size * scale;
    deg = col->angle;
    for (k = 0; k < 4; k++) {
        corner[k].x = g_cosTable[deg] * square_size;
        corner[k].y = 0.0f;
        corner[k].z = g_sinTable[deg] * square_size;
        deg = quarter_turn(deg);
    }
    prim_quad_turn(&corner[0], &corner[1], &corner[2], &corner[3], yaw, pitch);
    for (k = 0; k < 4; k++) {
        corner[k].x = col->pos.x + corner[k].x;
        corner[k].y = col->pos.y + corner[k].y;
        corner[k].z = col->pos.z + corner[k].z;
    }

    for (level = 0; level < 36; level++) {
        short swing = (short)(level * 45 + (short)col->frame);
        int swing_deg = swing % 360;
        float dx = g_cosTable[swing_deg] * 10.0f, dz = g_sinTable[swing_deg] * 10.0f;
        float up = (float)(level * 2);
        Vec3 p[4];

        for (k = 0; k < 4; k++) {
            p[k].x = dx + corner[k].x;
            p[k].y = corner[k].y - up;
            p[k].z = dz + corner[k].z;
        }
        prim_draw_quad(self, count, col->alpha, p[0], p[1], p[2], p[3], view, blend, col->texture, 1, r, g, b,
                       0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
    }
}

void prim_spiral_column_render(struct EffectPrim *self, const float *view)
{
    struct PrimSpiralColumn *col = self->records.spiral_columns;
    int count = 0;
    short i;

    for (i = 0; i < 4; i++, col++) {
        if (col->active != 1 || col->alpha <= 0)
            continue;
        prim_spiral_column_draw(self, view, &count, 1.0f, i, BLEND_ALPHA, 0x37, 0x37, 0xff);
        prim_spiral_column_draw(self, view, &count, 2.0f, i, PRIM_BLEND_ADDITIVE, 0x19, 0x19, 0xff);
    }
}

void prim_swing_arc_process(struct EffectPrim *self)
{
    struct PrimSwingArc *arc = self->records.swing_arcs;
    int i, step, k;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, arc++) {
        int frame;

        if (arc->active != 1)
            continue;
        arc->frame++;
        arc->pos = self->base.pos;
        arc->pos.y = self->base.pos.y - 10.0f;
        frame = arc->frame;
        if (frame <= 0)
            continue;

        if (arc->clockwise == 1) {
            if (frame > i * 10)
                arc->heading += 2;
            if (arc->heading >= 360)
                arc->heading -= 360;
        } else {
            if (frame > i * 10)
                arc->heading += -2;
            if (arc->heading < 0)
                arc->heading += 360;
        }
        if (frame > 60) {
            arc->alpha += -6;
            if (arc->alpha < 0)
                arc->alpha = 0;
        }
        if (arc->alpha < 0)
            arc->alpha = 0;

        if (frame == 1) {
            for (step = 0; step < 21; step++) {
                float tip = arc->points[0] + 9.0f;
                arc->points[0] = tip;
                if (tip > 180.0f) {
                    arc->points[0] = 180.0f;
                    continue;
                }
                for (k = (int)arc->count; k >= 1; k--) {
                    arc->points[k] = arc->points[k - 1];
                    arc->marks[k] = arc->marks[k - 1];
                }
            }
        }
    }
}

static float swing_arc_jitter(void)
{
    int r = rand() % 7;

    return (float)(r < 0 ? 0 : r) * 0.1f - 0.3f;
}

static float swing_arc_reach(float deg, float size)
{
    if (deg > 0.0f)
        return deg / 90.0f * size + 0.0f;
    return 0.0f;
}

void prim_swing_arc_render(struct EffectPrim *self, const float *view)
{
    struct PrimSwingArc *arc = self->records.swing_arcs;
    float jitter[64] = {0};
    float *jx = jitter, *jz = jitter + 21, *jy = jitter + 42;
    Vec3 first_a[20], first_b[20];
    Vec3 prev_a = {0.0f, 0.0f, 0.0f}, prev_b = {0.0f, 0.0f, 0.0f};
    Vec3 qa = {0.0f, 0.0f, 0.0f}, qb = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i, j, k;

    memset(first_a, 0, sizeof(first_a));
    memset(first_b, 0, sizeof(first_b));

    for (i = 0; i < 4; i++, arc++) {
        if (arc->count > 0.0f) {
            k = 0;
            do {
                jy[k] = swing_arc_jitter();
                jx[k] = swing_arc_jitter();
                jz[k] = swing_arc_jitter();
                k++;
            } while (arc->count > (float)k);
        }
        if (arc->active != 1 || arc->alpha <= 0 || arc->frame <= 0 || !(arc->count > 0.0f))
            continue;

        for (j = 0; arc->count > (float)j; j++) {
            float deg0 = arc->points[j], deg1 = arc->points[j + 1];
            float size = arc->size, height = arc->height_scale;
            float head_cos = g_cosTable[arc->heading], head_sin = g_sinTable[arc->heading];
            float reach0 = swing_arc_reach(deg0, size), reach1;
            float x0, y0, z0, x1, y1, z1, sin1;
            unsigned short heading;
            short perp;

            y0 = jy[j] - 2.0f;
            z0 = head_sin * reach0 + jz[j];
            x0 = head_cos * reach0 + jx[j];
            y0 = y0 - g_sinTable[(short)(int)deg0] * size * height;
            reach1 = swing_arc_reach(deg1, size);
            z1 = head_sin * reach1 + jz[j + 1];
            x1 = head_cos * reach1 + jx[j + 1];
            sin1 = g_sinTable[(short)(int)deg1];
            heading = (unsigned short)(int)deg1;
            prim_heading_degrees(x0 - x1, z0 - z1, (short *)&heading);

            x0 = x0 + arc->pos.x;
            x1 = x1 + arc->pos.x;
            y1 = jy[j + 1] - 2.0f - size * sin1 * height;
            y0 = y0 + arc->pos.y;
            z0 = z0 + arc->pos.z;
            z1 = z1 + arc->pos.z;
            y1 = y1 + arc->pos.y;
            perp = (short)(heading + 90);
            if (perp >= 360)
                perp = (short)(perp - 360);

            if ((unsigned)(j - 1) > 17)
                continue;
            for (k = 0; k < 4; k++) {
                float width = arc->width;
                float across = g_cosTable[k * 120] * width, up = g_sinTable[k * 120] * width;
                float dx = g_cosTable[perp] * across, dz = g_sinTable[perp] * across;

                qa.x = x0 + dx;
                qa.y = y0 + up;
                qa.z = z0 + dz;
                qb.x = x1 + dx;
                qb.y = y1 + up;
                qb.z = z1 + dz;
                if (k > 0) {
                    int alpha = arc->alpha - 5 * j;
                    if (alpha > 0 && i == 0) {
                        unsigned char g, b;
                        if (self->records.swing_arcs[0].style == 3) {
                            int roll_b = rand() % 26;
                            int roll_g = rand() % 51;
                            b = (unsigned char)((roll_b >= 0 ? (unsigned char)roll_b : 0) + 100);
                            g = roll_g >= 0 ? (unsigned char)roll_g : 0;
                        } else {
                            g = b = (unsigned char)(j * 3);
                        }
                        prim_draw_quad(self, &count, (short)alpha, qa, qb, prev_b, prev_a, view, PRIM_BLEND_ADDITIVE,
                                       0, 1, 0xff, g, b, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
                    }
                }
                prev_a = qa;
                prev_b = qb;
            }
            if (i > 0) {
                int alpha = arc->alpha;
                if (i == 1)
                    alpha /= 3;
                else if (i == 2)
                    alpha /= 2;
                prim_draw_quad(self, &count, (short)alpha, qa, qb, first_b[j], first_a[j], view, PRIM_BLEND_ADDITIVE,
                               0, 1, 0xff, 0, 0, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
            } else {
                first_a[j] = qa;
                first_b[j] = qb;
            }
        }
    }
}

void prim_crown_process(struct EffectPrim *self)
{
    struct PrimCrownRing *crown = self->records.crown_rings;
    int i, k;

    for (i = 0; i < 4; i++, crown++) {
        short start;
        int frame;

        if (crown->active != 1)
            continue;
        start = (short)(crown->arc_start + 1);
        frame = ++crown->frame;
        crown->arc_start = start;
        if (start >= 360)
            crown->arc_start = (short)(start - 360);
        if (frame >= 125) {
            crown->alpha += -15;
            if (crown->alpha < 0)
                crown->alpha = 0;
            crown->base_height = crown->base_height - 5.0f;
            if (0.0f > crown->base_height)
                crown->base_height = 0.0f;
        }
        for (k = 0; k < CROWN_RING_POINTS; k++) {
            float h;

            frame = crown->frame;
            if (frame < 100)
                h = 0.0f;
            else if (frame <= 115)
                h = g_sinTable[(frame - 100) * 6] * crown->base_height;
            else
                h = crown->base_height;
            crown->heights[k] = h;
            if (0.0f > h)
                crown->heights[k] = 0.0f;
        }
    }
}

static void crown_points(const struct PrimCrownRing *crown, int deg, int point, float out_x, float out_z, Vec3 *low,
                         Vec3 *high)
{
    float height = crown->heights[point];
    float lean_out = g_cosTable[crown->tilt] * height;
    float y;

    low->x = out_x + crown->pos.x + crown->offset.x;
    low->y = crown->pos.y + 0.0f + crown->offset.y;
    low->z = out_z + crown->pos.z + crown->offset.z;
    high->x = lean_out * g_cosTable[deg] + out_x + crown->pos.x + crown->offset.x;
    y = crown->pos.y - g_sinTable[crown->tilt] * height;
    high->y = y + crown->offset.y;
    high->z = lean_out * g_sinTable[deg] + out_z + crown->pos.z + crown->offset.z;
}

void prim_crown_render(struct EffectPrim *self, const float *view)
{
    struct PrimCrownRing *crown = self->records.crown_rings;
    Vec3 low = {0.0f, 0.0f, 0.0f}, high = {0.0f, 0.0f, 0.0f};
    Vec3 prev_low = {0.0f, 0.0f, 0.0f}, prev_high = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, crown++) {
        int step, along, point;

        if (crown->active != 1)
            continue;
        step = crown->arc / 4;
        if (crown->arc >= 0) {
            point = 0;
            for (along = 0; along <= crown->arc; along += step) {
                int deg = (crown->arc_start + along) % 360;
                float radius;

                if ((unsigned short)crown->arc == 360 && point == 4)
                    deg = crown->arc_start;
                radius = crown->radius;
                crown_points(crown, deg, point, radius * g_cosTable[deg], radius * g_sinTable[deg], &low, &high);
                if (point > 0)
                    prim_draw_quad(self, &count, (short)(crown->alpha / 5), prev_low, low, prev_high, high, view,
                                   BLEND_ALPHA, 0, 0, 0xff, 0x0a, 0x0a, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
                if (++point > 4)
                    point = 4;
                prev_low = low;
                prev_high = high;
            }
        }
        if (0 > crown->arc)
            continue;
        point = 0;
        for (along = 0; along <= crown->arc; along += step) {
            int deg = (crown->arc_start + 45 + along) % 360;
            float radius;

            if ((unsigned short)crown->arc == 360 && point == 4)
                deg = crown->arc_start;
            radius = crown->radius;
            crown_points(crown, deg, point, radius * g_cosTable[deg] * 1.5f, radius * g_sinTable[deg] * 1.5f, &low,
                         &high);
            if (point > 0)
                prim_draw_strip(self, &count, point - 1, crown->alpha, prev_low,
                                low, high, prev_high, view, 0);
            if (++point > 4)
                point = 4;
            prev_low = low;
            prev_high = high;
        }
    }
}

void prim_stretch_ray_process(struct EffectPrim *self)
{
    struct PrimStretchRay *ray = self->records.stretch_rays;
    int i;

    prim_face_camera(self);
    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, ray++) {
        if (ray->active != 1)
            continue;
        ray->frame++;
        if (ray->frame <= 0)
            continue;
        ray->stretch += 5;
        if (ray->stretch > 100) {
            ray->alpha += -2;
            ray->stretch = 100;
        }
        if (ray->alpha >= 255)
            ray->alpha = 255;
        else if (ray->alpha < 0)
            ray->alpha = 0;
        ray->pos.y = ray->height;
        ray->pos.x = self->base.pos.x;
        ray->pos.z = self->base.pos.z;
    }
}

void prim_stretch_ray_render(struct EffectPrim *self, const float *view)
{
    struct PrimStretchRay *ray = self->records.stretch_rays;
    short yaw = self->camera_yaw, pitch = self->camera_pitch;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ray++) {
        float size, near_reach, nx, nz, fx, fz;
        int deg, side, side_back, far_side, far_back, k;
        Vec3 c[4];

        if (ray->active != 1 || ray->alpha <= 0)
            continue;
        size = ray->size;
        deg = ray->angle;
        near_reach = size - (float)ray->stretch * size / 50.0f;
        nx = g_cosTable[deg] * near_reach;
        nz = g_sinTable[deg] * near_reach;
        fx = g_cosTable[deg] * size;
        fz = g_sinTable[deg] * size;

        side = deg - 90;
        if (side < 0)
            side += 360;
        side_back = side - 180 < 0 ? side + 180 : side - 180;
        far_side = deg >= 90 ? deg - 90 : deg + 270;
        far_back = far_side - 180;
        if (far_back < 0)
            far_back = far_side + 180;

        c[0].x = g_cosTable[side] * size * 0.5f + nx;
        c[0].z = g_sinTable[side] * size * 0.5f + nz;
        c[1].x = g_cosTable[side_back] * size * 0.5f + nx;
        c[1].z = g_sinTable[side_back] * size * 0.5f + nz;
        c[3].x = g_cosTable[far_side] * size * 0.5f + fx;
        c[3].z = g_sinTable[far_side] * size * 0.5f + fz;
        c[2].x = g_cosTable[far_back] * size * 0.5f + fx;
        c[2].z = g_sinTable[far_back] * size * 0.5f + fz;
        for (k = 0; k < 4; k++)
            c[k].y = 0.0f;
        prim_quad_turn(&c[0], &c[1], &c[2], &c[3], yaw, pitch);
        for (k = 0; k < 4; k++) {
            c[k].x = ray->pos.x + c[k].x;
            c[k].y = ray->pos.y + c[k].y;
            c[k].z = ray->pos.z + c[k].z;
        }
        prim_draw_quad(self, &count, ray->alpha, c[0], c[1], c[2], c[3], view, PRIM_BLEND_ADDITIVE, ray->texture, 1,
                       0x64, 0x64, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    }
}

void prim_animated_panel_process(struct EffectPrim *self)
{
    struct PrimAnimatedPanel *panel = self->records.animated_panels;
    int i;

    for (i = 0; i < 4; i++, panel++)
        if (panel->active > 0)
            panel->frame++;
}

static Vec3 panel_point(float cos_t, float sin_t, float lx, float y, float lz)
{
    Vec3 p;

    p.x = cos_t * lx - sin_t * lz;
    p.y = y;
    p.z = sin_t * lx + cos_t * lz;
    return p;
}

void prim_animated_panel_render(struct EffectPrim *self, const float *view)
{
    struct PrimAnimatedPanel *panel = self->records.animated_panels;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, panel++) {
        float cos_t, sin_t, size, left, right, top, plate;
        unsigned char tex;

        if (panel->active != 1 || panel->alpha <= 0)
            continue;
        tex = (unsigned char)(panel->frame % 30);
        cos_t = g_cosTable[panel->angle];
        sin_t = g_sinTable[panel->angle];

        size = panel->size;
        left = size * -0.5f;
        right = size * 0.5f;
        top = size * -2.0f;
        prim_draw_quad(self, &count, panel->alpha, panel_point(cos_t, sin_t, right, top, 0.0f),
                       panel_point(cos_t, sin_t, left, top, 0.0f), panel_point(cos_t, sin_t, left, top, size),
                       panel_point(cos_t, sin_t, right, top, size), view, BLEND_ALPHA, (char)(tex + 87), 0, 0xff, 0xff,
                       0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);

        size = panel->size;
        left = size * -0.5f;
        right = size * 0.5f;
        prim_draw_quad(self, &count, panel->alpha, panel_point(cos_t, sin_t, right, 0.0f, 0.0f),
                       panel_point(cos_t, sin_t, left, 0.0f, 0.0f), panel_point(cos_t, sin_t, left, -size, 0.0f),
                       panel_point(cos_t, sin_t, right, -size, 0.0f), view, BLEND_ALPHA, (char)tex, 0, 0xff, 0xff, 0xff,
                       0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);

        size = panel->size;
        left = size * -0.5f;
        right = size * 0.5f;
        top = size * -2.0f;
        prim_draw_quad(self, &count, panel->alpha, panel_point(cos_t, sin_t, right, -size, 0.0f),
                       panel_point(cos_t, sin_t, left, -size, 0.0f), panel_point(cos_t, sin_t, left, top, 0.0f),
                       panel_point(cos_t, sin_t, right, top, 0.0f), view, BLEND_ALPHA, (char)(tex + 30), 0, 0xff, 0xff,
                       0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);

        plate = panel->size * 0.47f;
        prim_draw_quad(self, &count, panel->alpha, panel_point(cos_t, sin_t, plate, panel->size * -0.0f, -1.0f),
                       panel_point(cos_t, sin_t, -plate, panel->size * -0.0f, -1.0f),
                       panel_point(cos_t, sin_t, -plate, panel->size * -0.6f, -1.0f),
                       panel_point(cos_t, sin_t, plate, panel->size * -0.6f, -1.0f), view, BLEND_ALPHA,
                       (char)(panel->frame % 27 + 60), 0, 0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f,
                       PRIM_QUAD_PROJECTED);
    }
}
