#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define COS_DEG(d) g_cosTable[d]
#define SIN_DEG(d) g_sinTable[d]

#define BLEND_ALPHA 1

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

static int rand_mod(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
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

void prim_framed_panel_render(struct EffectPrim *self, const float *view)
{
    struct PrimFramedPanel *pn = self->records.framed_panels;
    unsigned char grey = 10;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, pn++) {
        Vec3 p0, p1, p2, p3;
        short alpha;
        float inner, outer;

        if (pn->active != 1)
            continue;
        alpha = pn->alpha;
        if (alpha <= 0)
            continue;
        if (pn->bright == 1)
            grey = 30;

        inner = pn->size;
        p0 = vec3(0.0f, 0.0f, 0.0f);
        p1 = vec3(0.0f, 0.0f, inner);
        p2 = vec3(inner, 0.0f, inner);
        p3 = vec3(inner, 0.0f, 0.0f);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);

        inner = pn->size;
        p0 = vec3(0.0f, 0.0f, 0.0f);
        p1 = vec3(0.0f, 0.0f, -inner);
        p2 = vec3(inner, 0.0f, -inner);
        p3 = vec3(inner, 0.0f, 0.0f);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, pn->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);

        inner = pn->size;
        p0 = vec3(0.0f, 0.0f, 0.0f);
        p1 = vec3(0.0f, 0.0f, inner);
        p2 = vec3(-inner, 0.0f, inner);
        p3 = vec3(-inner, 0.0f, 0.0f);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, pn->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);

        inner = -pn->size;
        p0 = vec3(0.0f, 0.0f, 0.0f);
        p1 = vec3(0.0f, 0.0f, inner);
        p2 = vec3(inner, 0.0f, inner);
        p3 = vec3(inner, 0.0f, 0.0f);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, pn->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);

        inner = pn->size;
        outer = inner + 100.0f;
        p0 = vec3(inner, 0.0f, -inner);
        p1 = vec3(outer, 0.0f, -inner);
        p2 = vec3(outer, 0.0f, outer);
        p3 = vec3(inner, 0.0f, outer);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, pn->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 1, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);

        inner = pn->size;
        outer = inner + 100.0f;
        p0 = vec3(inner, 0.0f, inner);
        p1 = vec3(inner, 0.0f, outer);
        p2 = vec3(-outer, 0.0f, outer);
        p3 = vec3(-outer, 0.0f, inner);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, pn->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 1, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);

        inner = pn->size;
        outer = -(inner + 100.0f);
        p0 = vec3(-inner, 0.0f, inner);
        p1 = vec3(outer, 0.0f, inner);
        p2 = vec3(outer, 0.0f, outer);
        p3 = vec3(-inner, 0.0f, outer);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, pn->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 1, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);

        inner = pn->size;
        outer = inner + 100.0f;
        p0 = vec3(-inner, 0.0f, -inner);
        p1 = vec3(-inner, 0.0f, -outer);
        p2 = vec3(outer, 0.0f, -outer);
        p3 = vec3(outer, 0.0f, -inner);
        prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
        draw_quad(self, &count, pn->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 1, 0, grey, grey, grey,
                  PRIM_QUAD_PROJECTED);
    }
}

void prim_scroll_grid_process(struct EffectPrim *self)
{
    struct PrimScrollGrid *gr;
    int i;

    face_camera(self);
    follow_master(self);
    gr = self->records.scroll_grids;
    for (i = 0; i < 4; i++, gr++) {
        int age;

        if (gr->active != 1)
            continue;
        age = ++gr->age;
        if (gr->scroll >= 30.0f)
            gr->scroll = gr->scroll - 30.0f;
        if (gr->style == 1) {
            if (age < 10) {
                if (gr->alpha <= 45)
                    gr->alpha += 10;
            } else if (age > 65 && gr->alpha >= 10) {
                gr->alpha -= 10;
            }
        } else if (gr->alpha <= 45) {
            gr->alpha += 5;
        }
    }
}

void prim_scroll_grid_render(struct EffectPrim *self, const float *view)
{
    struct PrimScrollGrid *gr = self->records.scroll_grids;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, gr++) {
        int across, deep;

        if (gr->active != 1 || gr->alpha <= 0)
            continue;
        for (across = -150; across < 150; across += 30) {
            for (deep = -90; deep < 90; deep += 30) {
                float scroll = gr->scroll, cell = gr->cell;
                Vec3 p0, p1, p2, p3;
                unsigned char r, g, b;

                p0 = vec3(scroll + (float)across, -10.0f, scroll + (float)deep);
                p1 = vec3((cell + (float)across) + scroll, -10.0f, scroll + (float)deep);
                p2 = vec3((cell + (float)across) + scroll, -10.0f, (cell + (float)deep) + scroll);
                p3 = vec3(scroll + (float)across, -10.0f, (cell + (float)deep) + scroll);
                prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
                if (gr->style == 0) {
                    r = 255, g = 255, b = 255;
                } else if (gr->style == 1) {
                    r = 255, g = 0, b = 0;
                } else if (gr->style == 3) {
                    r = 255, g = 255, b = 255;
                } else {
                    r = 0, g = 0, b = 255;
                }
                draw_quad(self, &count, gr->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, r, g, b,
                          PRIM_QUAD_PROJECTED);
            }
        }
    }
}

void prim_leaning_ring_render(struct EffectPrim *self, const float *view)
{
    struct PrimRing *ring = self->records.rings;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ring++) {
        Vec3 bottom, top, prev_bottom, prev_top;
        int seg = 0, arc_pos = 0, step;

        if (count >= 256 || ring->active != 1)
            continue;
        step = ring->span / 20;
        if (ring->span < 0)
            continue;
        do {
            int deg = ring->turn + arc_pos;
            float radius, cos_d, sin_d, lean_out, lean_up;

            if (deg >= 360)
                deg -= 360;
            if (ring->span == 360 && seg == 20)
                deg = ring->turn;
            cos_d = COS_DEG(deg);
            sin_d = SIN_DEG(deg);
            radius = ring->radius;
            bottom.x = radius * cos_d;
            bottom.y = -3.0f;
            bottom.z = radius * sin_d;
            lean_out = COS_DEG(ring->tilt) * ring->height;
            lean_up = SIN_DEG(ring->tilt) * ring->height;
            top.x = lean_out * cos_d + radius * cos_d;
            top.y = -(lean_up + 3.0f);
            top.z = lean_out * sin_d + radius * sin_d;
            if (seg > 0 && ring->alpha > 0)
                prim_draw_strip(self, &count, seg - 1, ring->alpha, prev_bottom, bottom,
                                top, prev_top, view, 0);
            seg++;
            if (seg >= 21)
                seg = 20;
            arc_pos += step;
            prev_bottom = bottom;
            prev_top = top;
        } while (arc_pos <= ring->span);
    }
}

void prim_column_ring_render(struct EffectPrim *self, const float *view)
{
    struct PrimRing *ring = self->records.rings;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ring++) {
        Vec3 bottom, top, prev_bottom, prev_top;
        int seg = 0, step, pass;

        if (ring->active != 1)
            continue;
        step = ring->span / 10;
        for (pass = 10; pass != 0; pass--) {
            int arc_pos = 0;

            if (0 > ring->span)
                continue;
            do {
                int deg = (ring->turn + arc_pos) % 360;
                float radius = ring->radius, cos_d = COS_DEG(deg), sin_d = SIN_DEG(deg), lean_out, lean_up;

                bottom.x = radius * cos_d;
                bottom.y = 0.0f;
                bottom.z = radius * sin_d;
                lean_out = COS_DEG(ring->tilt) * ring->column[seg];
                lean_up = SIN_DEG(ring->tilt) * ring->column[seg];
                top.x = lean_out * cos_d + radius * cos_d;
                top.y = -lean_up;
                top.z = lean_out * sin_d + radius * sin_d;
                if (seg > 0)
                    prim_draw_strip(self, &count, seg - 1, ring->alpha, prev_bottom, bottom,
                                    top, prev_top, view, 0);
                seg++;
                if (seg >= 21)
                    seg = 20;
                arc_pos += step;
                prev_bottom = bottom;
                prev_top = top;
            } while (arc_pos <= ring->span);
        }
    }
}

void prim_blade_fan_render(struct EffectPrim *self, const float *view)
{
    struct PrimBladeFan *fan = self->records.blade_fans;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, fan++) {
        int blade;

        if (fan->active != 1 || fan->alpha <= 0)
            continue;
        for (blade = 0; blade <= 2; blade++) {
            int deg = fan->turn - 1 + blade;
            float inner, outer, cos_d, sin_d, height;
            int alpha;
            Vec3 p0, p1, p2, p3;

            if (deg < 0)
                deg += 360;
            else if (deg >= 360)
                deg -= 360;
            inner = fan->inner;
            cos_d = COS_DEG(deg);
            sin_d = SIN_DEG(deg);
            height = fan->height;
            p0 = vec3(inner * cos_d, -0.0f, inner * sin_d);
            p3 = vec3(inner * cos_d, -0.0f - (blade == 1 ? height * 0.2f : height * 0.1f), inner * sin_d);
            outer = inner + fan->length;
            p1 = vec3(outer * cos_d, -0.0f, outer * sin_d);
            p2 = vec3(outer * cos_d, -0.0f - (blade == 1 ? height : height * 0.4f), outer * sin_d);
            alpha = fan->alpha;
            if (blade != 1)
                alpha /= 3;
            draw_quad(self, &count, (short)alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, 255, 255, 255,
                      PRIM_QUAD_PROJECTED);
        }
    }
}

void prim_arch_render(struct EffectPrim *self, const float *view)
{
    struct PrimArch *arches = self->records.arches;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++) {
        struct PrimArch *ar = &arches[i];
        int quad;

        if (ar->active != 1)
            continue;
        for (quad = 0; quad < 10; quad++) {
            int turn = ar->turn;
            int heading, left_deg, right_deg, curl_deg;
            float half_width = ar->half_width, lift = ar->lift, left_x, left_z, right_x, right_z, curl_radius,
                  curl_out, curl_up, tip_x, tip_z;
            Vec3 p0, p1, p2, p3;

            left_deg = (turn < 425 ? -65 : -425) + turn;
            if (left_deg < 0)
                left_deg += 360;
            left_x = COS_DEG(left_deg) * half_width;
            left_z = SIN_DEG(left_deg) * half_width;
            right_deg = (turn < 295 ? 65 : -295) + turn;
            if (right_deg < 0)
                right_deg += 360;
            right_x = COS_DEG(right_deg) * half_width;
            right_z = SIN_DEG(right_deg) * half_width;
            if (quad < 5)
                curl_radius = ar->radius;
            else
                curl_radius = SIN_DEG(ar->wobble) * 0.5f + 0.5f + ar->radius;
            heading = turn >= 360 ? turn - 360 : turn;
            if (heading < 0)
                heading += 360;
            curl_deg = 3 * (quad + quad / 5 * 10);
            if (quad < 5)
                curl_deg += ar->phase;
            curl_out = COS_DEG(curl_deg) * curl_radius;
            curl_up = SIN_DEG(curl_deg) * curl_radius;
            tip_x = COS_DEG(heading) * curl_out;
            tip_z = SIN_DEG(heading) * curl_out;
            ar->tip_x = tip_x;
            ar->tip_z = tip_z;
            p0 = vec3(left_x, lift, left_z);
            p1 = vec3(tip_x + left_x, lift - curl_up, left_z + tip_z);
            p2 = vec3(right_x + tip_x, lift - curl_up, tip_z + right_z);
            p3 = vec3(right_x, lift, right_z);

            if (i == 1 || i == 0) {
                int layer = quad % 5;
                int alpha = ar->alpha;
                short blend;
                unsigned char rg;

                if (layer < 4) {
                    int fade;

                    for (fade = 0; fade < 4 - layer; fade++)
                        alpha = (int)((float)alpha * 0.7f);
                    rg = 105;
                    blend = PRIM_BLEND_ADDITIVE;
                } else {
                    rg = 205;
                    blend = BLEND_ALPHA;
                }
                if (i == 1)
                    draw_quad(self, &count, (short)alpha, p0, p1, p2, p3, view, blend, 0, 0, rg, rg, 255,
                              PRIM_QUAD_PROJECTED);
                else
                    draw_quad(self, &count, (short)alpha, p3, p2, p1, p0, view, blend, 0, 0, rg, rg, 255,
                              PRIM_QUAD_PROJECTED);
            }
        }
    }
}

void prim_tilted_band_render(struct EffectPrim *self, const float *view)
{
    struct PrimRing *ring = self->records.rings;
    Vec3 prev_a = {0.0f, 0.0f, 0.0f}, prev_b = {0.0f, 0.0f, 0.0f};
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ring++) {
        int seg = 0, arc_pos = 0, step;

        if (ring->active != 1)
            continue;
        step = ring->span / 20;
        if (ring->span < 0)
            continue;
        do {
            int deg = ring->turn + arc_pos;
            float radius = ring->radius, height = ring->height, tilt_cos, tilt_sin, cos_d, sin_d, hs, rc, rs, hc, sr;
            Vec3 pa, pb;

            if (deg >= 360)
                deg -= 360;
            else if (deg < 0)
                deg += 360;
            if (ring->span == 360 && seg == 20)
                deg = ring->turn;
            tilt_cos = COS_DEG(ring->tilt);
            tilt_sin = SIN_DEG(ring->tilt);
            cos_d = COS_DEG(deg);
            sin_d = SIN_DEG(deg);
            hs = -(height * tilt_sin);
            rc = radius * tilt_cos * cos_d;
            sr = sin_d * radius;
            rs = radius * tilt_sin * cos_d;
            hc = height * tilt_cos;
            pa = vec3(hs + rc, (hc + rs) + ring->column[0], sr + 0.0f);
            pb = vec3(rc - hs, (rs - hc) + ring->column[0], sr - 0.0f);
            if (seg > 0)
                prim_draw_strip(self, &count, seg - 1, ring->alpha, prev_a, pa, pb, prev_b, view, 0);
            prev_a = pa;
            prev_b = pb;
            seg++;
            if (seg >= 21)
                seg = 20;
            arc_pos += step;
        } while (arc_pos <= ring->span);
    }
}

static int wrap_low(int a)
{
    return a < 0 ? a + 360 : a;
}

void prim_tilted_square_render(struct EffectPrim *self, const float *view)
{
    struct PrimTiltedSquare *sq = self->records.tilted_squares;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, sq++) {
        int turn, corner_deg[4], corner;
        float radius, tilt_cos, tilt_sin;
        Vec3 p[4];

        if (sq->active != 1)
            continue;
        turn = sq->turn;
        radius = sq->radius;
        tilt_cos = COS_DEG(sq->tilt);
        tilt_sin = SIN_DEG(sq->tilt);
        corner_deg[0] = wrap_low(turn >= 360 ? turn - 360 : turn);
        corner_deg[1] = wrap_low((turn < 270 ? 90 : -270) + turn);
        corner_deg[2] = wrap_low((turn < 180 ? 180 : -180) + turn);
        corner_deg[3] = wrap_low((turn < 90 ? 270 : -90) + turn);
        for (corner = 0; corner < 4; corner++) {
            float rise = SIN_DEG(corner_deg[corner]) * radius;

            p[corner] = vec3(COS_DEG(corner_deg[corner]) * radius + sq->pos.x, sq->pos.y + rise * tilt_cos,
                             sq->pos.z + rise * tilt_sin);
        }
        draw_quad(self, &count, sq->alpha, p[0], p[1], p[2], p[3], view, PRIM_BLEND_ADDITIVE, 0, 0,
                  (unsigned char)(int)sq->red, (unsigned char)(int)sq->green, (unsigned char)(int)sq->blue,
                  PRIM_QUAD_PROJECTED);
    }
}

void prim_scroll_column_render(struct EffectPrim *self, const float *view)
{
    struct PrimScrollColumn *col = self->records.scroll_columns;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, col++) {
        int period, phase, band;
        float base;

        if (col->active != 1 || col->alpha <= 0)
            continue;
        period = (int)col->steps;
        if (period == 0) {
            phase = 0;
            base = 40.0f;
            period = 1;
        } else {
            phase = col->age % (period * 3);
            base = (float)(col->age % period) * 40.0f / (float)period;
        }
        if (col->style == 1 || col->style == 10)
            base = 40.0f - base;

        for (band = 0; band < 5; band++) {
            float left, right, z, y1, y2, v_p2p3 = 0.0f, v_p0p1 = 1.0f;
            Vec3 p0, p1, p2, p3;
            int tex, frame_step;
            unsigned char r, g, b;
            short blend;

            if (period == 1 && band > 0)
                break;
            left = col->width * -0.5f;
            right = col->width * 0.5f;
            z = col->depth;
            y1 = base - col->edge[band];
            y2 = base - col->edge[band + 1];
            if (col->squash > 10) {
                y1 = y1 * 10.0f / (float)col->squash;
                y2 = y2 * 10.0f / (float)col->squash;
            }

            tex = band % 3;
            frame_step = phase / period;
            if (col->style != 1 && col->style != 10) {
                tex += frame_step;
                if (tex > 2)
                    tex -= 3;
            } else {
                tex -= frame_step;
                if (tex < 0)
                    tex += 3 + ~tex / 3 * 3;
            }

            if (band == 0) {
                float fill = base * 0.025f;

                v_p2p3 = fill;
                y1 = y1 - (y1 - y2) * fill;
            } else if (band == 4) {
                float fill = base * 0.025f;

                v_p0p1 = fill;
                y2 = (y1 - y2) * (1.0f - fill) + y2;
            }

            if (col->lengthwise == 1) {
                p0 = vec3(z, y2, left);
                p1 = vec3(z, y2, right);
                p2 = vec3(z, y1, right);
                p3 = vec3(z, y1, left);
            } else {
                p0 = vec3(left, y2, z);
                p1 = vec3(right, y2, z);
                p2 = vec3(right, y1, z);
                p3 = vec3(left, y1, z);
            }

            if (col->style == 1) {
                r = 55, g = 55, b = 255;
                blend = PRIM_BLEND_ADDITIVE;
            } else if (col->style == 10) {
                r = 255, g = 55, b = 255;
                blend = PRIM_BLEND_ADDITIVE;
            } else {
                r = 255, g = 255, b = 255;
                blend = BLEND_ALPHA;
            }
            prim_draw_quad(self, &count, col->alpha, p0, p1, p2, p3, view, blend, (char)tex, 0, r, g, b, v_p2p3,
                           v_p0p1, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
        }
    }
}

void prim_camera_panel_render(struct EffectPrim *self, const float *view)
{
    struct PrimScrollGrid *gr = self->records.scroll_grids;
    int count = 0;
    Vec3 p0, p1, p2, p3;

    if (gr->active != 1 || gr->alpha <= 0)
        return;
    p0 = vec3(-200.0f, -10.0f, -200.0f);
    p1 = vec3(200.0f, -10.0f, -200.0f);
    p2 = vec3(200.0f, -10.0f, 200.0f);
    p3 = vec3(-200.0f, -10.0f, 200.0f);
    prim_quad_turn(&p0, &p1, &p2, &p3, self->camera_yaw, self->camera_pitch);
    draw_quad(self, &count, gr->alpha, p0, p1, p2, p3, view, BLEND_ALPHA, 0, 0, (unsigned char)self->argb,
              (unsigned char)(self->argb >> 8), (unsigned char)(self->argb >> 16), PRIM_QUAD_PROJECTED);
}

void prim_framed_panel_process(struct EffectPrim *self)
{
    struct PrimFramedPanel *fp = self->records.framed_panels;
    int i;

    face_camera(self);
    follow_master(self);
    for (i = 0; i < 4; i++, fp++) {
        if (fp->active != 1)
            continue;
        fp->age++;
        if (fp->alpha < 255)
            fp->alpha++;
    }
}

void prim_leaning_ring_process(struct EffectPrim *self)
{
    struct PrimRing *ring = self->records.rings;
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        if (ring->active != 1)
            continue;
        ring->age++;
        if (ring->tilt < 0)
            ring->tilt = 0;
        if (ring->turn >= 360)
            ring->turn -= 360;
        if (ring->age < 9) {
            if (ring->mode == 0) {
                ring->alpha += 10;
                if (ring->alpha > 80)
                    ring->alpha = 80;
            } else {
                ring->alpha += 5;
                if (ring->alpha > 40)
                    ring->alpha = 40;
            }
        } else {
            ring->alpha -= 30;
            if (ring->alpha < 0) {
                ring->alpha = 0;
                ring->active = 0;
            }
        }
    }
}

void prim_column_ring_process(struct EffectPrim *self)
{
    struct PrimRing *ring = self->records.rings;
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        if (ring->active != 1)
            continue;
        if (ring->age++ < 10 && ring->alpha < 100)
            ring->alpha++;
    }
}

void prim_blade_fan_process(struct EffectPrim *self)
{
    struct PrimBladeFan *bf = self->records.blade_fans;
    int i;

    follow_master(self);
    for (i = 0; i < 4; i++, bf++) {
        int fade_in;

        if (bf->active != 1)
            continue;
        bf->age++;
        bf->inner = bf->inner + 1.0f;
        if (bf->inner > 10.0f) {
            bf->alpha -= 2;
            if (bf->alpha < 0) {
                bf->alpha = 0;
                bf->active = 0;
            }
        }
        if (bf->style == 1) {
            bf->length = bf->length + 1.0f;
            if (2.0f > bf->height)
                bf->height = bf->height + 0.2f;
            fade_in = 5;
        } else {
            bf->length = bf->length + 2.0f;
            if (4.0f > bf->height)
                bf->height = bf->height + 0.4f;
            fade_in = 7;
        }
        if (bf->age <= fade_in)
            bf->alpha += 5;
    }
}

void prim_arch_process(struct EffectPrim *self)
{
    struct PrimArch *ar = self->records.arches;
    int i;

    follow_master(self);
    for (i = 0; i < 4; i++, ar++) {
        const struct RenderObject *player;

        if (ar->active != 1)
            continue;
        ar->age++;
        ar->wobble++;
        if (ar->wobble >= 360)
            ar->wobble -= 360;
        ar->lift = -9.0f - (float)(self->camera_pitch - 115) * 0.17f;
        ar->phase = (short)(int)(SIN_DEG(ar->wobble) * 20.0f + 20.0f);
        player = (const struct RenderObject *)game_current_player();
        if (player)
            ar->turn = (short)(int)(player->roty + (i == 0 || i == 2 ? 135.0f : 45.0f));
    }
}

static void ring_turn_by_speed(struct PrimRing *ring)
{
    ring->turn = (short)(int)((float)ring->turn + ring->column[1]);
    if (ring->turn >= 360)
        ring->turn -= 360;
}

void prim_tilted_band_process(struct EffectPrim *self)
{
    struct PrimRing *ring = self->records.rings;
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        if (ring->active != 1)
            continue;
        ring->age++;
        ring_turn_by_speed(ring);
    }
}

void prim_widening_band_process(struct EffectPrim *self)
{
    struct PrimRing *ring = self->records.rings;
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++, ring++) {
        if (ring->active != 1)
            continue;
        ring->age++;
        if (ring->age <= 0)
            continue;
        if (ring->age <= 25)
            ring->alpha += 2;
        else if (ring->age > 70)
            ring->alpha -= 2;
        if (ring->alpha < 0)
            ring->alpha = 0;
        ring->radius = ring->radius * 1.01f;
        ring->tilt++;
        if (rand_mod(2) == 0)
            ring->tilt++;
        if (ring->tilt >= 360)
            ring->tilt -= 360;
        ring_turn_by_speed(ring);
    }
}

void prim_tilted_square_process(struct EffectPrim *self)
{
    struct PrimTiltedSquare *sq = self->records.tilted_squares;
    int i;

    for (i = 0; i < 4; i++, sq++) {
        if (sq->active != 1)
            continue;
        sq->age++;
        sq->tilt += 2;
        if (sq->tilt >= 360)
            sq->tilt -= 360;
        sq->pos.y = sq->fall + sq->pos.y;
        if (sq->pos.y > -10.0f) {
            sq->alpha -= 10;
            if (sq->alpha < 0) {
                sq->alpha = 0;
                sq->respawns++;
                if (sq->respawns < 3) {
                    sq->pos.x = (float)(rand_mod(101) - 50);
                    sq->pos.z = (float)(rand_mod(101) - 50);
                    sq->pos.y = (float)(-110 - rand_mod(50));
                    sq->age = 0;
                }
            }
        } else if (sq->pos.y > 0.0f) {
            sq->active = 0;
        }
        if (sq->age < 60) {
            sq->alpha += 5;
            if (sq->alpha > 250)
                sq->alpha = 250;
        }
    }
}

void prim_scroll_column_process(struct EffectPrim *self)
{
    struct PrimScrollColumn *sc = self->records.scroll_columns;
    int i;

    for (i = 0; i < 4; i++, sc++) {
        if ((signed char)sc->active > 0)
            sc->age++;
    }
}
