#include "contents/champion_monster.h"
#include "actor/render_ops.h"
#include "render/renderer.h"
#include "resources/palette.h"
#include "session/state.h"
#include <stdio.h>
#include <windows.h>
#include "contents/body_layer.h"
#include "actor/sprite_render.h"
#include "session/session_names.h"
#include "session/session_state.h"

struct Effect;

#define RS_RES(path) res_mgr_get(g_resMgr(), (path))

void render_object_render_sprite(struct RenderObject *self, const float *matrix,
                                           unsigned char look, float extra)
{
    struct ScreenBounds bounds;
    struct SqQuad quad, flash;
    struct SprRes *spr = self->spr;
    struct ActRes *act = self->act;
    struct Motion *motion;
    struct SprClip *clip;
    struct Effect *effect;
    void *image, *palette;
    char path[256];
    float view[3], sx, sy, rhw, screen_scale, screen_shift, k, size, depth_a, depth_b;
    int rect[4], flags, i, state;
    unsigned colour;
    unsigned char drew;

    if (g_session1.ownIllusion == 1 && look == 1) {
        SYSTEMTIME now;
        int job;

        GetLocalTime(&now);
        switch (now.wDay % 3) {
        case 0: job = 0x520; break;
        case 1: job = 0x56d; break;
        default: job = 0x4f8; break;
        }
        sprintf(path, monster_act_fmt, session_get_job_name(&g_session1, job, -1));
        act = (struct ActRes *)RS_RES(path);
        sprintf(path, monster_spr_fmt, session_get_job_name(&g_session1, job, -1));
        spr = (struct SprRes *)RS_RES(path);
    }
    motion = act_res_get_motion(act, self->action, self->motion);

    view[0] = self->pos.y * matrix[3] + self->pos.x * matrix[0] + matrix[6] * self->pos.z + matrix[9];
    view[1] = matrix[1] * self->pos.x + matrix[4] * self->pos.y + matrix[7] * self->pos.z + matrix[10];
    view[2] = matrix[2] * self->pos.x + matrix[5] * self->pos.y + matrix[8] * self->pos.z + matrix[11];
    renderer_project_point(g_renderer, &self->pos, matrix, &sx, &sy, &rhw);
    screen_scale = (float)renderer_screen_scale(g_renderer, rhw);

    k = extra;
    effect = effect_mgr_find_effect(g_effectMgr, self->aid, 0x946);
    if (effect) {
        extra = effect->params.scale + extra;
        k = extra;
    }
    if (champion_monster_is_champion(g_championMonster, self->monster_job))
        k = champion_monster_size(g_championMonster, self->monster_job) + k;
    if (k != 0.0f)
        screen_scale = k * screen_scale;

    self->scale = screen_scale / g_spriteScaleDivisor;
    size = (float)self->shift_x * self->zoom;
    size = (float)(renderer_scale_x(g_renderer) * size);
    screen_shift = size * self->scale;

    drew = 0;
    bounds.left = (float)g_renderer->width;
    bounds.top = (float)g_renderer->height;
    bounds.right = 0.0f;
    bounds.bottom = 0.0f;
    bounds.z = rhw;
    render_object_apply_pulse_scale(self, &screen_scale);

    for (i = 0; i < motion->clip_count; i++) {
        clip = motion_get_clip(motion, i);
        if (clip->sprIndex == -1)
            continue;
        image = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
        if (!image) continue;
        if (!render_object_clip_to_quad(self, i, i, screen_scale, clip, sx, sy, view, screen_shift, &depth_a, &depth_b,
                                                  image, &quad, rect, &colour, 0, 0, 0))
            continue;
        quad.image = image;
        drew = 1;
        if (self->pickable) {
            float lo = quad.x0 < quad.x1 ? quad.x0 : quad.x1;
            float hi = quad.x0 > quad.x1 ? quad.x0 : quad.x1;

            if (bounds.left > lo)
                bounds.left = lo;
            if (bounds.top > quad.top)
                bounds.top = quad.top;
            if (hi > bounds.right)
                bounds.right = hi;
            if (quad.bottom > bounds.bottom)
                bounds.bottom = quad.bottom;
        }
        palette = spr->palette;
        if (self->body_palette)
            palette = palette_resource_colors(RS_RES(string_cstr(&self->body_pal_name)), spr->palette);
        flags = 0x101;
        render_object_draw_clip_overlays(self, &quad, clip, rect, screen_shift, depth_a, depth_b, palette, image, &flags, colour);
        if (self->monster_job == 0x235) {
            quad.depth_a = depth_a - 3.0e-7f;
            quad.depth_b = depth_b - 3.0e-7f;
        }
        if (session_is_screen_flipped(&g_session1)) {
            quad.angle = 180.0f;
            QUAD_BUILDER(self, &quad, palette, flags, (sx - quad.x1) + (sx - quad.x0),
                         (sy - quad.bottom) + (sy - quad.top));
        } else {
            QUAD_BUILDER(self, &quad, palette, flags, 0.0f, 0.0f);
        }

        flash = quad;
        state = self->overlay_flags;
        if (state & 0x100) {
            short step = (short)self->overlay_step;
            short alpha;

            if (step <= 5) {
                alpha = step * 0x32;
                flash.color = ((unsigned)(int)alpha << 24) + 0xffffff;
            } else if (step <= 8) {
                flash.color = 0xffffffff;
            } else if (step <= 11) {
                alpha = 0x37f - step * 0x50;
                flash.color = ((unsigned)(int)alpha << 24) + 0xffffff;
            }
            if (step <= 11) {
                QUAD_BUILDER(self, &flash, palette, 5, 0.0f, 0.0f);
                state = self->overlay_flags;
            }
        }
        if (state & 0x4000) {
            render_object_draw_tint_quad(self, (struct SqQuad *)&flash, 5, 5, 0xff, palette);
            state = self->overlay_flags;
        }
        if (state & 0x40000)
            render_object_draw_tint_quad(self, (struct SqQuad *)&flash, 0xff, 5, 5, palette);
    }
    if (drew)
        render_object_ops_of(self)->render_body(self, &bounds, sx, sy);
    else
        render_object_set_screen_position(self, sx, sy);
    self->drawn = 1;
}
