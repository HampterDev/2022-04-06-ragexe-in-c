#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS_DEG(d) g_cosTable[d]
#define SIN_DEG(d) g_sinTable[d]

#define BLEND_ALPHA 1

#define RECORD_COUNT 4

static void draw_quad(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                      const float *view, short blend, char texture, unsigned char in_world, unsigned char r,
                      unsigned char g, unsigned char b, unsigned char mode)
{
    prim_draw_quad(self, count, alpha, p0, p1, p2, p3, view, blend, texture, in_world, r, g, b, 0.0f, 1.0f, 0.0f,
                   1.0f, mode);
}

static Vec3 vec3(float x, float y, float z)
{
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

static void face_camera(struct EffectPrim *self)
{
    self->camera_yaw = (short)(360 - (int)game_camera_heading());
    self->camera_pitch = (short)(450 - (int)game_camera_pitch());
    if (self->camera_pitch >= 360)
        self->camera_pitch -= 360;
}

static void follow_master(struct EffectPrim *self)
{
    self->base.pos = self->master->base.pos;
}

#define STRIPE_COLOR_BRIGHT 10
#define STRIPE_Y (-6.0f)

void prim_slide_stripe_render(struct EffectPrim *self, const float *view)
{
    struct PrimSlideStripe *st = self->records.slide_stripes;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, st++) {
        short alpha;
        int h, right, left;
        float shift_z, shift_x, right_z, right_x, left_x, left_z, start, end, start_dist, end_dist, start_cos,
            start_sin, end_cos, end_sin;
        Vec3 a, b, c, d;
        unsigned char r, g, bl, mode;
        short blend;

        if (st->active != 1 || st->alpha <= 0)
            continue;
        alpha = st->alpha;
        h = st->heading;
        right = h >= 90 ? h - 90 : h + 270;
        shift_z = SIN_DEG(right) * st->side_shift;
        shift_x = COS_DEG(right) * st->side_shift;
        right_z = SIN_DEG(right) * st->half_width + shift_z;
        right_x = COS_DEG(right) * st->half_width + shift_x;

        start = st->start;
        if (100.0f > start) {
            if (h < 180) {
                start_cos = COS_DEG(h + 180);
                start_sin = SIN_DEG(h + 180);
            } else {
                start_cos = COS_DEG(h - 180);
                start_sin = SIN_DEG(h - 180);
            }
            start_dist = st->back - start;
        } else {
            start_dist = start - st->back;
            start_cos = COS_DEG(h);
            start_sin = SIN_DEG(h);
        }
        a = vec3(right_x + start_dist * start_cos, STRIPE_Y, right_z + start_dist * start_sin);

        end = st->length + start;
        if (100.0f > end) {
            int back = h < 180 ? h + 180 : h - 180;
            end_dist = st->back - end;
            end_cos = COS_DEG(back);
            end_sin = SIN_DEG(back);
        } else {
            end_cos = COS_DEG(h);
            end_dist = end - st->back;
            end_sin = SIN_DEG(h);
        }
        b = vec3(right_x + end_dist * end_cos, STRIPE_Y, right_z + end_dist * end_sin);

        left = h < 270 ? h + 90 : h - 270;
        left_x = COS_DEG(left) * st->half_width + shift_x;
        left_z = SIN_DEG(left) * st->half_width + shift_z;
        c = vec3(start_dist * start_cos + left_x, STRIPE_Y, start_dist * start_sin + left_z);
        d = vec3(left_x + end_dist * end_cos, STRIPE_Y, left_z + end_dist * end_sin);

        mode = PRIM_QUAD_PROJECTED;
        blend = BLEND_ALPHA;
        switch (st->color) {
        case 0: r = 225; g = 225; bl = 255; break;
        case 1: r = 205; g = 205; bl = 255; break;
        case 2: r = 185; g = 185; bl = 255; break;
        case 10: r = 255; g = 255; bl = 225; break;
        case 11: r = 255; g = 255; bl = 205; break;
        case 12: r = 255; g = 255; bl = 185; break;
        case 20: r = 225; g = 255; bl = 225; break;
        case 21: r = 205; g = 255; bl = 205; break;
        case 22: r = 185; g = 255; bl = 185; break;
        case 30: r = 105; g = 105; bl = 255; blend = PRIM_BLEND_ADDITIVE; mode = PRIM_QUAD_IN_FRONT; break;
        case 31: r = 65; g = 65; bl = 255; blend = PRIM_BLEND_ADDITIVE; mode = PRIM_QUAD_IN_FRONT; break;
        case 32: r = 25; g = 25; bl = 255; blend = PRIM_BLEND_ADDITIVE; mode = PRIM_QUAD_IN_FRONT; break;
        default: continue;
        }
        draw_quad(self, &count, alpha, d, c, a, b, view, blend, 0, 0, r, g, bl, mode);
    }
}

#define SLASH_COLOR_SPIN 1
#define SLASH_COLOR_BLUE 5

static int wrap360(int a)
{
    if (a >= 360)
        a -= 360;
    if (a < 0)
        a += 360;
    return a;
}

static void slash_square(Vec3 p[4], float cx, float cy, float cz, float w, int upright, int turn)
{
    float cc = COS_DEG(upright), sc = SIN_DEG(upright), cd = COS_DEG(turn), sd = SIN_DEG(turn);
    float wi = (float)(int)w;
    float wi2 = (float)(int)wi;
    float wn = (float)(int)-w;
    float wn2 = (float)(int)wn;
    float vi = wi * cc, vn = wn * cc;
    float yi = wi * sc + cy, yn = wn * sc + cy;

    p[0] = vec3(wi2 * cd - vi * sd + cx, yi, vi * cd + wi2 * sd + cz);
    p[1] = vec3(wn2 * cd - vi * sd + cx, yi, wn2 * sd + vi * cd + cz);
    p[2] = vec3(wn2 * cd - vn * sd + cx, yn, vn * cd + wn2 * sd + cz);
    p[3] = vec3(wi2 * cd - vn * sd + cx, yn, vn * cd + wi2 * sd + cz);
}

static void slash_color(unsigned char color, unsigned char fallback, unsigned char fallback_g, unsigned char *r,
                        unsigned char *g, unsigned char *b)
{
    switch (color) {
    case 3: *r = 255; *g = 100; *b = 0; break;
    case 2: *r = 255; *g = 155; *b = 0; break;
    case 4: *r = 255; *g = 150; *b = 255; break;
    case 5: *r = 100; *g = 100; *b = 255; break;
    default: *r = fallback; *g = fallback_g; *b = fallback; break;
    }
}

void prim_slash_render(struct EffectPrim *self, const float *view)
{
    struct PrimSlash *sl = self->records.slashes;
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, sl++) {
        int row, outer;

        if (sl->active != 1)
            continue;
        for (row = 0, outer = -405; outer < -270; row++, outer += 45) {
            float row_offset = (float)(row - 1);
            int elevation = outer + 450;
            int lift = elevation - 90;
            int col;

            for (col = 0; col < 225; col += 45) {
                float row_dist = row_offset;
                float r1, r2, r3, ring;
                int lat, around, upright, turn;
                Vec3 p[4];
                unsigned char cr, cg, cb;

                if (0.0f > row_dist)
                    row_dist = row_dist * -1.0f;
                lat = wrap360(lift);
                sl->width = (1.5f - row_dist) + 1.5f;
                around = wrap360(col - 90 + sl->turn);
                upright = wrap360(elevation);
                turn = wrap360(col - 180 + sl->turn);

                r1 = sl->radius;
                ring = COS_DEG(lat) * r1;
                slash_square(p, COS_DEG(around) * ring, SIN_DEG(lat) * r1 - (r1 + 2.0f), SIN_DEG(around) * ring,
                             sl->width, upright, turn);
                slash_color(sl->color, 205, 255, &cr, &cg, &cb);
                draw_quad(self, &count, sl->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0, cr, cg,
                          cb, PRIM_QUAD_PROJECTED);

                r2 = sl->radius + 0.2f;
                ring = COS_DEG(lat) * r2;
                slash_square(p, COS_DEG(around) * ring, SIN_DEG(lat) * r2 - sl->radius, SIN_DEG(around) * ring,
                             sl->width, upright, turn);
                slash_color(sl->color, 155, 255, &cr, &cg, &cb);
                draw_quad(self, &count, sl->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0, cr, cg,
                          cb, PRIM_QUAD_PROJECTED);

                r3 = (sl->radius + 0.4f) + sl->flare;
                ring = COS_DEG(lat) * r3;
                slash_square(p, COS_DEG(around) * ring, SIN_DEG(lat) * r3 - sl->radius, SIN_DEG(around) * ring,
                             sl->width, upright, turn);
                slash_color(sl->color, 225, 255, &cr, &cg, &cb);
                if (sl->color == SLASH_COLOR_BLUE)
                    draw_quad(self, &count, sl->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0, cr,
                              cg, cb, PRIM_QUAD_PROJECTED);
                else
                    draw_quad(self, &count, sl->alpha, p[0], p[1], p[2], p[3], view, BLEND_ALPHA, 1, 0, cr, cg, cb,
                              PRIM_QUAD_PROJECTED);
            }
        }
    }
}

#define STRIP_RING_FADING 1
#define STRIP_RING_WIDE   2
#define STRIP_RING_STAYS  3

unsigned char prim_strip_ring_process(struct EffectPrim *self)
{
    struct PrimRing *ring = self->records.rings;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        int age;
        short turn, span, mode;

        if (ring->active != 1)
            continue;
        age = ++ring->age;
        if (age <= 0)
            continue;
        turn = (short)(ring->turn + 5);
        ring->turn = turn;
        if (turn >= 360)
            ring->turn = (short)(turn - 360);
        span = (short)(ring->span + 3);
        ring->span = span;
        mode = ring->mode;
        if (mode == STRIP_RING_WIDE) {
            if (span > 180)
                ring->span = 180;
            if (age > 50) {
                ring->alpha--;
                if (ring->alpha < 0)
                    ring->alpha = 0;
            }
            if (age > 12)
                ring->radius += 0.1f;
        } else {
            if (span > 120)
                ring->span = 120;
            if (mode == STRIP_RING_FADING && age > 20) {
                ring->alpha -= 2;
                if (ring->alpha < 0)
                    ring->alpha = 0;
                ring->radius += 0.1f;
            }
        }
        if (age > 1400) {
            if (mode != STRIP_RING_STAYS)
                ring->alpha -= 3;
            if (ring->alpha < 0)
                ring->alpha = 0;
        }
        if (mode == STRIP_RING_WIDE) {
            if (age < 12)
                ring->alpha++;
        } else if (age < 12) {
            ring->alpha += 10;
            if (ring->alpha > 250)
                ring->alpha = 250;
        }
    }
    return 1;
}

void prim_strip_ring_render(struct EffectPrim *self, const float *view)
{
    struct PrimRing *ring = self->records.rings;
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        Vec3 bottom, top, prev_bottom, prev_top;
        int step, off, j;

        if (ring->active != 1)
            continue;
        step = ring->span / PRIM_RING_COLUMNS;
        j = 0;
        for (off = 0; off < ring->span; off += step) {
            int angle = (ring->turn + off) % 360;
            float c, s, rc, rs, col_c, col_s;

            if ((unsigned short)ring->span == 360 && j == PRIM_RING_COLUMNS - 1)
                angle = ring->turn;
            c = COS_DEG(angle);
            s = SIN_DEG(angle);
            rc = ring->radius * c;
            rs = ring->radius * s;
            bottom.y = -ring->height;
            bottom.x = rc;
            bottom.z = rs;
            col_c = COS_DEG(ring->tilt) * ring->column[j];
            col_s = SIN_DEG(ring->tilt) * ring->column[j];
            top.z = col_c * s + rs;
            top.x = col_c * c + rc;
            top.y = -(ring->height + col_s);
            if (j > 0)
                prim_draw_strip(self, &count, j - 1, ring->alpha, prev_bottom, bottom,
                                top, prev_top, view, 0);
            j++;
            if (j >= PRIM_RING_COLUMNS)
                j = PRIM_RING_COLUMNS - 1;
            prev_bottom = bottom;
            prev_top = top;
        }
    }
}

#define OPEN_BOX_WHITE      0.0f
#define OPEN_BOX_PALE_GREEN 1.0f
#define OPEN_BOX_GREY       2.0f
#define OPEN_BOX_YELLOW     4.0f

static Vec3 box_corner(float x, float z, float c, float s)
{
    return vec3(x * c - z * s, -1.0f, x * s + z * c);
}

void prim_open_box_render(struct EffectPrim *self, const float *view)
{
    struct PrimOpenBox *box = self->records.open_boxes;
    int count = 0;
    int i;
    unsigned char r = 0, g = 0, b = 0, blend = 0;

    for (i = 0; i < RECORD_COUNT; i++, box++) {
        float c, s, top;
        Vec3 *q = box->corner;
        Vec3 t0, t1, t2, t3;
        float style;

        if (box->active != 1 || box->alpha <= 0)
            continue;
        c = COS_DEG(box->angle);
        s = SIN_DEG(box->angle);
        q[2] = box_corner(box->half_x, box->half_z, c, s);
        q[0] = box_corner(box->half_x, -box->half_z, c, s);
        q[1] = box_corner(-box->half_x, -box->half_z, c, s);
        q[3] = box_corner(-box->half_x, box->half_z, c, s);
        top = -1.0f - box->height;
        t0 = vec3(q[0].x, top, q[0].z);
        t1 = vec3(q[1].x, top, q[1].z);
        t2 = vec3(q[2].x, top, q[2].z);
        t3 = vec3(q[3].x, top, q[3].z);

        style = box->style;
        if (style == OPEN_BOX_WHITE) {
            blend = PRIM_BLEND_ADDITIVE;
            r = 255; g = 255; b = 255;
        } else if (style == OPEN_BOX_PALE_GREEN) {
            blend = BLEND_ALPHA;
            r = 235; g = 255; b = 235;
        } else if (style == OPEN_BOX_GREY) {
            blend = BLEND_ALPHA;
            r = 80; g = 80; b = 80;
        } else if (style == OPEN_BOX_YELLOW) {
            r = 255; g = 255;
            b = (unsigned char)(int)((box->blue + 0.0f) + box->blue_add);
            blend = BLEND_ALPHA;
        }

        draw_quad(self, &count, box->alpha, q[2], q[0], t0, t2, view, blend, 0, 0, r, g, b, PRIM_QUAD_PROJECTED);
        draw_quad(self, &count, box->alpha, q[0], q[1], t1, t0, view, blend, 0, 0, r, g, b, PRIM_QUAD_PROJECTED);
        draw_quad(self, &count, box->alpha, q[1], q[3], t3, t1, view, blend, 0, 0, r, g, b, PRIM_QUAD_PROJECTED);
        draw_quad(self, &count, box->alpha, q[3], q[2], t2, t3, view, blend, 0, 0, r, g, b, PRIM_QUAD_PROJECTED);
        if (box->style == OPEN_BOX_GREY) {
            int twice = box->alpha * 2;
            short diag_alpha = (short)(twice > 255 ? 255 : twice);
            draw_quad(self, &count, diag_alpha, q[2], q[1], t1, t2, view, blend, 0, 0, r, g, b, PRIM_QUAD_PROJECTED);
            draw_quad(self, &count, diag_alpha, q[0], q[3], t3, t0, view, blend, 0, 0, r, g, b, PRIM_QUAD_PROJECTED);
        }
    }
}

#define PRISM_COLOR_FADING 11
#define PRISM_COLOR_BLUE   13

void prim_prism_render(struct EffectPrim *self, const float *view)
{
    struct PrimPrism *pr = self->records.prisms;
    int count = 0;
    int i;
    Vec3 prev_bottom = {0.0f, 0.0f, 0.0f}, prev_top = {0.0f, 0.0f, 0.0f};

    for (i = 0; i < RECORD_COUNT; i++, pr++) {
        int k;

        if (pr->active != 1 || pr->alpha <= 0)
            continue;
        for (k = 0; k <= 5; k++) {
            short angle = (short)(pr->turn + k * 72);
            float c, s;
            Vec3 bottom, top;

            if (angle >= 360)
                angle = (short)(angle - 360);
            c = COS_DEG(angle) * pr->radius;
            s = SIN_DEG(angle) * pr->radius;
            bottom = vec3(pr->base.x + c, pr->base.y, s + pr->base.z);
            top = vec3(pr->top.x + c, pr->top.y, pr->top.z + s);
            if (k > 0) {
                if (pr->color == PRISM_COLOR_BLUE)
                    draw_quad(self, &count, pr->alpha, prev_bottom, bottom, top, prev_top, view,
                              PRIM_BLEND_ADDITIVE, 0, 1, 80, 80, 255, PRIM_QUAD_PROJECTED);
                else
                    draw_quad(self, &count, pr->alpha, prev_bottom, bottom, top, prev_top, view,
                              PRIM_BLEND_ADDITIVE, 0, 1, 230, 255, 230, PRIM_QUAD_PROJECTED);
            }
            prev_bottom = bottom;
            prev_top = top;
        }
    }
}

#define HALO_BLACK     0
#define HALO_FOLLOW    1
#define HALO_FINE      2
#define HALO_DARK      3
#define HALO_QUICK     4
#define HALO_BLUE_COPY 5
#define HALO_WIDE_BLUE 6
#define HALO_WIDE_RED  7

static const float halo_cap_2[10] = {33.0f, 56.0f, 79.0f, 102.0f, 125.0f, 148.0f, 171.0f, 194.0f, 217.0f, 240.0f};
static const float halo_cap[10] = {70.0f, 90.0f, 110.0f, 130.0f, 150.0f, 170.0f, 190.0f, 210.0f, 230.0f, 250.0f};

static void halo_fade(struct PrimHalo *h, int k, float step)
{
    float v = h->layer_alpha[k] - step;
    h->layer_alpha[k] = v;
    if (0.0f > v)
        h->layer_alpha[k] = 0.0f;
}

static void halo_fade_grow(struct PrimHalo *h, int k, float grow)
{
    float v = h->layer_alpha[k] - 5.0f;
    h->layer_alpha[k] = v;
    if (v > 0.0f) {
        h->size = h->size * grow;
        v = h->layer_alpha[k];
    }
    if (0.0f > v)
        h->layer_alpha[k] = 0.0f;
}

unsigned char prim_halo_process(struct EffectPrim *self)
{
    struct PrimHalo *h = self->records.halos;
    int i, k;

    face_camera(self);
    if (h[0].style == HALO_FOLLOW)
        follow_master(self);
    for (i = 0; i < RECORD_COUNT; i++, h++) {
        int age;

        if (h->active != 1)
            continue;
        age = ++h->age;
        if (age <= 0)
            continue;
        if (h->style == HALO_FINE) {
            if (age < 20) {
                for (k = 0; k < HALO_LAYERS; k++) {
                    float v = h->layer_alpha[k] + 13.0f;
                    h->layer_alpha[k] = v;
                    if (v >= halo_cap_2[k])
                        h->layer_alpha[k] = halo_cap_2[k];
                }
            } else {
                for (k = 0; k < 8; k++)
                    halo_fade(h, k, 2.0f);
                if (h->age > 100) {
                    halo_fade_grow(h, 8, 1.005f);
                    halo_fade_grow(h, 9, 1.005f);
                }
            }
        } else {
            if (age < 20) {
                for (k = 0; k < HALO_LAYERS; k++) {
                    float v = h->layer_alpha[k] + 13.0f;
                    h->layer_alpha[k] = v;
                    if (v >= halo_cap[k])
                        h->layer_alpha[k] = halo_cap[k];
                }
            } else {
                for (k = 0; k < 9; k++)
                    halo_fade(h, k, 5.0f);
                if (h->age > (h->style == HALO_QUICK ? 25 : 140))
                    halo_fade_grow(h, 9, 1.01f);
            }
            if (h->age < 50 && h->style != HALO_QUICK)
                h->size -= 0.1f;
        }
    }
    return 1;
}

static void halo_square(struct EffectPrim *self, const struct PrimHalo *h, Vec3 *q0, Vec3 *q1, Vec3 *q2, Vec3 *q3,
                        float lift)
{
    float shift = h->shift;

    *q0 = vec3(shift + h->corner_a.x, h->corner_a.y, h->corner_a.z);
    *q1 = vec3(shift + h->corner_b.x, h->corner_b.y, h->corner_b.z);
    *q2 = vec3(shift + h->corner_c.x, h->corner_c.y, h->corner_c.z);
    *q3 = vec3(shift + h->corner_d.x, h->corner_d.y, h->corner_d.z);
    prim_quad_turn(q0, q1, q2, q3, self->camera_yaw, self->camera_pitch);
    q0->y = q0->y - lift;
    q1->y = q1->y - lift;
    q2->y = q2->y - lift;
    q3->y = q3->y - lift;
}

static Vec3 scaled(Vec3 v, float k)
{
    return vec3(v.x * k, v.y * k, v.z * k);
}

void prim_halo_render(struct EffectPrim *self, const float *view)
{
    struct PrimHalo *h = self->records.halos;
    int count = 0;
    int i, j;

    for (i = 0; i < RECORD_COUNT; i++, h++) {
        if (h->active != 1)
            continue;
        for (j = HALO_LAYERS; j >= 1; j--) {
            unsigned char style = h->style;
            int wide = style == HALO_WIDE_BLUE || style == HALO_WIDE_RED;
            int deg_a = wide ? 35 : 45;
            int deg_b = wide ? 145 : 135;
            int layer = HALO_LAYERS - j;
            int deg_c, deg_d, alpha;
            float radius, lift;
            Vec3 q0, q1, q2, q3;

            if (style == HALO_FINE)
                radius = (float)layer + h->size;
            else if (style == HALO_QUICK || style == HALO_BLUE_COPY)
                radius = (float)layer + h->size;
            else
                radius = (float)(layer * 3) + h->size;
            h->corner_a = vec3(COS_DEG(deg_a) * radius, 0.0f, SIN_DEG(deg_a) * radius);
            h->corner_b = vec3(COS_DEG(deg_b) * radius, 0.0f, SIN_DEG(deg_b) * radius);
            deg_c = deg_b + (wide ? 70 : 90);
            if (deg_c >= 360)
                deg_c -= 360;
            h->corner_c = vec3(COS_DEG(deg_c) * radius, 0.0f, SIN_DEG(deg_c) * radius);
            deg_d = deg_c + (wide ? 110 : 90);
            if (deg_d >= 360)
                deg_d -= 360;
            h->corner_d = vec3(COS_DEG(deg_d) * radius, 0.0f, SIN_DEG(deg_d) * radius);

            style = h->style;
            lift = (style == HALO_FINE || style == HALO_QUICK || style == HALO_WIDE_BLUE || style == HALO_WIDE_RED)
                       ? 40.0f
                       : 30.0f;
            halo_square(self, h, &q0, &q1, &q2, &q3, lift);
            alpha = (int)h->layer_alpha[j - 1];
            if (alpha > 0) {
                if (style == HALO_BLACK) {
                    draw_quad(self, &count, (short)alpha, q1, q0, q3, q2, view, BLEND_ALPHA, (char)h->texture, 0,
                              0, 0, 0, PRIM_QUAD_PROJECTED);
                } else if (style == HALO_DARK) {
                    if (j == HALO_LAYERS)
                        draw_quad(self, &count, (short)(alpha / 2), q1, q0, q3, q2, view, BLEND_ALPHA,
                                  (char)h->texture, 0, 10, 10, 10, PRIM_QUAD_PROJECTED);
                    draw_quad(self, &count, (short)alpha, q1, q0, q3, q2, view, BLEND_ALPHA, (char)h->texture, 0,
                              10, 10, 10, PRIM_QUAD_PROJECTED);
                } else if (style == HALO_WIDE_BLUE || style == HALO_WIDE_RED) {
                    draw_quad(self, &count, (short)alpha, q1, q0, q3, q2, view, PRIM_BLEND_ADDITIVE,
                              (char)h->texture, 0, 255, 255, 255, PRIM_QUAD_PROJECTED);
                } else {
                    draw_quad(self, &count, (short)alpha, q1, q0, q3, q2, view, BLEND_ALPHA, (char)h->texture, 0,
                              255, 255, 255, PRIM_QUAD_PROJECTED);
                }
            }

            style = h->style;
            if (style == HALO_BLUE_COPY || style == HALO_WIDE_BLUE || style == HALO_WIDE_RED) {
                h->corner_a = scaled(h->corner_a, 1.1f);
                h->corner_b = scaled(h->corner_b, 1.1f);
                h->corner_d = scaled(h->corner_d, 1.1f);
                h->corner_c = scaled(h->corner_c, 1.1f);
                halo_square(self, h, &q0, &q1, &q2, &q3, h->style == HALO_BLUE_COPY ? 30.0f : 40.0f);
                alpha = (int)h->layer_alpha[j - 1];
                if (alpha > 0) {
                    if (h->style == HALO_WIDE_RED)
                        draw_quad(self, &count, (short)alpha, q1, q0, q3, q2, view, PRIM_BLEND_ADDITIVE,
                                  (char)h->texture, 0, 255, 5, 5, PRIM_QUAD_PROJECTED);
                    else
                        draw_quad(self, &count, (short)alpha, q1, q0, q3, q2, view, PRIM_BLEND_ADDITIVE,
                                  (char)h->texture, 0, 5, 5, 255, PRIM_QUAD_PROJECTED);
                }
            }
        }
    }
}

#define PIPE_STYLE_WHITE 2.0f
#define PIPE_STYLE_PINK  3.0f
#define PIPE_STYLE_BLUE  4.0f
#define PIPE_STYLE_DARK  5.0f

void prim_pipe_render(struct EffectPrim *self, const float *view)
{
    struct PrimPipe *pp = self->records.pipes;
    int count = 0;
    int i;
    Vec3 prev_a = {0.0f, 0.0f, 0.0f}, prev_b = {0.0f, 0.0f, 0.0f};
    float radius = 0.0f;

    for (i = 0; i < RECORD_COUNT; i++, pp++) {
        int side, pass, j;

        if (pp->active != 1 || pp->alpha <= 0)
            continue;
        side = (short)((pp->heading + 90) % 360);
        for (pass = 0; pass < 2; pass++) {
            for (j = 0; j <= 5; j++) {
                float ring, dy, dx, dz;
                Vec3 a, b;
                float style;

                if (pass == 0)
                    radius = pp->radius;
                else if (pass == 1)
                    radius = pp->style == PIPE_STYLE_PINK ? pp->radius + pp->radius : pp->radius + 0.25f;
                ring = COS_DEG(j * 72) * radius;
                dy = SIN_DEG(j * 72) * radius - pp->drop;
                dx = COS_DEG(side) * ring;
                dz = ring * SIN_DEG(side);
                a = vec3(pp->start.x + dx, dy + pp->start.y, dz + pp->start.z);
                b = vec3(pp->end.x + dx, pp->end.y + dy, dz + pp->end.z);
                if (j > 0) {
                    style = pp->style;
                    if (pass == 0) {
                        if (style == PIPE_STYLE_WHITE)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, BLEND_ALPHA, 0, 1, 255,
                                      255, 255, PRIM_QUAD_PROJECTED);
                        else if (style == PIPE_STYLE_PINK)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, BLEND_ALPHA, 0, 1, 255,
                                      89, 182, PRIM_QUAD_PROJECTED);
                        else if (style == PIPE_STYLE_BLUE)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, BLEND_ALPHA, 0, 1, 55,
                                      55, 255, PRIM_QUAD_IN_FRONT);
                        else if (style == PIPE_STYLE_DARK)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, BLEND_ALPHA, 0, 1, 55,
                                      55, 55, PRIM_QUAD_IN_FRONT);
                        else
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, BLEND_ALPHA, 0, 1, 255,
                                      255, 55, PRIM_QUAD_PROJECTED);
                    } else if (pass == 1) {
                        if (style == PIPE_STYLE_WHITE)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0,
                                      1, 255, 255, 255, PRIM_QUAD_PROJECTED);
                        else if (style == PIPE_STYLE_PINK)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0,
                                      1, 255, 89, 182, PRIM_QUAD_PROJECTED);
                        else if (style == PIPE_STYLE_BLUE)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0,
                                      1, 55, 55, 255, PRIM_QUAD_IN_FRONT);
                        else if (style == PIPE_STYLE_DARK)
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0,
                                      1, 55, 55, 55, PRIM_QUAD_IN_FRONT);
                        else
                            draw_quad(self, &count, pp->alpha, prev_a, a, b, prev_b, view, PRIM_BLEND_ADDITIVE, 0,
                                      1, 255, 255, 200, PRIM_QUAD_PROJECTED);
                    }
                }
                prev_a = a;
                prev_b = b;
            }
        }
    }
}

#define WHIP_RED  2
#define WHIP_BLUE 8

static void whip_joint(const struct PrimWhip *wh, int k, float *reach, float *drop)
{
    int link_count = wh->links[k];
    float dist = 0.0f, shrink = 1.0f, bend;
    int m;

    for (m = 0; m < link_count; m++) {
        dist = (wh->link_length + wh->link_length) * shrink + dist;
        shrink = shrink * 0.7f;
    }
    bend = wh->bend[k];
    if (bend > 0.0f)
        dist = dist + (wh->link_length * shrink) * (bend / 90.0f);
    *drop = ((SIN_DEG((short)(int)bend) * wh->link_length) * -1.5f) * shrink;
    *reach = dist;
}

void prim_whip_render(struct EffectPrim *self, const float *view)
{
    struct PrimWhip *wh = self->records.whips;
    int count = 0;
    int i, k, j;
    Vec3 prev_p = {0.0f, 0.0f, 0.0f}, prev_q = {0.0f, 0.0f, 0.0f};

    for (i = 0; i < RECORD_COUNT; i++, wh++) {
        float jy[WHIP_JOINTS] = {0}, jz[WHIP_JOINTS] = {0}, jx[WHIP_JOINTS] = {0};
        int joints = 0;

        /* The joints shimmer: every frame each link up to the link length draws three rand() values
         * (y, z, x, in that order) and moves its joint by up to 0.3 either way. The client draws them
         * for every record whose link length is set, before and whether or not the record is drawn,
         * so the draws happen here before the active test. Joints past the link length keep 0. */
        while (joints < WHIP_JOINTS - 1 && wh->link_length > (float)joints) {
            jy[joints] = (float)rag_rand(7) * 0.1f - 0.3f;
            jz[joints] = (float)rag_rand(7) * 0.1f - 0.3f;
            jx[joints] = (float)rag_rand(7) * 0.1f - 0.3f;
            joints++;
        }
        if (wh->active != 1 || wh->alpha <= 0 || wh->age <= 0)
            continue;
        for (k = 0; k < WHIP_JOINTS - 1; k++) {
            float reach_a, drop_a, reach_b, drop_b, ch, sh, ax, az, bx, bz;
            short facing;
            int side;
            Vec3 a, b;

            whip_joint(wh, k, &reach_a, &drop_a);
            ch = COS_DEG(wh->heading);
            sh = SIN_DEG(wh->heading);
            ax = ch * reach_a + jx[k];
            az = sh * reach_a + jz[k];
            whip_joint(wh, k + 1, &reach_b, &drop_b);
            bx = ch * reach_b + jx[k + 1];
            bz = sh * reach_b + jz[k + 1];
            prim_heading_degrees(ax - bx, az - bz, &facing);
            side = (facing + 90) % 360;
            a = vec3(wh->root.x + ax, wh->root.y + drop_a + jy[k], wh->root.z + az);
            b = vec3(wh->root.x + bx, wh->root.y + drop_b + jy[k + 1], wh->root.z + bz);

            for (j = 0; j <= 3; j++) {
                float across = COS_DEG(j * 120) * wh->thickness;
                float dy = SIN_DEG(j * 120) * wh->thickness;
                float ox = across * COS_DEG(side);
                float oz = across * SIN_DEG(side);
                Vec3 p = vec3(a.x + ox, a.y + dy, a.z + oz);
                Vec3 q = vec3(b.x + ox, b.y + dy, b.z + oz);
                int alpha;

                if (j > 0 && (alpha = wh->alpha - k * 5) > 0) {
                    unsigned char shade = (unsigned char)(k * 7);
                    if (wh->mode == WHIP_RED) {
                        shade = (unsigned char)(shade + 60);
                        draw_quad(self, &count, (short)alpha, prev_p, p, q, prev_q, view, PRIM_BLEND_ADDITIVE, 0, 1,
                                  255, shade, shade, PRIM_QUAD_PROJECTED);
                    } else if (wh->mode == WHIP_BLUE) {
                        shade = (unsigned char)(shade + 10);
                        draw_quad(self, &count, (short)alpha, prev_p, p, q, prev_q, view, PRIM_BLEND_ADDITIVE, 0, 1,
                                  shade, shade, 255, PRIM_QUAD_PROJECTED);
                    } else {
                        shade = (unsigned char)(shade + 10);
                        draw_quad(self, &count, (short)alpha, prev_p, p, q, prev_q, view, PRIM_BLEND_ADDITIVE, 0, 1,
                                  255, 200, shade, PRIM_QUAD_PROJECTED);
                    }
                }
                prev_p = p;
                prev_q = q;
            }
        }
    }
}

void prim_slide_stripe_process(struct EffectPrim *self)
{
    struct PrimSlideStripe *st = self->records.slide_stripes;
    int i;

    face_camera(self);
    for (i = 0; i < RECORD_COUNT; i++, st++) {
        int bright;

        if (st->active != 1)
            continue;
        bright = (signed char)st->color >= STRIPE_COLOR_BRIGHT;
        if (st->age++ < 10)
            st->alpha += bright ? 20 : 12;
        if (bright)
            st->length = st->length + 0.05f;
        st->start = st->start + st->length;
        if (st->start >= 200.0f)
            st->start = st->start - 200.0f;
        if (st->age > 20) {
            st->alpha += bright ? -2 : -4;
            if (st->alpha < 0)
                st->alpha = 0;
        }
    }
}

void prim_slash_process(struct EffectPrim *self)
{
    struct PrimSlash *sl = self->records.slashes;
    int i;

    follow_master(self);
    for (i = 0; i < RECORD_COUNT; i++, sl++) {
        if (sl->active != 1)
            continue;
        if (sl->age++ >= 1 && sl->color == SLASH_COLOR_SPIN)
            sl->turn += 10;
        else
            sl->turn = (short)(int)(self->master->master->roty + 270.0f);
        if (sl->turn >= 360)
            sl->turn -= 360;
        if (sl->age < 10) {
            if (sl->alpha < 100)
                sl->alpha += 20;
        } else if (sl->age > 26) {
            sl->flare = sl->flare + 0.1f;
            if (sl->alpha > 0)
                sl->alpha -= 6;
            if (sl->alpha < 0)
                sl->alpha = 0;
        }
    }
}

void prim_open_box_process(struct EffectPrim *self)
{
    struct PrimOpenBox *box = self->records.open_boxes;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, box++) {
        float swing, height;

        if (box->active != 1)
            continue;
        box->age++;
        if (box->age <= 0)
            continue;
        box->phase++;
        if (box->phase >= 360)
            box->phase -= 360;
        if (box->age > box->fade_age) {
            box->alpha -= 2;
            if (box->alpha < 0)
                box->alpha = 0;
        }
        swing = SIN_DEG(box->phase) * box->full_height * 0.35f;
        box->height = swing;
        height = box->full_height * 0.65f + swing;
        box->height = height;
        if (box->age < 90)
            box->height = SIN_DEG(box->age) * height;

        if (box->style == OPEN_BOX_YELLOW) {
            if (box->blue_falling == 0.0f) {
                box->blue_add = box->blue_add + 1.0f;
                if (box->blue_add > 200.0f) {
                    box->blue_falling = 1.0f;
                    box->blue_add = 200.0f;
                }
            } else if (box->blue_falling == 1.0f) {
                box->blue_add = box->blue_add - 1.0f;
                if (0.0f > box->blue_add) {
                    box->blue_falling = 0.0f;
                    box->blue_add = 0.0f;
                }
            }
        }
    }
}

void prim_prism_process(struct EffectPrim *self)
{
    struct PrimPrism *pr = self->records.prisms;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, pr++) {
        signed char color;

        if ((signed char)pr->active <= 0)
            continue;
        pr->age++;
        if (pr->turn >= 360)
            pr->turn -= 360;
        color = (signed char)pr->color;
        if (color == PRISM_COLOR_BLUE) {
            if (pr->age <= 40)
                pr->alpha++;
            if (pr->alpha <= 0)
                pr->alpha = 0;
        } else if (color >= PRISM_COLOR_FADING) {
            if (pr->age <= 40)
                pr->alpha++;
            else if (pr->age > 140)
                pr->alpha--;
            if (pr->alpha <= 0)
                pr->alpha = 0;
        }
        if (i == 1 || i == 3) {
            float base = pr->base_radius;
            pr->radius = base;
            pr->radius = SIN_DEG(pr->age % 720 / 2) * 0.5f + base;
        }
    }
}

void prim_pipe_process(struct EffectPrim *self)
{
    struct PrimPipe *pp = self->records.pipes;
    int i;

    for (i = 0; i < RECORD_COUNT; i++, pp++) {
        if ((signed char)pp->active <= 0)
            continue;
        pp->age++;
        if (pp->age <= 0)
            continue;
        if (pp->style == PIPE_STYLE_DARK) {
            if (pp->age <= 20) {
                pp->alpha += 8;
            } else {
                pp->alpha -= 2;
                if (pp->alpha < 0)
                    pp->alpha = 0;
            }
        } else if (pp->age <= 5) {
            pp->alpha += pp->style == PIPE_STYLE_WHITE ? 2 : 8;
        } else {
            pp->alpha -= pp->style == PIPE_STYLE_WHITE ? 1 : 4;
            if (pp->alpha < 0)
                pp->alpha = 0;
        }
        pp->end.x = COS_DEG(pp->heading) * pp->length + pp->start.x;
        pp->end.z = SIN_DEG(pp->heading) * pp->length + pp->start.z;
    }
}

void prim_whip_process(struct EffectPrim *self)
{
    struct PrimWhip *wh = self->records.whips;
    int i, k;

    for (i = 0; i < RECORD_COUNT; i++, wh++) {
        if (wh->active != 1)
            continue;
        wh->age++;
        if (wh->age <= 0)
            continue;
        if (wh->age > 35)
            wh->alpha -= 5;
        if (wh->alpha < 0)
            wh->alpha = 0;
        wh->bend[0] = wh->bend[0] + 8.0f;
        if (wh->bend[0] > 180.0f) {
            wh->bend[0] = wh->bend[0] - 180.0f;
            wh->links[0]++;
        }
        for (k = WHIP_JOINTS - 1; k >= 1; k--) {
            wh->bend[k] = wh->bend[k - 1];
            wh->links[k] = wh->links[k - 1];
        }
    }
}
