#include "actor/render_ops.h"
#include "render/projection.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "session/state.h"
#include "world/actor_lookup.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/actor_ops.h"
#include "actor/game_actor.h"

#define CLIP_MIRRORED 1
#define SPR_PALETTE(spr) (((struct SprRes *)(spr))->palette)
#define RENDERER_WIDTH(r) ((r)->width)
#define RENDERER_HEIGHT(r) ((r)->height)

struct SpriteQuad {
    float top, left, bottom, right;
    float rhw_bottom, rhw_top;
    unsigned argb;
    float angle;
    struct SprImg *image;
};
OFFSET_ASSERT(SpriteQuad, argb, 0x18);
#define SPRITE_QUAD_BLENDED 0x101
#define SPRITE_QUAD_ADDITIVE 5

typedef void(*SpriteQuadFn)(struct RenderObject *self, const struct SpriteQuad *quad,
                           const void *palette, int flags, float dx, float dy);
#define sprite_quad_submit ((SpriteQuadFn)render_object_submit_spr_quad)

#define BLEND_ALPHA 1

void prim_camera_square(struct EffectPrim *self, float size, Vec3 q[4])
{
    q[0].x = g_cosTable[45] * size;
    q[0].y = 0.0f;
    q[0].z = g_sinTable[45] * size;
    q[1].x = g_cosTable[135] * size;
    q[1].y = 0.0f;
    q[1].z = g_sinTable[135] * size;
    q[2].x = g_cosTable[225] * size;
    q[2].y = 0.0f;
    q[2].z = g_sinTable[225] * size;
    q[3].x = g_cosTable[315] * size;
    q[3].y = 0.0f;
    q[3].z = g_sinTable[315] * size;
    prim_quad_turn(&q[0], &q[1], &q[2], &q[3], self->camera_yaw, self->camera_pitch);
}

static void square_at(const Vec3 q[4], const Vec3 *at, Vec3 out[4])
{
    int k;

    for (k = 0; k < 4; k++) {
        out[k].x = at->x + q[k].x;
        out[k].y = at->y + q[k].y;
        out[k].z = at->z + q[k].z;
    }
}

unsigned char prim_crossfade_square_process(struct EffectPrim *self)
{
    struct PrimSpinSquare *sq = self->records.spin_squares;
    int i;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++)
        if (sq[i].active == 1)
            sq[i].frame++;
    return 1;
}

void prim_crossfade_square_render(struct EffectPrim *self, const float *view)
{
    struct PrimSpinSquare *sq = self->records.spin_squares;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, sq++) {
        int twice;
        unsigned short alpha;

        if (sq->active != 1)
            continue;
        prim_spin_square_corners(sq);
        twice = sq->frame % 256 * 2;
        alpha = (short)twice <= 255 ? (unsigned short)twice : (unsigned short)(511 - twice);
        prim_draw_quad(self, &count, (short)alpha, sq->corners[2], sq->corners[0], sq->corners[1], sq->corners[3],
                       view, BLEND_ALPHA, 0, 0, 0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
        prim_draw_quad(self, &count, (short)(255 - alpha), sq->corners[2], sq->corners[0], sq->corners[1],
                       sq->corners[3], view, BLEND_ALPHA, 1, 0, 0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f,
                       PRIM_QUAD_PROJECTED);
    }
}

void prim_styled_ring_process(struct EffectPrim *self)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    int i, j;

    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, ring++) {
        if (ring->active != 1)
            continue;
        if (++ring->frame <= 0)
            continue;
        ring->arc_start += 3;
        if (ring->arc_start >= 360)
            ring->arc_start = (short)((unsigned short)ring->arc_start - 360);
        if (ring->frame < 30) {
            ring->alpha += 3;
            if ((short)ring->alpha > 100)
                ring->alpha = 100;
        }
        if (ring->frame > self->duration - 46) {
            ring->alpha += 0xfffe;
            if ((short)ring->alpha < 0)
                ring->alpha = 0;
        }
        for (j = 0; j < WAVE_RING_POINTS; j++) {
            int phase = i * 45 < 90 ? ring->frame + i * 90 : (ring->frame + i * 45) * 2;
            float base = ring->base_height;
            ring->heights[j] = g_sinTable[j * 9] * base * 0.3f * g_sinTable[phase % 360] + base;
        }
    }
}

void prim_styled_ring_render(struct EffectPrim *self, const float *view)
{
    struct PrimWaveRing *ring = self->records.wave_rings;
    Vec3 low = {0.0f, 0.0f, 0.0f}, high = {0.0f, 0.0f, 0.0f};
    Vec3 prev_low = {0.0f, 0.0f, 0.0f}, prev_high = {0.0f, 0.0f, 0.0f};
    unsigned short bright = (unsigned short)(size_t)view;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, ring++) {
        int step, along, point;

        if (ring->active != 1)
            continue;
        step = ring->arc / 20;
        point = 0;
        if (ring->arc < 0)
            continue;
        low.y = 0.0f;
        for (along = 0; along <= ring->arc; along += step) {
            int deg = (ring->arc_start + along) % 360;
            float cos_d, sin_d, radius, height, out;

            if ((unsigned short)ring->arc == 360 && point == 20)
                deg = ring->arc_start;
            cos_d = g_cosTable[deg];
            sin_d = g_sinTable[deg];
            radius = g_sinTable[ring->arc_start] + ring->radius;
            height = ring->heights[point];
            out = g_cosTable[ring->tilt] * height;
            low.x = radius * cos_d;
            low.z = radius * sin_d;
            high.x = out * cos_d + low.x;
            high.z = out * sin_d + low.z;
            high.y = -(g_sinTable[ring->tilt] * height);

            if (point > 0) {
                unsigned short style = (unsigned short)ring->style;
                char texture;

                if (style == 10) {
                    texture = (char)(ring->frame / 3 % self->motion_count);
                } else {
                    texture = (char)(ring->frame % self->motion_count);
                    self->scale = 0.0f;
                    prim_draw_strip(self, &count, point - 1, (short)ring->alpha, prev_low, low, high,
                                    prev_high, view, texture);
                }
                switch ((short)style) {
                case 3:
                    self->scale = 18.0f;
                    bright = (unsigned short)(ring->alpha * 3);
                    break;
                case 4:
                    self->scale = 19.0f;
                    bright = (unsigned short)(ring->alpha * 2);
                    break;
                case 5:
                    self->scale = 20.0f;
                    bright = (unsigned short)(ring->alpha * 2);
                    break;
                case 6:
                    self->scale = 5.0f;
                    bright = (unsigned short)(ring->alpha * 2);
                    break;
                case 7:
                    self->scale = 21.0f;
                    bright = (unsigned short)(ring->alpha * 2);
                    break;
                case 8:
                    self->scale = 22.0f;
                    bright = (unsigned short)(ring->alpha * 2);
                    break;
                case 9:
                case 11:
                    self->scale = (short)style == 9 ? 30.0f : 32.0f;
                    bright = g_session1.map_rule_flags[0] == 1 ? ring->alpha : (unsigned short)(ring->alpha * 2);
                    break;
                case 10:
                    self->scale = 6.0f;
                    bright = (unsigned short)(ring->alpha * 3);
                    break;
                default:
                    if ((short)style <= 2) {
                        self->scale = 17.0f;
                        bright = (unsigned short)(ring->alpha * 3);
                    }
                    break;
                }
                if ((short)bright > 255)
                    bright = 255;
                prim_draw_strip(self, &count, point - 1, (short)bright, prev_low, low, high,
                                prev_high, view, texture);
            }
            if (++point >= 21)
                point = 20;
            prev_low = low;
            prev_high = high;
        }
    }
}

static void rising_mote(Vec3 *mote, const struct PrimRisingMotes *rec, int drift_x, int drift_z, float sign_x,
                        float sign_z, float size, int narrow)
{
    if (0.0f > mote->y) {
        mote->x = sign_x * (g_sinTable[(int)rec->drift[drift_x][0]] * 0.01f) + mote->x;
        mote->z = sign_z * (g_sinTable[(int)rec->drift[drift_z][0]] * 0.01f) + mote->z;
        if (size * -8.0f > mote->y) {
            float x, y, z, yaw_cos, yaw_sin;

            x = (float)(narrow ? prim_rand_below(25) - 12 : prim_rand_below(45) - 22);
            y = (float)prim_rand_below(30);
            z = (float)(prim_rand_below(11) - 5);
            x = x * size;
            mote->y = y * size;
            z = z * size;
            yaw_cos = g_cosTable[rec->yaw];
            yaw_sin = g_sinTable[rec->yaw];
            mote->z = z * yaw_cos + yaw_sin * x;
            mote->x = yaw_cos * x - z * yaw_sin;
        }
    }
    mote->y = mote->y - rec->rise;
}

void prim_rising_motes_process(struct EffectPrim *self)
{
    struct PrimRisingMotes *rec = self->records.rising_motes;
    int i;

    prim_face_camera(self);
    for (i = 0; i < 4; i++, rec++) {
        float size;

        if (rec->active != 1)
            continue;
        size = (float)rec->size10 / 10.0f;
        prim_drift_angles(rec->drift, 8);
        rising_mote(&rec->motes[2], rec, 0, 1, 1.0f, 1.0f, size, rec->tint[2] != 0);
        rising_mote(&rec->motes[0], rec, 2, 3, -1.0f, -1.0f, size, 0);
        rising_mote(&rec->motes[3], rec, 4, 5, 1.0f, -1.0f, size, 0);
        rising_mote(&rec->motes[1], rec, 6, 7, -1.0f, 1.0f, size, 0);
    }
}

void prim_rising_motes_render(struct EffectPrim *self, const float *view)
{
    static const unsigned char order[4] = {2, 0, 3, 1};
    struct PrimRisingMotes *rec = self->records.rising_motes;
    int count = 0;
    int i, k;

    for (i = 0; i < 4; i++, rec++) {
        unsigned char r, g, b;
        Vec3 q[4];

        if (rec->active != 1)
            continue;
        r = (unsigned char)(unsigned short)(int)((float)rec->tint[0] * 2.57f);
        g = (unsigned char)(unsigned short)(int)((float)rec->tint[1] * 2.57f);
        b = (unsigned char)(unsigned short)(int)((float)rec->tint[2] * 2.57f);
        prim_camera_square(self, rec->size, q);
        for (k = 0; k < 4; k++) {
            Vec3 corner[4];
            float y;
            int alpha = 50;

            square_at(q, &rec->motes[order[k]], corner);
            y = corner[0].y;
            if (-4.0f > y)
                alpha = (int)((y + 4.0f) * 15.0f + 50.0f);
            if (y > -0.5f)
                alpha = (int)(y * -100.0f);
            if (0.0f >= y)
                prim_draw_quad(self, &count, (short)(alpha >= 0 ? (unsigned short)alpha : 0), corner[0], corner[1],
                               corner[2], corner[3], view, PRIM_BLEND_ADDITIVE, 0, 0, r, g, b, 0.0f, 1.0f, 0.0f, 1.0f,
                               PRIM_QUAD_PROJECTED);
        }
    }
}

unsigned char prim_orbit_launcher_process(struct EffectPrim *self)
{
    struct PrimOrbitLauncher *rec = self->records.orbit_launchers;
    int i;

    prim_face_camera(self);
    self->base.pos = self->master->base.pos;
    for (i = 0; i < 4; i++, rec++) {
        int step;
        float reach;

        if (rec->active != 1)
            continue;
        if (++rec->frame <= 0)
            continue;
        if (rec->frame <= 10) {
            rec->alpha = rec->alpha + 20.0f;
        } else if (rec->frame > ((signed char)rec->style >= 1 ? 90 : 270)) {
            rec->alpha = rec->alpha - 5.0f;
            if (0.0f > rec->alpha)
                rec->alpha = 0.0f;
        }
        rec->radius = rec->radius + 0.05f;
        step = rec->frame % 60;
        if (rec->style == 1)
            step = 60 - step;
        reach = (float)(int)rec->radius;
        rec->pos.y = -25.0f;
        rec->pos.x = g_sinTable[step * 6] * reach;
        rec->pos.z = g_cosTable[step * 6] * reach;
    }
    return 1;
}

void prim_orbit_launcher_render(struct EffectPrim *self)
{
    struct PrimOrbitLauncher *rec = self->records.orbit_launchers;
    int i;

    for (i = 0; i < 4; i++, rec++) {
        Vec3 q[4], at;
        float own_aid;
        void *actor;
        struct RagEffect *effect;
        int effect_id;

        if (rec->active != 1)
            continue;
        prim_camera_square(self, rec->size, q);
        at = rec->pos;
        if (!(0.0f >= at.y + q[0].y) || rec->frame >= 300 || (int)rec->alpha <= 0)
            continue;
        own_aid = (float)(unsigned)g_session1.ownAid;
        effect_id = rec->style == 0 ? 850 : 932;
        actor = own_aid == rec->owner
            ? (void *)game_current_player()
            : (void *)game_find_actor((int)(__int64)rec->owner);
        if (!actor)
            continue;
        {
            Vec3 zero = {0.0f, 0.0f, 0.0f};
            effect = game_actor_launch_effect((struct GameActor *)(struct RenderObject *)actor, effect_id, zero, 0);
        }
        if (effect)
            rag_effect_send_msg(effect, 0, RAG_MSG_SET_OFFSETS, (RenderMsgArg)(int)&at, 0, 0);
    }
}

unsigned char prim_sprite_ghosts_process(struct EffectPrim *self)
{
    struct PrimSpriteGhosts *rec = self->records.sprite_ghosts;
    int i, k;

    for (i = 0; i < 4; i++, rec++) {
        float speed;

        if (rec->active != 1)
            continue;
        rec->frame++;
        if (rec->spin_b >= 360)
            rec->spin_b = (short)((unsigned short)rec->spin_b - 360);
        if (rec->spin_a >= 360)
            rec->spin_a = (short)((unsigned short)rec->spin_a - 360);
        if (rec->frame < 30) {
            if (rec->style == 2) {
                self->alpha = self->alpha + 3.0f;
                if (self->alpha > 100.0f)
                    self->alpha = 100.0f;
            } else {
                self->alpha = self->alpha + 1.0f;
                if (self->alpha > 10.0f)
                    self->alpha = 10.0f;
            }
        } else if (rec->frame > self->duration - 30) {
            self->alpha = self->alpha - 3.0f;
            if (0.0f > self->alpha)
                self->alpha = 0.0f;
        }
        for (k = 0; k < 6; k++) {
            float cur = rec->drift[k];

            if (rec->target[k] > cur) {
                cur = cur + 2.0f;
                rec->drift[k] = cur;
                if (cur >= rec->target[k])
                    rec->target[k] = (float)(rand() % 360);
                if (!(rec->drift[k] < 360.0f))
                    rec->drift[k] = rec->drift[k] - 360.0f;
            } else {
                cur = cur - 2.0f;
                rec->drift[k] = cur;
                if (rec->target[k] >= cur)
                    rec->target[k] = (float)(rand() % 360);
                if (0.0f > rec->drift[k])
                    rec->drift[k] = rec->drift[k] + 360.0f;
            }
        }
        speed = rec->speed;
        rec->offsets[2].x = g_cosTable[(int)rec->drift[0]] * speed + rec->offsets[2].x;
        rec->offsets[2].y = g_sinTable[(int)rec->drift[1]] * speed + rec->offsets[2].y;
        rec->offsets[0].x = rec->offsets[0].x - g_cosTable[(int)rec->drift[2]] * speed;
        rec->offsets[0].y = g_sinTable[(int)rec->drift[3]] * speed + rec->offsets[0].y;
        rec->offsets[3].x = rec->offsets[3].x - g_cosTable[(int)rec->drift[4]] * speed;
        rec->offsets[3].y = rec->offsets[3].y - g_sinTable[(int)rec->drift[5]] * speed;
    }
    prim_update_alpha(self);
    prim_shift_trail(self);
    return prim_step_motion(self) != 0;
}

struct ScreenBox {
    float top, bottom, left, right;
};
static int clip_screen_box(const struct SprClip *clip, const struct SprImg *img, float scale, float zoom,
                           float sx, float sy, struct ScreenBox *box)
{
    double scale_d = scale, zoom_d = zoom, zoom_y = clip->zoomY, zoom_x;
    int far_y = (img->heightHalved + 1) * img->height + clip->y;
    int far_x;

    box->top = (float)((double)clip->y * scale_d * zoom_y * zoom_d + (double)sy);
    box->bottom = (float)((double)far_y * scale_d * zoom_y * zoom_d + (double)sy);
    if (box->top >= (float)RENDERER_HEIGHT(g_renderer))
        return 0;
    if (0.0f >= box->bottom)
        return 0;
    far_x = (img->widthHalved + 1) * img->width + clip->x;
    zoom_x = clip->zoomX;
    if (clip->flags & CLIP_MIRRORED) {
        box->left = (float)((double)far_x * scale_d * zoom_x * zoom_d + (double)sx);
        box->right = (float)((double)clip->x * scale_d * zoom_x * zoom_d + (double)sx);
        if (box->right >= (float)RENDERER_WIDTH(g_renderer))
            return 0;
        return !(0.0f >= box->left);
    }
    box->left = (float)((double)clip->x * scale_d * zoom_x * zoom_d + (double)sx);
    box->right = (float)((double)far_x * scale_d * zoom_x * zoom_d + (double)sx);
    if (box->left >= (float)RENDERER_WIDTH(g_renderer))
        return 0;
    if (0.0f >= box->right)
        return 0;
    return 1;
}

static float edge_rhw(float edge, float sy, float depth)
{
    return 1.0f / ((edge - sy) * g_spriteRowDepthStep + depth);
}

void prim_sprite_ghosts_render(struct EffectPrim *self, const float *view)
{
    static const unsigned char copy[3] = {2, 0, 3};
    void *spr = render_object_get_spr_res(&self->base);
    void *act = render_object_get_act_res(&self->base);
    unsigned short alpha = (unsigned short)(int)self->alpha;
    struct PrimSpriteGhosts *rec = self->records.sprite_ghosts;
    struct SpriteQuad q;
    int i, part, ghost, n;

    for (i = 0; i < 4; i++, rec++) {
        if (rec->active <= 0 || !(self->alpha > 0.0f))
            continue;
        for (part = 0; part < self->particle_count; part++) {
            const Vec3 *pos = &self->particles[part].pos;
            float depth, sx, sy, rhw, zoom;
            struct Motion *motion;

            depth = pos->y * view[5] + pos->x * view[2] + pos->z * view[8] + view[11];
            renderer_project_point(g_renderer, pos, view, &sx, &sy, &rhw);
            zoom = renderer_screen_scale(g_renderer, rhw);
            motion = act_res_get_motion(act, self->base.action, self->base.motion);
            for (n = 0; n < motion->clip_count; n++) {
                struct SprClip *clip = motion_get_clip(motion, n);
                struct SprImg *img;
                struct ScreenBox box;
                unsigned halo;

                if (clip->sprIndex == -1)
                    continue;
                img = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
                if (!img) continue;
                if (!clip_screen_box(clip, img, self->particles[part].scale, zoom, sx, sy, &box))
                    continue;
                halo = ((unsigned)(int)(short)alpha << 24) + (rec->style == 2 ? 0x0aff0a : 0x64ff64);
                for (ghost = 0; ghost < 3; ghost++) {
                    const Vec3 *off = &rec->offsets[copy[ghost]];
                    int ghost_alpha = ghost == 1 ? (short)alpha / 2 : (short)alpha;

                    q.image = img;
                    q.top = off->y + box.top;
                    q.bottom = off->y + box.bottom;
                    q.left = off->x + box.left;
                    q.right = off->x + box.right;
                    q.rhw_bottom = edge_rhw(box.bottom, sy, depth) + 0.0001f;
                    q.rhw_top = edge_rhw(box.top, sy, depth) + 0.0001f;
                    q.argb = ((((((unsigned)ghost_alpha << 8) + clip->color[0]) << 8) + clip->color[1]) << 8) + clip->color[2];
                    q.angle = (float)clip->angle + self->angle[2];
                    sprite_quad_submit(&self->base, &q, SPR_PALETTE(spr), SPRITE_QUAD_BLENDED, 0.0f, 0.0f);

                    q.image = img;
                    q.top = off->y + (box.top - 2.0f);
                    q.bottom = off->y + (box.bottom + 2.0f);
                    q.left = off->x + (box.left - 2.0f);
                    q.right = off->x + (box.right + 2.0f);
                    q.rhw_bottom = edge_rhw(box.bottom, sy, depth) + 0.0002f;
                    q.rhw_top = edge_rhw(box.top, sy, depth) + 0.0002f;
                    q.argb = halo;
                    q.angle = (float)clip->angle + self->angle[2];
                    sprite_quad_submit(&self->base, &q, SPR_PALETTE(spr), SPRITE_QUAD_ADDITIVE, 0.0f, 0.0f);
                }
            }
        }
    }
}

unsigned char prim_sprite_halo_process(struct EffectPrim *self)
{
    struct PrimSpriteHalo *rec = self->records.sprite_halos;
    int i;

    for (i = 0; i < 4; i++, rec++) {
        if (rec->active != 1)
            continue;
        if (++rec->frame < 50) {
            self->alpha = self->alpha + 5.0f;
            if (self->alpha > 150.0f)
                self->alpha = 150.0f;
        } else if (rec->frame > self->duration - 50) {
            self->alpha = self->alpha - 5.0f;
            if (0.0f > self->alpha)
                self->alpha = 0.0f;
        }
    }
    prim_update_alpha(self);
    prim_shift_trail(self);
    return prim_step_motion(self) != 0;
}

void prim_sprite_halo_render(struct EffectPrim *self, const float *view)
{
    void *spr = render_object_get_spr_res(&self->base);
    void *act = render_object_get_act_res(&self->base);
    unsigned short alpha = (unsigned short)(int)self->alpha;
    struct PrimSpriteHalo *rec = self->records.sprite_halos;
    int i, part, n;

    for (i = 0; i < 4; i++, rec++) {
        if (rec->active <= 0 || !(self->alpha > 0.0f))
            continue;
        for (part = 0; part < self->particle_count; part++) {
            const Vec3 *pos = &self->particles[part].pos;
            float depth, sx, sy, rhw, zoom;
            struct Motion *motion;

            depth = pos->y * view[5] + pos->x * view[2] + pos->z * view[8] + view[11];
            renderer_project_point(g_renderer, pos, view, &sx, &sy, &rhw);
            zoom = renderer_screen_scale(g_renderer, rhw);
            motion = act_res_get_motion(act, self->base.action, self->base.motion);
            for (n = 0; n < motion->clip_count; n++) {
                struct SprClip *clip = motion_get_clip(motion, n);
                struct SprImg *img;
                struct ScreenBox box;
                struct SpriteQuad q, outline;
                unsigned char r, g, b;
                unsigned argb;

                if (clip->sprIndex == -1)
                    continue;
                img = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
                if (!img) continue;
                if (!clip_screen_box(clip, img, self->particles[part].scale, zoom, sx, sy, &box))
                    continue;
                q.top = box.top;
                q.left = box.left;
                q.bottom = box.bottom;
                q.right = box.right;
                q.rhw_bottom = edge_rhw(box.bottom, sy, depth);
                q.rhw_top = edge_rhw(box.top, sy, depth);
                q.angle = (float)clip->angle + self->angle[2];
                q.image = img;
                r = clip->color[0];
                g = clip->color[1];
                b = clip->color[2];
                prim_master_color(self, &r, &g, &b);
                argb = ((((((unsigned)(int)(short)alpha << 8) + r) << 8) + g) << 8) + b;
                q.argb = argb;
                sprite_quad_submit(&self->base, &q, SPR_PALETTE(spr),
                           rec->additive == 1 ? SPRITE_QUAD_ADDITIVE : SPRITE_QUAD_BLENDED, 0.0f, 0.0f);

                outline.image = img;
                outline.argb = argb;
                outline.rhw_bottom = edge_rhw(box.bottom, sy, depth) + 0.0001f;
                outline.rhw_top = edge_rhw(box.top, sy, depth) + 0.0001f;
                outline.top = box.top - 1.0f;
                outline.left = box.left - 1.0f;
                outline.bottom = box.bottom + 1.0f;
                outline.right = box.right + 1.0f;
                outline.angle = (float)clip->angle + self->angle[2];
                sprite_quad_submit(&self->base, &outline, SPR_PALETTE(spr), SPRITE_QUAD_ADDITIVE, 0.0f, 0.0f);
            }
        }
    }
}

void prim_wedge_pair_process(struct EffectPrim *self)
{
    struct PrimWedgePair *pair = self->records.wedge_pairs;
    int i;

    prim_face_camera(self);
    for (i = 0; i < 4; i++, pair++) {
        int age_before;

        if (pair->active != 1)
            continue;
        age_before = pair->frame++;
        if (age_before < 15) {
            pair->alpha += 13;
            if (pair->alpha > 255)
                pair->alpha = 255;
        } else if (pair->frame > 20) {
            pair->alpha += -3;
            if (pair->alpha < 0) {
                pair->active = 0;
                pair->alpha = 0;
            }
        }
    }
}

static void wedge(struct EffectPrim *self, const struct PrimWedgePair *pair, const float *view, int *count, int deg)
{
    int back = deg - 90;
    float tip_x = g_cosTable[deg] * pair->reach, tip_z = g_sinTable[deg] * pair->reach;
    Vec3 centre = {0.0f, -6.0f, 0.0f}, edge, tip;

    if (back < 0)
        back = deg + 270;
    edge.x = g_cosTable[back] * pair->side + tip_x;
    edge.y = -6.0f;
    edge.z = g_sinTable[back] * pair->side + tip_z;
    tip.x = tip_x;
    tip.y = -6.0f;
    tip.z = tip_z;
    if (pair->style == 1) {
        prim_draw_quad(self, count, (short)(pair->alpha / 4), centre, edge, tip, centre, view, BLEND_ALPHA, 0, 0,
                       0xff, 0x59, 0xb6, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
        prim_draw_quad(self, count, pair->alpha, centre, edge, tip, centre, view, PRIM_BLEND_ADDITIVE, 0, 0, 0xff,
                       0x19, 0x19, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    } else if (pair->style == 2) {
        prim_draw_quad(self, count, (short)(pair->alpha / 4), centre, edge, tip, centre, view, BLEND_ALPHA, 0, 0,
                       0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
        prim_draw_quad(self, count, pair->alpha, centre, edge, tip, centre, view, PRIM_BLEND_ADDITIVE, 0, 0, 0xff,
                       0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    } else {
        prim_draw_quad(self, count, (short)(pair->alpha / 4), centre, edge, tip, centre, view, BLEND_ALPHA, 0, 0,
                       0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
        prim_draw_quad(self, count, pair->alpha, centre, edge, tip, centre, view, PRIM_BLEND_ADDITIVE, 0, 0, 0x19,
                       0x19, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    }
}

void prim_wedge_pair_render(struct EffectPrim *self, const float *view)
{
    struct PrimWedgePair *pair = self->records.wedge_pairs;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, pair++) {
        if (pair->active != 1 || pair->alpha <= 0)
            continue;
        wedge(self, pair, view, &count, pair->angle >= 25 ? pair->angle - 25 : pair->angle + 335);
        wedge(self, pair, view, &count, pair->angle < 335 ? pair->angle + 25 : pair->angle - 335);
    }
}

static int steer(float *cur, float target, float step)
{
    float diff = *cur - target;

    if (diff < 180.0f && diff >= 0.0f) {
        *cur = *cur - step;
        return target >= *cur;
    }
    if (diff < -180.0f) {
        *cur = *cur - step;
        return target - 360.0f >= *cur;
    }
    *cur = *cur + step;
    return *cur >= target;
}

static void wrap_degrees(float *v)
{
    if (0.0f > *v)
        *v = *v + 360.0f;
    if (!(*v < 360.0f))
        *v = *v - 360.0f;
}

unsigned char prim_twin_wisps_process(struct EffectPrim *self)
{
    struct PrimTwinWisps *rec = self->records.twin_wisps;
    int i, k;

    prim_face_camera(self);
    for (i = 0; i < 4; i++, rec++) {
        float speed;

        if (rec->active != 1)
            continue;
        rec->frame++;
        for (k = 0; k < 2; k++) {
            struct PrimWispCourse *course = &rec->course[k];

            if (steer(&course->yaw, course->yaw_target, 2.0f))
                course->yaw_target = course->base_yaw - 45.0f + (float)(rand() % 91);
            if (steer(&course->pitch, course->pitch_target, 1.0f))
                course->pitch_target = (float)(rand() % 11 + 175);
        }
        for (k = 0; k < 2; k++) {
            wrap_degrees(&rec->course[k].base_yaw);
            wrap_degrees(&rec->course[k].yaw_target);
            wrap_degrees(&rec->course[k].yaw);
            wrap_degrees(&rec->course[k].pitch_target);
            wrap_degrees(&rec->course[k].pitch);
        }
        for (k = 0; k < 2; k++) {
            if (rec->frame < 10) {
                rec->alpha[k] = rec->alpha[k] + 25.0f;
                if (!(rec->alpha[k] < 255.0f))
                    rec->alpha[k] = 255.0f;
            } else if (rec->frame > 20) {
                rec->alpha[k] = rec->alpha[k] - 3.0f;
                if (0.0f > rec->alpha[k]) {
                    rec->active = 0;
                    rec->alpha[k] = 0.0f;
                }
            }
        }
        speed = rec->speed;
        rec->pos0.x = g_cosTable[(int)rec->course[0].yaw] * speed + rec->pos0.x;
        rec->pos0.z = g_sinTable[(int)rec->course[0].yaw] * speed + rec->pos0.z;
        rec->pos0.y = rec->pos0.y - g_sinTable[(int)rec->course[0].pitch] * speed;
        rec->pos1.x = g_cosTable[(int)rec->course[1].yaw] * speed + rec->pos1.x;
        rec->pos1.z = g_sinTable[(int)rec->course[1].yaw] * speed + rec->pos1.z;
        rec->pos1.y = g_sinTable[(int)rec->course[1].pitch] * speed + rec->pos1.y;
    }
    return 1;
}

static void wisp(struct EffectPrim *self, const Vec3 corner[4], float alpha_f, unsigned char style, const float *view,
                 int *count)
{
    int alpha = (int)alpha_f;

    if (alpha < 0)
        alpha = 0;
    if (style == 2 || style == 3) {
        prim_draw_quad(self, count, (short)alpha, corner[1], corner[0], corner[3], corner[2], view,
                       PRIM_BLEND_ADDITIVE, 0, 0, 0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
        return;
    }
    prim_draw_quad(self, count, (short)(alpha / 3), corner[0], corner[1], corner[2], corner[3], view, BLEND_ALPHA, 0,
                   0, 0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    if (style == 1)
        prim_draw_quad(self, count, (short)alpha, corner[0], corner[1], corner[2], corner[3], view,
                       PRIM_BLEND_ADDITIVE, 0, 0, 0xff, 0x32, 0x32, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
    else
        prim_draw_quad(self, count, (short)alpha, corner[0], corner[1], corner[2], corner[3], view,
                       PRIM_BLEND_ADDITIVE, 0, 0, 0x32, 0x32, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_IN_FRONT);
}

void prim_twin_wisps_render(struct EffectPrim *self, const float *view)
{
    struct PrimTwinWisps *rec = self->records.twin_wisps;
    int count = 0;
    int i;

    for (i = 0; i < 4; i++, rec++) {
        Vec3 q[4], corner[4];

        if (rec->active != 1)
            continue;
        prim_camera_square(self, rec->size, q);
        square_at(q, &rec->pos0, corner);
        wisp(self, corner, rec->alpha[0], rec->style, view, &count);
        square_at(q, &rec->pos1, corner);
        if (rec->style != 3)
            wisp(self, corner, rec->alpha[1], rec->style, view, &count);
    }
}
