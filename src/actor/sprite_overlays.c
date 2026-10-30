#include "render/renderer.h"
#include "render/projection.h"
#include "game/own_states.h"
#include "session/state.h"
#include "actor/render_ops.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <stdlib.h>
#include "contents/body_layer.h"
#include "actor/sprite_render.h"
#include "session/session_names.h"
#include "session/session_skills.h"

#define QUAD_IMAGE(q)  (*(void **)&(q)->image)

static float ov_unit(void)
{
    return game_camera_scale();
}

void render_object_draw_pulse_outline(struct RenderObject *self, const struct SprClip *clip, const int *rect,
                           float screen_shift, float depth_a, float depth_b, void *palette, void *image, unsigned colour)
{
    struct SqQuad quad;
    float k, left, right;
    unsigned char phase;

    if (!(self->overlay_flags & 8))
        return;
    phase = ++self->pulse_phase;
    if (phase > 180) {
        self->pulse_phase = 0;
        phase = 0;
    }
    k = g_sinTable[phase] * 1.5f + 5.0f;
    k = ov_unit() * k;
    quad.top = (float)rect[1] - k;
    quad.bottom = (float)rect[3] + k;
    left = (float)rect[0] + screen_shift;
    right = (float)rect[2] + screen_shift;
    if (left > right) {
        left += k;
        right -= k;
    } else {
        left -= k;
        right += k;
    }
    quad.depth_a = depth_a - 5e-5f;
    quad.depth_b = depth_b - 5e-5f;
    quad.color = colour;
    quad.angle = (float)(clip->angle + self->spr_angle);
    QUAD_IMAGE(&quad) = image;
    quad.x0 = left;
    quad.x1 = right;
    QUAD_BUILDER(self, &quad, palette, 5, 0.0f, 0.0f);
}

void render_object_draw_echoes(struct RenderObject *self, const struct SprClip *clip,
                                         const int *rect, float screen_shift, float depth_a, float depth_b, void *palette,
                                         void *image, float scale, short hover)
{
    struct SqQuad quad;
    float top, bottom, left, right, k;
    int dir, h;

    if (!(self->overlay_flags & 0x1000))
        return;
    depth_a += 0.1f;
    depth_b += 0.1f;
    h = hover;
    for (dir = 1; dir <= 4; dir++) {
        short fade, alpha5;
        float t = (float)(short)self->overlay_step * scale;
        k = ov_unit() * t;
        fade = (short)(30 - (short)self->overlay_step);
        alpha5 = (short)(fade + (short)(fade << 2));
        if (alpha5 < 0)
            alpha5 = 0;
        top = (float)rect[1];
        bottom = (float)rect[3];
        if (dir == 1) {
            left = (float)rect[0] + screen_shift + k;
            right = (float)rect[2] + screen_shift + k;
        } else if (dir == 2) {
            left = (float)rect[0] + screen_shift - k;
            right = (float)rect[2] + screen_shift - k;
        } else {
            if (dir == 3) {
                top += k;
                bottom += k;
            } else {
                top -= k;
                bottom -= k;
            }
            left = (float)rect[0] + screen_shift;
            right = (float)rect[2] + screen_shift;
        }
        quad.top = top - (float)h;
        quad.x0 = left;
        quad.bottom = bottom - (float)h;
        quad.x1 = right;
        quad.depth_a = depth_a;
        quad.depth_b = depth_b;
        quad.color = ((unsigned)(int)alpha5 << 24) + 0xffffff;
        quad.angle = (float)(clip->angle + self->spr_angle);
        QUAD_IMAGE(&quad) = image;
        QUAD_BUILDER(self, &quad, palette, 0x201, 0.0f, 0.0f);
    }
}

void render_object_draw_rings(struct RenderObject *self, struct SqQuad *out,
                                        const struct SprClip *clip, const int *rect, float screen_shift, float depth_a,
                                        float depth_b, void *palette, void *image, int flags, short hover)
{
    struct SqQuad quad;
    int step, h;

    if (!(self->overlay_flags & 0x200))
        return;
    if (++self->ring_phase >= 200)
        self->ring_phase += 0x38;
    h = hover;
    for (step = 5; step < 25; step += 5) {
        float v = (float)self->ring_phase * 0.1f + (float)step;
        float k, k2, left, right;
        unsigned short alpha;

        if (v >= 20.0f)
            v -= 20.0f;
        alpha = (unsigned short)(int)(100.0f - v * 5.0f);
        k = ov_unit() * v;
        quad.top = ((float)rect[1] - k) - (float)h;
        quad.bottom = ((float)rect[3] + k) - (float)h;
        left = (float)rect[0] + screen_shift;
        right = (float)rect[2] + screen_shift;
        if (left > right) {
            left += k;
            right -= k;
        } else {
            left -= k;
            right += k;
        }
        k2 = k * 5e-6f;
        quad.x0 = left;
        quad.x1 = right;
        quad.depth_a = (depth_a - k2) - 1e-5f;
        quad.depth_b = (depth_b - k2) - 1e-5f;
        quad.color = ((unsigned)(int)(short)alpha << 24) + 0xffffff;
        quad.angle = (float)(self->spr_angle + clip->angle);
        QUAD_IMAGE(&quad) = image;
        if (flags != 5)
            QUAD_BUILDER(self, &quad, palette, flags, 0.0f, 0.0f);
    }
    out->color = 0x96ffffff;
}

void render_object_jitter_quad(struct RenderObject *self, struct SqQuad *out,
                                         const int *rect, float screen_shift, int range, int bias)
{
    float d[4];
    int i;

    if (!(self->overlay_flags & 0x2000))
        return;
    for (i = 0; i < 4; i++) {
        int r = 0;
        if (range != 0) {
            r = rand() % range;
            if (r < 0)
                r = 0;
        }
        d[i] = ov_unit() * (float)(r - bias);
    }
    out->top = (float)rect[1] + d[0];
    out->bottom = (float)rect[3] + d[1];
    out->x0 = (float)rect[0] + screen_shift + d[2];
    out->x1 = (float)rect[2] + screen_shift + d[3];
}

static void ov_grown(struct RenderObject *self, const struct SprClip *clip, const int *rect, float screen_shift, float k,
                     float depth_a, float depth_b, void *palette, void *image, unsigned colour, int k2_first)
{
    struct SqQuad quad;
    float left, right, k2;

    quad.top = (float)rect[1] - k;
    quad.bottom = (float)rect[3] + k;
    left = (float)rect[0] + screen_shift;
    right = (float)rect[2] + screen_shift;
    if (left > right) {
        left += k;
        right -= k;
    } else {
        left -= k;
        right += k;
    }
    k2 = k * 5e-6f;
    quad.x0 = left;
    quad.x1 = right;
    quad.depth_a = k2_first ? k2 + depth_a : depth_a + k2;
    quad.depth_b = k2_first ? k2 + depth_b : depth_b + k2;
    quad.color = colour;
    quad.angle = (float)(self->spr_angle + clip->angle);
    QUAD_IMAGE(&quad) = image;
    QUAD_BUILDER(self, &quad, palette, 5, 0.0f, 0.0f);
}

void render_object_draw_clip_overlays(struct RenderObject *self, struct SqQuad *quad,
                           const struct SprClip *clip, const int *rect, float screen_shift, float depth_a, float depth_b,
                           void *palette, void *image, int *flags, unsigned colour)
{
    int i;

    if ((unsigned char)(colour >> 24) < 0xff)
        *flags = 1;
    if (self->overlay_flags & 1)
        *flags = 5;
    render_object_draw_pulse_outline(self, clip, rect, screen_shift, depth_a, depth_b, palette, image, colour);

    if (self->overlay_flags & 0x800) {
        float za = depth_a + 1e-5f, zb = depth_b + 1e-5f;
        int n;
        for (i = 1, n = 3; n <= 30; i++, n += 3) {
            unsigned short f4 = self->overlay_step;
            unsigned short a;
            short alpha;
            if ((short)f4 <= 0x82)
                a = (unsigned short)(f4 - (unsigned short)(5 * i + 0x32));
            else
                a = (unsigned short)(0xf5 - 10 * i - f4);
            if ((short)a > 200)
                a = 200;
            alpha = (short)a >= 0 ? (short)a : 0;
            ov_grown(self, clip, rect, screen_shift, ov_unit() * (float)n, za, zb, palette, image,
                     ((unsigned)(int)alpha << 24) + 0xffffff, 0);
        }
    }
    if (self->overlay_flags & 0x80002) {
        float za = depth_a + 1e-5f, zb = depth_b + 1e-5f;
        for (i = 1; i <= 2; i++) {
            unsigned alpha = self->outline_alpha;
            float k = ov_unit() * (float)i;
            ov_grown(self, clip, rect, screen_shift, k, za, zb, palette, image,
                     (alpha << 24) + ((self->overlay_flags & 2) ? 0x3737ff : 0x59b05), 1);
        }
    }

    render_object_draw_echoes(self, clip, rect, screen_shift, depth_a, depth_b, palette, image,
                                              (float)(rect[3] - rect[1]) * 0.05f, 0);
    render_object_draw_rings(self, quad, clip, rect, screen_shift, depth_a, depth_b, palette, image,
                                             *flags, 0);
    render_object_jitter_quad(self, quad, rect, screen_shift, 0x15, 0xa);
    if (self->overlay_flags & 0x100000) {
        unsigned short turn = (unsigned short)((unsigned short)clip->angle + (unsigned short)self->spr_angle + 0x3c);
        short r = (short)turn < 0x168 ? (short)turn : (short)(turn - 0x168);
        quad->angle = (float)r;
    }
    if (self->overlay_flags & 2)
        quad->color = 0x643737ff;
}

void pc_draw_layer_overlays(struct Pc *pc, struct SqQuad *quad, struct SprClip *clip,
                           int layer, const int *rect, float screen_shift, float z, float a, float b, float x, float y,
                           void *palette, void *image, int *flags, int dx, int dy)
{
    struct RenderObject *object = (struct RenderObject *)pc;
    const short *img = (const short *)image;
    struct SqQuad q;
    unsigned short hover = 0;
    float hover_f, k, l, r, depth_a = 0.0f, depth_b = 0.0f;
    int pass, step;

    if (own_states_find(0xb8) && (!pc->visible || !pc->revealed) && !effect_state_bit6(pc->effect_state))
        return;

    if ((pc->overlay_flags & 1) && layer <= 4)
        *flags = 5;
    if ((pc->overlay_flags & 2) && layer == 5)
        *flags = layer;
    if ((pc->overlay_flags & 4) && layer == 5) {
        if (!pc->layer5_toggle) {
            QUAD_BUILDER(pc, quad, palette, layer, 0.0f, 0.0f);
            pc->layer5_toggle = 1;
        } else {
            pc->layer5_toggle = 0;
        }
    }

    if (effect_state_bit15(game_actor_get_effect_state((struct RenderObject *)pc)) == 1)
        hover = render_object_hover_height((struct RenderObject *)pc);
    pc->hover_height = (short)hover;
    hover_f = (float)(int)(short)hover;

    if ((pc->overlay_flags & 8) && layer <= 4 && !(pc->overlay_flags & 0x30000)) {
        if (++pc->pulse_phase > 0xb4)
            pc->pulse_phase = 0;
        for (pass = 0; pass < 2; pass++) {
            float sine = g_sinTable[pc->pulse_phase];

            k = pass == 0 ? game_camera_scale() * (sine + 3.0f)
                          : game_camera_scale() * (sine + sine + 5.0f);
            q.top = (float)rect[1] - k - hover_f;
            q.bottom = (float)rect[3] + k - hover_f;
            l = (float)rect[0] + screen_shift;
            r = (float)rect[2] + screen_shift;
            if (l > r) {
                l += k;
                r -= k;
            } else {
                l -= k;
                r += k;
            }
            q.x0 = l;
            q.x1 = r;
            if (pass == 0) {
                depth_a = a - 5.0e-5f;
                depth_b = b - 5.0e-5f;
            }
            q.depth_a = depth_a;
            q.depth_b = depth_b;
            q.image = (struct SprImg *)image;
            q.color = pass == 0 ? 0x64ffffff : 0x78ffffff;
            q.angle = (float)(clip->angle + pc->spr_angle);
            if (pass == 0)
                QUAD_BUILDER(pc, &q, palette, 5, 0.0f, 0.0f);
            else
                QUAD_BUILDER(pc, &q, palette, 5, 0.0f, 0.0f);
        }
    }

    if ((pc->overlay_flags & 0x80800) && !(pc->overlay_flags & 0x20000)) {
        float zoom = pc->zoom;

        for (pass = 1, step = 3; step <= 6; pass++, step += 3) {
            unsigned char alpha = pc->outline_alpha;
            float rhw_scale = (float)renderer_screen_scale(g_renderer, z);
            float s = (float)(session_get_baby_job_scale(&g_session1, game_actor_get_job((struct GameActor *)pc)) * rhw_scale
                              * (float)((float)pass * 0.01f + 1.0f));
            double scale_x = renderer_scale_x(g_renderer);
            float h = (float)(scale_x * ((float)pc->shift_x * pc->zoom) * s);
            float top = (float)((double)(dy + clip->y) * s * clip->zoomY * zoom + y);
            float bottom = (float)((double)((img[3] + 1) * img[1] + clip->y + dy) * s * clip->zoomY * zoom + y + 1.0);
            const struct Renderer *screen = g_renderer;
            double xl, xr;

            if (clip->flags & 1) {
                xl = (double)((img[2] + 1) * img[0] + clip->x + dx) * s * clip->zoomX;
                xr = (double)(dx + clip->x) * s * clip->zoomX;
                if (!(rect[1] < screen->height && rect[1] > 0 && rect[2] < screen->width && rect[0] > 0))
                    continue;
            } else {
                if (!(rect[1] < screen->height && rect[3] > 0 && rect[0] < screen->width && rect[2] > 0))
                    continue;
                xl = (double)(clip->x + dx) * s * clip->zoomX;
                xr = (double)((img[2] + 1) * img[0] + clip->x + dx) * s * clip->zoomX;
            }
            q.image = (struct SprImg *)image;
            q.x0 = (float)(zoom * xl + x) + h;
            q.x1 = (float)(zoom * xr + x + 1.0) + h;
            q.top = top - hover_f;
            q.bottom = ((float)rect[1] - top) * 0.5f + bottom - hover_f;
            q.depth_a = a + 1.0e-5f + (float)step * 5.0e-6f;
            q.depth_b = b + 1.0e-5f + (float)step * 5.0e-6f;
            if (pc->overlay_flags & 0x800)
                q.color = ((unsigned)alpha << 24) + 0x3737ff;
            else if (pc->overlay_flags & 0x80000)
                q.color = ((unsigned)alpha << 24) + 0x56405;
            q.angle = (float)(clip->angle + pc->spr_angle);
            QUAD_BUILDER(pc, &q, palette, 5, 0.0f, 0.0f);
        }
    }

    if (!(pc->overlay_flags & 0x30000)) {
        render_object_draw_echoes(object, clip, rect, screen_shift, a, b, palette, image, 4.0f, (short)hover);
        render_object_draw_rings(object, quad, clip, rect, screen_shift, a, b, palette, image, *flags, (short)hover);
    }
    render_object_jitter_quad(object, quad, rect, screen_shift, 0xb, 5);

    if (pc->overlay_flags & 0x10000) {
        unsigned short rise = pc->overlay_step;
        short fade = (short)(unsigned short)(0xff - rise * 20);

        k = game_camera_scale() * (float)((short)rise * 20);
        quad->top = (float)rect[1] - k;
        quad->bottom = (float)rect[3] - k;
        quad->color = ((unsigned)(int)(fade < 0 ? 0 : fade) << 24) + 0xffffff;
    }
    if (pc->overlay_flags & 0x20000) {
        short fade = (short)((pc->tint_fade_step - 10) * 17);

        k = game_camera_scale() * (float)((0x19 - pc->tint_fade_step) * 20);
        if (fade > 0xff)
            fade = 0xff;
        if (fade < 0)
            fade = 0;
        k = 0.0f > k ? 0.0f : k;
        quad->top = (float)rect[1] - k;
        quad->color = ((unsigned)(int)fade << 24) + 0xffffff;
        quad->bottom = (float)rect[3] - k;
    }
    if ((short)hover != 0) {
        quad->top -= hover_f;
        quad->bottom -= hover_f;
    }
    if (pc->overlay_flags & 0x100000) {
        if (layer == 0 || layer > 4) {
            quad->color = 0x1e373737;
            *flags = 5;
        }
        k = 60.0f * game_camera_scale();
        quad->top = (float)rect[1] + k;
        quad->bottom = (float)rect[3] + k;
    }
}
