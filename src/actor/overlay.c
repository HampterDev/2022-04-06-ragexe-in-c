#include "render/math3d.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/sprite_render.h"
#include "actor/game_actor.h"

#define QUAD_SUBMIT(self, quad, palette) render_object_submit_spr_quad((self), (quad), (palette), 5, 0.0f, 0.0f)

unsigned short render_object_hover_height(struct RenderObject *self)
{
    short angle;
    unsigned char rise;
    short height;
    int px;

    self->hover_angle += 2;
    if ((short)self->hover_angle >= 360)
        self->hover_angle -= 360;
    angle = (short)self->hover_angle;
    rise = self->hover_rise;
    if (self->state == 0 || self->state == 1 || (self->overlay_flags & 0x40000)) {
        if (rise < 20) {
            if (angle % 30 == 0)
                self->hover_rise = ++rise;
        } else if (rise > 20) {
            if (angle % 30 == 0)
                self->hover_rise = --rise;
        }
    } else if (self->state == 6) {
        if (rise < 100) {
            if (angle % 30 == 0)
                self->hover_rise = ++rise;
        } else if (rise > 100) {
            if (angle % 30 == 0)
                self->hover_rise = --rise;
        }
    } else if (rise) {
        self->hover_rise = --rise;
    }
    height = (short)(unsigned short)(int)(g_sinTable[angle] * 4.0f + (float)rise);
    px = (int)(game_camera_scale() * (float)height);
    return (short)px >= 0 ? (unsigned short)px : 0;
}

void render_object_draw_tint_quad(struct RenderObject *self, struct SqQuad *quad, short r,
                           short g, short b, void *palette)
{
    unsigned step = self->tint_fade_step;
    unsigned c;

    if (step <= 10) {
        quad->color = (((((step * 0xf00 + r) << 8) + g) << 8) + b);
    } else if (step <= 20) {
        c = r + 0xa000;
        quad->color = ((((c << 8) + g) << 8) + b);
    } else if (step <= 50) {
        c = ((unsigned)(int)(short)(5 * (unsigned short)(0x33 - step)) << 8) + r;
        quad->color = ((((c << 8) + g) << 8) + b);
    }
    if (self->tint_fade_step <= 50) {
        QUAD_SUBMIT(self, quad, palette);
        QUAD_SUBMIT(self, quad, palette);
    }
}

void render_object_draw_flashes(struct RenderObject *self, struct SqQuad quad, void *palette)
{
    int flags = self->overlay_flags;

    if (flags & 0x100) {
        short step = (short)self->overlay_step;
        short alpha;

        if (step <= 5) {
            alpha = step * 0x32;
            quad.color = ((unsigned)(int)alpha << 24) + 0xffffff;
        } else if (step <= 8) {
            quad.color = 0xffffffff;
        } else if (step <= 11) {
            alpha = 0x37f - step * 0x50;
            quad.color = ((unsigned)(int)alpha << 24) + 0xffffff;
        }
        if (step <= 11) {
            QUAD_SUBMIT(self, &quad, palette);
            flags = self->overlay_flags;
        }
    }
    if (flags & 0x4000) {
        render_object_draw_tint_quad(self, &quad, 5, 5, 0xff, palette);
        flags = self->overlay_flags;
    }
    if (flags & 0x40000)
        render_object_draw_tint_quad(self, &quad, 0xff, 5, 5, palette);
}

void render_object_apply_pulse_scale(struct RenderObject *self, float *scale)
{
    unsigned deg;

    if (self->overlay_flags & 0x10) {
        deg = self->swell_step * 4 + 90;
        if (deg < 90)
            deg = 90;
        if (deg < 270)
            *scale = ((g_sinTable[deg] + 1.0f) * 0.25f + 0.5f) * *scale;
        else
            *scale = *scale * 0.5f;
    }
    if (self->overlay_flags & 0x400) {
        deg = self->shrink_step * 4 + 90;
        if (deg < 90)
            deg = 90;
        if (deg < 270)
            *scale = (1.5f - (g_sinTable[deg] + 1.0f) * 0.25f) * *scale;
        else
            *scale = *scale * 1.5f;
    }
}

void render_object_swing_point(struct RenderObject *self, int kind, int *x, int *y, int *out_x,
                           int *out_y)
{
    int deg;
    float radius;

    if (!(self->overlay_flags & 0x40))
        return;
    deg = (int)(180.0f - g_sinTable[90 + (short)self->overlay_step * 5] * 180.0f);
    if (deg >= 360)
        deg -= 360;
    if (kind == 1) {
        radius = 40.0f;
        *x = (int)(g_sinTable[deg] * radius + (float)*x);
        *y = (int)((float)*y - (g_cosTable[deg] * radius - radius));
        *out_x = *x;
        *out_y = *y;
    } else if (kind >= 2) {
        radius = 10.0f;
        *x = (int)(g_sinTable[deg] * radius + (float)*x);
        *y = (int)((float)*y - (g_cosTable[deg] * radius - radius));
    }
}
