#include "render/projection.h"
#include "render/math3d.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "world/world_imports.h"
#include "actor/game_actor.h"

#define COS(deg) g_cosTable[deg]
#define SIN(deg) g_sinTable[deg]

static int rand_below(int n)
{
    int roll = rand() % n;
    return roll < 0 ? 0 : roll;
}

static void follow_master(struct EffectPrim *self)
{
    self->base.pos = self->master->base.pos;
}

#define CLIP_MIRRORED 1

#define EFFECT_DIGITAL_SPACE 1240
#define SPR_PALETTE(spr) (((struct SprRes *)(spr))->palette)
#define OBJ_SPR(o) ((o)->base.spr)
#define OBJ_SHIFT_X(o) ((o)->shift_x)

struct SprScreenQuad {
    float top;
    float left;
    float bottom;
    float right;
    float rhw_bottom;
    float rhw_top;
    unsigned argb;
    float angle;
    struct SprImg *image;
};

typedef void(*SprQuadFn)(struct RenderObject *self, struct SprScreenQuad *q,
                                    void *palette, int list_flags, float dx, float dy);
#define sprite_quad_submit ((SprQuadFn)render_object_submit_spr_quad)

void prim_ribbon_process(struct EffectPrim *self)
{
    int i;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRibbon *rb = &self->records.ribbons[i];
        if (rb->active != 1)
            continue;
        if (++rb->frame <= 0)
            continue;
        rb->texture = rb->texture + 1.0f;
        if (rb->texture >= (float)self->motion_count)
            rb->texture = 0.0f;
        rb->scroll = rb->scroll_speed + rb->scroll;
        if (rb->scroll >= 1.0f)
            rb->scroll = rb->scroll - 1.0f;
    }
}

static Vec3 ribbon_corner(float x, float z, float y, int turn, int side)
{
    Vec3 p;
    p.x = 12.0f * COS(side) + (x * COS(turn) - z * SIN(turn));
    p.y = y;
    p.z = 12.0f * SIN(side) + (z * COS(turn) + x * SIN(turn));
    return p;
}

static Vec3 ribbon_reach(Vec3 p, float len, int deg)
{
    p.x = COS(deg) * len + p.x;
    p.z = SIN(deg) * len + p.z;
    return p;
}

void prim_ribbon_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRibbon *rb = &self->records.ribbons[i];
        unsigned char red_green;
        int turn, side, bend[2], k;
        float scroll, half_w, y, seam;
        Vec3 p0, p1, p2, p3, in0, in1, out0, out1, cut0, cut1;

        if (rb->active != 1 || rb->alpha <= 0)
            continue;
        red_green = rb->tone == 1.0f ? 0x1e : 0x9b;

        turn = rb->angle;
        side = turn + (turn >= 270 ? -270 : 90);
        scroll = rb->scroll;
        half_w = rb->half_width;
        y = rb->y;
        seam = 12.0f - scroll * 24.0f;
        p0 = ribbon_corner(half_w, -12.0f, y, turn, side);
        p1 = ribbon_corner(half_w, seam, y, turn, side);
        p2 = ribbon_corner(-half_w, seam, y, turn, side);
        p3 = ribbon_corner(-half_w, -12.0f, y, turn, side);
        prim_draw_quad(self, &count, rb->alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE,
                       (char)(unsigned char)(int)rb->texture, 0, red_green, red_green, 0xff, 0.0f, 1.0f, rb->scroll,
                       1.0f, 0);

        scroll = rb->scroll;
        half_w = rb->half_width;
        seam = 12.0f - scroll * 24.0f;
        p0 = ribbon_corner(half_w, seam, y, turn, side);
        p1 = ribbon_corner(half_w, 12.0f, y, turn, side);
        p2 = ribbon_corner(-half_w, 12.0f, y, turn, side);
        p3 = ribbon_corner(-half_w, seam, y, turn, side);
        prim_draw_quad(self, &count, rb->alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE,
                       (char)(unsigned char)(int)rb->texture, 0, red_green, red_green, 0xff, 0.0f, 1.0f, 0.0f,
                       rb->scroll, 0);
        in0 = p1;
        in1 = p2;

        bend[0] = side - 20 < 0 ? side + 340 : side - 20;
        bend[1] = bend[0] - 20 < 0 ? bend[0] + 340 : bend[0] - 20;
        for (k = 0; k < 2; k++) {
            float rest = 1.0f - rb->scroll;
            cut0 = ribbon_reach(in0, rest * 32.0f, bend[k]);
            cut1 = ribbon_reach(in1, rest * 40.0f, bend[k]);
            prim_draw_quad(self, &count, rb->alpha, in0, cut0, cut1, in1, view, PRIM_BLEND_ADDITIVE,
                           (char)(unsigned char)(int)rb->texture, 0, red_green, red_green, 0xff, 0.0f, 1.0f, rb->scroll,
                           1.0f, 0);
            out0 = ribbon_reach(in0, 32.0f, bend[k]);
            out1 = ribbon_reach(in1, 40.0f, bend[k]);
            prim_draw_quad(self, &count, rb->alpha, cut0, out0, out1, cut1, view, PRIM_BLEND_ADDITIVE,
                           (char)(unsigned char)(int)rb->texture, 0, red_green, red_green, 0xff, 0.0f, 1.0f, 0.0f,
                           rb->scroll, 0);
            in0 = out0;
            in1 = out1;
        }
    }
}

#define PRIM_JITTER(p)        ((p)->drift)
#define PRIM_RANDOM_PERIOD(p) ((p)->pattern_arg)

#define PRIM_PATTERN_FORWARD     0x001
#define PRIM_PATTERN_JITTER      0x002
#define PRIM_PATTERN_WANDER      0x004
#define PRIM_PATTERN_WOBBLE      0x008
#define PRIM_PATTERN_FACE_CAMERA 0x020
#define PRIM_PATTERN_HOME        0x100
#define PRIM_PATTERN_CLIP_TINT   0x200
#define PRIM_PATTERN_ACCELERATE  0x400

unsigned char prim_flying_sprites_process(struct EffectPrim *self)
{
    int moving = 1;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimFlyingSprite *fs = &self->records.flying_sprites[i];
        if (fs->active != 1)
            continue;
        fs->offset.x = COS(fs->heading) * fs->speed + fs->offset.x;
        fs->offset.z = SIN(fs->heading) * fs->speed + fs->offset.z;
    }

    self->master_pos.x = self->delta2.x + self->master->base.pos.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;

    if (self->frame == self->change_frame) {
        self->velocity = self->next_velocity;
        self->velocity_accel = self->next_velocity_accel;
    }
    self->speed = self->accel + self->speed;
    self->scale_speed = self->scale_accel + self->scale_speed;

    if (self->pattern & PRIM_PATTERN_ACCELERATE) {
        Vec3 *v = &self->velocity;
        const Vec3 *a = &self->velocity_accel;
        v->y = a->y + v->y;
        v->x = a->x + v->x;
        v->z = a->z + v->z;
    } else if ((self->pattern & PRIM_PATTERN_FORWARD) || self->frame > self->start_frame) {
        const float *m = self->matrix;
        float speed = self->speed;
        Vec3 *v = &self->velocity;
        v->x = m[3] * 0.0f + m[0] * 0.0f + m[6] * speed + m[9];
        v->y = m[4] * 0.0f + m[1] * 0.0f + m[7] * speed + m[10];
        v->z = m[5] * 0.0f + m[2] * 0.0f + m[8] * speed + m[11];
    } else {
        moving = 0;
    }
    if (moving) {
        self->delta.x = self->velocity.x + self->delta.x;
        self->delta.y = self->velocity.y + self->delta.y;
        self->delta.z = self->velocity.z + self->delta.z;
        self->angle[2] = self->angle_speed[2] + self->angle[2];
        self->scale = self->scale + self->scale_speed;
    }

    if (self->pattern & PRIM_PATTERN_JITTER) {
        if (self->frame % PRIM_RANDOM_PERIOD(self) == 0) {
            Vec3 *j = &PRIM_JITTER(self);
            PRIM_RANDOM_PERIOD(self) = rand_below(8) + 12;
            j->x = (float)(rand_below(5) + 5) / 100.0f;
            j->z = (float)(rand_below(5) + 5) / 100.0f;
            if (rand_below(2) == 0)
                j->x = j->x * -1.0f;
            if (rand_below(2) == 0)
                j->z = j->z * -1.0f;
        }
        self->delta.x = PRIM_JITTER(self).x + self->delta.x;
    }

    if ((self->pattern & PRIM_PATTERN_WANDER) && self->frame % PRIM_RANDOM_PERIOD(self) == 0) {
        self->speed = (float)(rand_below(35) + 15) / 100.0f;
        self->angle[0] = (float)rand_below(80) + (self->angle[0] - 40.0f);
        self->angle[1] = (float)rand_below(80) + (self->angle[1] - 40.0f);
        mat_set_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
        mat_rotate_x(self->matrix, deg_to_rad((int)self->angle[1]));
        PRIM_RANDOM_PERIOD(self) = rand_below(10) + 5;
    }

    if ((self->pattern & PRIM_PATTERN_WOBBLE) && self->frame % PRIM_RANDOM_PERIOD(self) == 0) {
        self->angle[0] = (float)rand_below(10) + (self->angle[0] - 5.0f);
        self->angle[1] = (float)rand_below(10) + (self->angle[1] - 5.0f);
        mat_set_rotate_x(self->matrix, deg_to_rad((int)self->angle[1]));
        mat_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
        PRIM_RANDOM_PERIOD(self) = rand_below(15) + 5;
    }

    self->base.pos.x = self->master_pos.x + self->delta.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;

    if (self->pattern & PRIM_PATTERN_HOME) {
        float dist = render_object_calc_ground_dist(&self->base, self->master->offset.x, self->master->offset.z);
        if (3.0f >= dist)
            return 0;
    }
    if (self->pattern & PRIM_PATTERN_FACE_CAMERA)
        self->angle[2] = self->base.roty - game_camera_heading();
    prim_update_alpha(self);
    prim_shift_trail(self);
    return prim_step_motion(self) ? 1 : 0;
}

static float row_rhw(float row, float sy, float depth, float per_row, float eps)
{
    return 1.0f / ((row - sy) * per_row + depth) + eps;
}

static unsigned pack_argb(unsigned short a, unsigned char r, unsigned char g, unsigned char b)
{
    unsigned c = (unsigned)(short)a;
    c = (c << 8) + r;
    c = (c << 8) + g;
    return (c << 8) + b;
}

void prim_flying_sprites_render(struct EffectPrim *self, const float *view)
{
    void *spr = OBJ_SPR(self);
    void *act = render_object_get_act_res(&self->base);
    unsigned argb = 0;
    int i, part, clip_index;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimFlyingSprite *fs = &self->records.flying_sprites[i];

        for (part = 0; part < self->particle_count; part++) {
            const struct PrimParticle *pt = &self->particles[part];
            struct Motion *motion;
            float sx, sy, rhw, pixel, depth;
            Vec3 at;

            at.x = fs->offset.x + pt->pos.x;
            at.y = pt->pos.y + fs->offset.y;
            at.z = pt->pos.z + fs->offset.z;
            depth = at.y * view[5] + view[2] * at.x + at.z * view[8] + view[11];
            renderer_project_point(g_renderer, &at, view, &sx, &sy, &rhw);
            pixel = renderer_screen_scale(g_renderer, rhw);
            motion = act_res_get_motion(act, self->base.action, self->base.motion);

            for (clip_index = 0; clip_index < motion->clip_count; clip_index++) {
                const struct SprClip *clip = motion_get_clip(motion, clip_index);
                struct SprImg *img;
                struct SprScreenQuad q;
                double scale;
                float y0, y1, x0, x1;
                unsigned short clip_alpha;

                if (clip->sprIndex == -1)
                    continue;
                img = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
                if (!img) continue;
                scale = pt->scale;
                y0 = (float)((double)clip->y * scale * clip->zoomY * pixel + sy);
                y1 = (float)((double)(clip->y + (img->heightHalved + 1) * img->height) * scale * clip->zoomY * pixel
                             + sy);
                if (y0 >= (float)g_renderer->height || 0.0f >= y1)
                    continue;
                x0 = (float)((double)clip->x * scale * clip->zoomX * pixel + sx);
                x1 = (float)((double)(clip->x + (img->widthHalved + 1) * img->width) * scale * clip->zoomX * pixel
                             + sx);
                if (x0 >= (float)g_renderer->width || 0.0f >= x1)
                    continue;

                q.top = y0;
                q.bottom = y1;
                q.left = clip->flags & CLIP_MIRRORED ? x1 : x0;
                q.right = clip->flags & CLIP_MIRRORED ? x0 : x1;
                q.image = img;
                clip_alpha = (unsigned short)(int)((float)clip->color[3] * 0.85f);
                q.rhw_bottom = row_rhw(y1, sy, depth, g_spriteRowDepthStep, 0.0001f);
                q.rhw_top = row_rhw(y0, sy, depth, g_spriteRowDepthStep, 0.0001f);
                if (self->pattern & PRIM_PATTERN_CLIP_TINT) {
                    if (fs->tint == 0)
                        argb = pack_argb(clip_alpha, clip->color[0], clip->color[1], clip->color[2]);
                    else if (fs->tint == 1)
                        argb = ((unsigned)(short)clip_alpha << 24) + 0x3737ff;
                } else {
                    argb = pt->argb;
                }
                q.argb = argb;
                q.angle = (float)clip->angle + self->angle[2];
                sprite_quad_submit(&self->base, &q, SPR_PALETTE(spr), 5, 0.0f, 0.0f);

                clip_alpha = (unsigned short)(int)((float)clip->color[3] * 0.5f);
                q.image = img;
                q.top = q.top - 4.0f;
                q.bottom = q.bottom + 4.0f;
                q.left = q.left - 4.0f;
                q.right = q.right + 4.0f;
                q.rhw_bottom = row_rhw(y1, sy, depth, g_spriteRowDepthStep, 0.0002f);
                q.rhw_top = row_rhw(y0, sy, depth, g_spriteRowDepthStep, 0.0002f);
                if (self->pattern & PRIM_PATTERN_CLIP_TINT) {
                    if (fs->tint == 0)
                        argb = pack_argb(clip_alpha, clip->color[0], 0x37, 0x37);
                    else if (fs->tint == 1)
                        argb = ((unsigned)(short)clip_alpha << 24) + 0x3737ff;
                } else {
                    argb = pt->argb;
                }
                q.argb = argb;
                q.angle = (float)clip->angle + self->angle[2];
                sprite_quad_submit(&self->base, &q, SPR_PALETTE(spr), 5, 0.0f, 0.0f);
            }
        }
    }
}

unsigned char prim_ground_sprite_process(struct EffectPrim *self)
{
    follow_master(self);
    return prim_step_motion(self);
}

void prim_ground_sprite_render(struct EffectPrim *self, const float *view)
{
    struct RagEffect *master = self->master;
    float sx, sy, rhw, pixel, depth;
    void *spr, *act;
    struct Motion *motion;
    Vec3 at;
    int clip_index;

    if (master)
        self->base.pos = master->base.pos;
    at.x = self->base.pos.x;
    {
        void *attr = game_current_map_attr();
        if (!attr)
            return;
        at.y = c3d_attr_get_height(attr, self->base.pos.x, self->base.pos.z);
    }
    at.z = self->base.pos.z;
    depth = view[2] * at.x + view[5] * at.y + view[8] * at.z + view[11];
    renderer_project_point(g_renderer, &at, view, &sx, &sy, &rhw);
    pixel = renderer_screen_scale(g_renderer, rhw);
    master = self->master;
    if (master && master->effect_id == EFFECT_DIGITAL_SPACE && master->master)
        render_object_apply_shrink_scale((struct GameActor *)master->master, &pixel);

    spr = OBJ_SPR(self);
    act = render_object_get_act_res(&self->base);
    motion = act_res_get_motion(act, self->base.action, self->base.motion);
    for (clip_index = 0; clip_index < motion->clip_count; clip_index++) {
        const struct SprClip *clip = motion_get_clip(
            act_res_get_motion(act, self->base.action, self->base.motion), clip_index);
        struct SprImg *img;
        struct SprScreenQuad q;
        float x0, x1, y0, y1, rise, shift_x, fade;
        unsigned argb;

        if (clip->sprIndex == -1)
            break;
        img = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
        if (!img) break;
        x0 = (float)((double)clip->x * pixel * clip->zoomX + sx);
        x1 = (float)((double)(clip->x + (img->widthHalved + 1) * img->width) * pixel * clip->zoomX + sx + 1.0);
        rise = (float)(double)self->master_sprite_lift;
        y0 = (float)((double)clip->y * pixel * clip->zoomY + sy) - rise;
        y1 = (float)((double)(clip->y + (img->heightHalved + 1) * img->height) * pixel * clip->zoomY + sy + 1.0) - rise;
        if (y0 >= (float)g_renderer->height || 0.0f >= y1 || x0 >= (float)g_renderer->width || 0.0f >= x1)
            break;

        argb = pack_argb(clip->color[3], clip->color[0], clip->color[1], clip->color[2]);
        shift_x = (float)OBJ_SHIFT_X(&self->base);
        q.top = y0;
        q.left = shift_x + x0;
        q.bottom = y1;
        q.right = shift_x + x1;
        q.image = img;
        q.rhw_bottom = 1.0f / (depth - (y1 - sy) * g_depthPerPixelCot) + 1e-5f;
        q.rhw_top = 1.0f / (depth - (y0 - sy) * g_depthPerPixelCot) + 1e-5f;
        fade = (float)(unsigned char)(int)self->alpha / 255.0f * (float)(argb >> 24);
        q.argb = (unsigned)(unsigned char)(int)fade << 24 | (argb & 0xffffff);
        q.angle = (float)clip->angle;
        sprite_quad_submit(&self->base, &q, SPR_PALETTE(spr), self->list_flags, 0.0f, 0.0f);
    }
}

static int wisp_turn(float *angle, float target, float step)
{
    float diff = *angle - target;
    float turned;

    if (180.0f > diff && diff >= 0.0f) {
        turned = *angle - step;
        *angle = turned;
        return target >= turned;
    }
    if (-180.0f > diff) {
        turned = *angle - step;
        *angle = turned;
        return target - 360.0f >= turned;
    }
    turned = *angle + step;
    *angle = turned;
    return turned >= target;
}

static void wrap_360(float *angle)
{
    float value = *angle;
    if (0.0f > value) {
        value = value + 360.0f;
        *angle = value;
    }
    if (value >= 360.0f)
        *angle = value - 360.0f;
}

unsigned char prim_wisps_process(struct EffectPrim *self)
{
    short pitch;
    int i, k;

    self->camera_yaw = (short)(360 - (short)(int)game_camera_heading());
    pitch = (short)(450 - (short)(int)game_camera_pitch());
    self->camera_pitch = pitch;
    if (pitch >= 360)
        self->camera_pitch = (short)(pitch - 360);

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimTwinWisps *wp = &self->records.twin_wisps[i];
        float speed;

        if (wp->active != 1)
            continue;
        wp->frame++;
        for (k = 0; k < 2; k++) {
            struct PrimWispCourse *head = &wp->course[k];
            if (wisp_turn(&head->yaw, head->yaw_target, 2.0f))
                head->yaw_target = head->base_yaw + (float)(rand() % 360);
            if (wisp_turn(&head->pitch, head->pitch_target, 1.0f))
                head->pitch_target = (float)(rand() % 11 + 175);
        }
        for (k = 0; k < 2; k++) {
            wrap_360(&wp->course[k].base_yaw);
            wrap_360(&wp->course[k].yaw_target);
            wrap_360(&wp->course[k].yaw);
            wrap_360(&wp->course[k].pitch_target);
            wrap_360(&wp->course[k].pitch);
        }
        for (k = 0; k < 2; k++) {
            if (wp->frame < 50) {
                wp->alpha[k] = wp->alpha[k] + 25.0f;
                if (wp->alpha[k] >= 255.0f)
                    wp->alpha[k] = 255.0f;
            } else if (wp->frame > 60) {
                wp->alpha[k] = wp->alpha[k] - 3.0f;
                if (0.0f > wp->alpha[k]) {
                    wp->active = 0;
                    wp->alpha[k] = 0.0f;
                }
            }
        }

        speed = wp->speed;
        wp->pos0.x = COS((int)wp->course[0].yaw) * speed + wp->pos0.x;
        wp->pos0.z = SIN((int)wp->course[0].yaw) * speed + wp->pos0.z;
        wp->pos0.y = wp->pos0.y - SIN((int)wp->course[0].pitch) * speed;
        wp->pos1.x = COS((int)wp->course[1].yaw) * speed + wp->pos1.x;
        wp->pos1.z = SIN((int)wp->course[1].yaw) * speed + wp->pos1.z;
        wp->pos1.y = SIN((int)wp->course[1].pitch) * speed + wp->pos1.y;
    }
    return 1;
}

static void facing_square_corners(struct EffectPrim *self, float size, Vec3 c[4])
{
    int k;

    for (k = 0; k < 4; k++) {
        c[k].x = COS(45 + 90 * k) * size;
        c[k].y = 0.0f;
        c[k].z = SIN(45 + 90 * k) * size;
    }
    prim_quad_turn(&c[0], &c[1], &c[2], &c[3], self->camera_yaw, self->camera_pitch);
}

void prim_facing_square_render(struct EffectPrim *self, const float *view)
{
    const struct PrimFacingSquare *sq = &self->records.facing_square;
    int count = 0;
    float size, near_view[12];
    Vec3 rel[4], corner[4], nearer;
    int k;

    if (sq->active != 1)
        return;

    size = ((float)sq->alpha / 255.0f * 0.2f + 1.0f) * sq->size + SIN((int)sq->pulse) * sq->size * 0.05f;
    facing_square_corners(self, size, rel);
    for (k = 0; k < 4; k++) {
        corner[k].x = sq->centre.x + rel[k].x;
        corner[k].y = sq->centre.y + rel[k].y;
        corner[k].z = sq->centre.z + rel[k].z;
    }
    self->list_flags = 5;
    nearer.x = 0.0f;
    nearer.y = 0.0f;
    nearer.z = 0.01f;
    memcpy(near_view, view, sizeof(near_view));
    mat_translate(near_view, &nearer);
    prim_draw_quad(self, &count, (short)(unsigned short)sq->alpha, corner[0], corner[1], corner[2], corner[3],
                   near_view, PRIM_BLEND_ADDITIVE, 0, 0, 0xb4, 0xb4, 0xb4, 0.0f, 1.0f, 0.0f, 1.0f, 0);

    size = SIN((int)sq->pulse) * sq->size * 0.05f + sq->size;
    facing_square_corners(self, size, rel);
    for (k = 0; k < 4; k++) {
        corner[k].x = sq->centre.x + rel[k].x;
        corner[k].y = sq->centre.y + rel[k].y;
        corner[k].z = sq->centre.z + rel[k].z;
    }
    self->list_flags = 0x101;
    prim_draw_quad(self, &count, 0xff, corner[0], corner[1], corner[2], corner[3], view, PRIM_BLEND_ADDITIVE, 0, 0,
                   0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

void prim_swell_rings_process(struct EffectPrim *self)
{
    int i, j;

    follow_master(self);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSwellRing *rg = &self->records.swell_rings[i];
        short next;

        if (rg->active != 1)
            continue;
        if (++rg->frame <= 0)
            continue;

        next = (short)(rg->spin + 1);
        rg->spin = next;
        if (next >= 360)
            rg->spin = (short)(next - 360);
        if (rg->frame < 30) {
            int limit = g_session1.map_rule_flags[0] == 1 ? 80 : 150;
            next = (short)(rg->alpha + 5);
            rg->alpha = next;
            if (next > limit)
                rg->alpha = (short)limit;
        }
        if (rg->frame > self->duration - 46) {
            next = (short)(rg->alpha - 4);
            rg->alpha = next < 0 ? 0 : next;
        }
        for (j = 0; j <= SWELL_RING_SEGMENTS; j++) {
            int phase = i * 45 < 90 ? rg->frame + i * 90 : (rg->frame + i * 45) * 2;
            rg->swell[j] = SIN(9 * j) * rg->radius * 0.3f * SIN(phase % 360) + rg->radius;
        }
    }
}

void prim_swell_rings_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    Vec3 prev_out, prev_rim;
    int i;

    memset(&prev_out, 0, sizeof(prev_out));
    memset(&prev_rim, 0, sizeof(prev_rim));
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        const struct PrimSwellRing *rg = &self->records.swell_rings[i];
        int step, along, seg;

        if (rg->active != 1 || rg->frame <= 0)
            continue;
        step = rg->arc / 20;
        if (rg->arc < 0)
            continue;
        for (along = 0, seg = 0; along <= rg->arc; along += step) {
            int deg = (rg->spin + along) % 360;
            float reach;
            Vec3 out, rim;

            if (rg->arc == 360 && seg == SWELL_RING_SEGMENTS)
                deg = rg->spin;
            reach = COS(rg->tilt) * rg->swell[seg];
            rim.x = rg->rim * COS(deg);
            rim.y = 5.0f;
            rim.z = rg->rim * SIN(deg);
            out.x = reach * COS(deg) + rim.x;
            out.y = 5.0f - SIN(rg->tilt) * rg->swell[seg];
            out.z = reach * SIN(deg) + rim.z;
            if (seg > 0)
                prim_draw_strip(self, &count, seg - 1, rg->alpha, prev_rim, rim, out,
                                prev_out, view, (char)(rg->frame % self->motion_count));
            seg = seg + 1 >= SWELL_RING_SEGMENTS + 1 ? SWELL_RING_SEGMENTS : seg + 1;
            prev_out = out;
            prev_rim = rim;
        }
    }
}

void prim_facing_square_process(struct EffectPrim *self)
{
    struct PrimFacingSquare *sq = &self->records.facing_square;
    short pitch;

    if (sq->active != 1)
        return;
    self->camera_yaw = (short)(360 - (short)(int)game_camera_heading());
    pitch = (short)(450 - (short)(int)game_camera_pitch());
    self->camera_pitch = pitch;
    if (pitch >= 360)
        self->camera_pitch = (short)(pitch - 360);
    follow_master(self);
    sq->age++;
    if (sq->fading == 0.0f) {
        sq->alpha += 4;
        if (sq->alpha > 255) {
            sq->alpha = 255;
            sq->fading = 1.0f;
        }
    } else {
        sq->alpha -= 2;
        if (sq->alpha < 0) {
            sq->alpha = 0;
            sq->fading = 0.0f;
        }
    }
    sq->centre.x = SIN((int)sq->wave_x);
    sq->centre.z = SIN((int)sq->wave_z);
    sq->centre.y = SIN((int)sq->wave_y);
    sq->centre.x = SIN((int)sq->orbit) * 15.0f + sq->centre.x;
    sq->centre.z = COS((int)sq->orbit) * 15.0f + sq->centre.z;
    sq->centre.y = sq->lift + sq->centre.y;
}
