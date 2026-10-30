#include "render/projection.h"
#include "render/math3d.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/actor_ops.h"
#include "actor/game_actor.h"

#define CLIP_MIRRORED 1
#define SPRITE_CLIP_COLORS 0x200
#define SPR_PALETTE(spr) (((struct SprRes *)(spr))->palette)

struct PrimSpriteQuad {
    float top;
    float left;
    float bottom;
    float right;
    float rhw_bottom;
    float rhw_top;
    unsigned color;
    float angle;
    struct SprImg *image;
};

typedef void(*SpriteQuadFn)(struct RenderObject *self, struct PrimSpriteQuad *quad,
                                       void *palette, int list_flags, float dx, float dy);
#define sprite_quad_submit ((SpriteQuadFn)render_object_submit_spr_quad)

static int rand_below(int n)
{
    int rem = rand() % n;

    return rem < 0 ? 0 : rem;
}

static unsigned char finish_frame(struct EffectPrim *self)
{
    prim_update_alpha(self);
    prim_shift_trail(self);
    return prim_step_motion(self) != 0;
}

#define GRAVITY_ALWAYS       0x001
#define GRAVITY_DRIFT        0x002
#define GRAVITY_WANDER       0x004
#define GRAVITY_JITTER       0x008
#define GRAVITY_FACE_CAMERA  0x020
#define GRAVITY_HOME         0x100
#define GRAVITY_RAW_VELOCITY 0x400

static void gravity_move(struct EffectPrim *self)
{
    int frame = self->frame;

    if (frame == self->change_frame) {
        self->velocity = self->next_velocity;
        self->velocity_accel = self->next_velocity_accel;
    }
    self->speed = self->accel + self->speed;
    self->scale_speed = self->scale_accel + self->scale_speed;
    if (self->pattern & GRAVITY_RAW_VELOCITY) {
        self->velocity.y = self->velocity_accel.y + self->velocity.y;
        self->velocity.x = self->velocity_accel.x + self->velocity.x;
        self->velocity.z = self->velocity_accel.z + self->velocity.z;
    } else if ((self->pattern & GRAVITY_ALWAYS) || frame > self->start_frame) {
        self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->speed);
    } else {
        return;
    }
    self->delta.x = self->velocity.x + self->delta.x;
    self->delta.y = self->velocity.y + self->delta.y;
    self->delta.z = self->velocity.z + self->delta.z;
    self->angle[2] = self->angle_speed[2] + self->angle[2];
    self->scale = self->scale_speed + self->scale;
}

static void gravity_random(struct EffectPrim *self)
{
    if (self->pattern & GRAVITY_DRIFT) {
        if (self->frame % self->pattern_arg == 0) {
            self->pattern_arg = rand_below(8) + 12;
            self->drift.x = (float)(rand_below(5) + 5) / 100.0f;
            self->drift.z = (float)(rand_below(5) + 5) / 100.0f;
            if (rand_below(2) == 0)
                self->drift.x = self->drift.x * -1.0f;
            if (rand_below(2) == 0)
                self->drift.z = self->drift.z * -1.0f;
        }
        self->delta.x = self->drift.x + self->delta.x;
    }
    if ((self->pattern & GRAVITY_WANDER) && self->frame % self->pattern_arg == 0) {
        self->speed = (float)(rand_below(35) + 15) / 100.0f;
        self->angle[0] = (float)rand_below(80) + (self->angle[0] - 40.0f);
        self->angle[1] = (float)rand_below(80) + (self->angle[1] - 40.0f);
        mat_set_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
        mat_rotate_x(self->matrix, deg_to_rad((int)self->angle[1]));
        self->pattern_arg = rand_below(10) + 5;
    }
    if ((self->pattern & GRAVITY_JITTER) && self->frame % self->pattern_arg == 0) {
        self->angle[0] = (float)rand_below(10) + (self->angle[0] - 5.0f);
        self->angle[1] = (float)rand_below(10) + (self->angle[1] - 5.0f);
        mat_set_rotate_x(self->matrix, deg_to_rad((int)self->angle[1]));
        mat_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
        self->pattern_arg = rand_below(15) + 5;
    }
}

static void gravity_face_camera(struct EffectPrim *self)
{
    if (self->pattern & GRAVITY_FACE_CAMERA) {
        self->angle[2] = self->base.roty - game_camera_heading();
    }
}

unsigned char prim_gravity_process(struct EffectPrim *self)
{
    const Vec3 *at = &self->master->base.pos;

    self->master_pos.x = at->x + self->delta2.x;
    self->master_pos.y = at->y + self->delta2.y;
    self->master_pos.z = at->z + self->delta2.z;
    gravity_move(self);
    gravity_random(self);
    self->base.pos.x = self->master_pos.x + self->delta.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;
    if (self->pattern & GRAVITY_HOME) {
        float home_dist = render_object_calc_ground_dist(&self->base, self->master->offset.x, self->master->offset.z);

        if (3.0f >= home_dist)
            return 0;
    }
    gravity_face_camera(self);
    return finish_frame(self);
}

unsigned char prim_gravity_fixed_process(struct EffectPrim *self)
{
    Vec3 base;

    base.x = self->master_pos.x + self->delta2.x;
    base.y = self->master_pos.y + self->delta2.y;
    base.z = self->master_pos.z + self->delta2.z;
    gravity_move(self);
    gravity_random(self);
    self->base.pos.x = base.x + self->delta.x;
    self->base.pos.y = base.y + self->delta.y;
    self->base.pos.z = base.z + self->delta.z;
    gravity_face_camera(self);
    return finish_frame(self);
}

unsigned char prim_circling_process(struct EffectPrim *self)
{
    Vec3 offset;

    self->master_pos = self->master->base.pos;
    self->size[0].speed = self->size[0].speed + self->size[0].accel;
    self->angle_speed[0] = self->angle_accel[0] + self->angle_speed[0];
    self->fall_speed = self->fall_speed + self->gravity;
    self->size[0].value = self->size[0].value + self->size[0].speed;
    self->angle[0] = self->angle[0] + self->angle_speed[0];
    self->delta2.y = self->delta2.y + self->fall_speed;
    self->master_pos.x = self->delta2.x + self->master_pos.x;
    self->master_pos.y = self->delta2.y + self->master_pos.y;
    self->master_pos.z = self->delta2.z + self->master_pos.z;
    self->angle[2] = self->angle_speed[2] + self->angle[2];
    if (self->frame == self->change_frame) {
        self->fall_speed = self->next_speed;
        self->gravity = self->next_accel;
    }
    if (self->alpha_keys_set_size != 0) {
        int key = self->alpha_key;

        if (key < 0 || (unsigned)key >= (unsigned)VEC_COUNT(&self->alpha_key_frames, float)) {
            self->alpha_key = 0;
            key = 0;
        }
        if ((float)self->frame == ((const float *)self->alpha_key_frames.first)[key]) {
            self->size[0].speed = ((const float *)self->alpha_key_values.first)[key];
            self->size[0].accel = ((const float *)self->alpha_key_ceilings.first)[key];
            self->alpha_key = key + 1;
        }
    }
    mat_set_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
    offset = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->size[0].value);
    self->delta = offset;
    self->base.pos.z = self->master_pos.z + offset.z;
    self->base.pos.x = offset.x + self->master_pos.x;
    self->base.pos.y = self->master_pos.y + offset.y;
    self->scale = self->scale_speed + self->scale;
    self->angle[1] = self->angle_speed[1] + self->angle[1];
    mat_rotate_z(self->matrix, self->angle[1]);
    return finish_frame(self);
}

unsigned char prim_swing_process(struct EffectPrim *self)
{
    const Vec3 *at = &self->master->base.pos;
    float swing_speed, travel;

    self->master_pos.y = at->y + self->delta2.y;
    self->master_pos.z = at->z + self->delta2.z;
    self->master_pos.x = self->delta2.x + at->x;
    swing_speed = self->angle_speed[1];
    self->angle[1] = swing_speed + self->angle[1];
    self->speed = self->accel + self->speed;
    self->angle_speed[1] = self->angle_accel[1] + swing_speed;
    self->fall_speed = self->gravity + self->fall_speed;
    travel = self->speed - self->fall_speed;
    self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, travel);
    mat_set_rotate_axis(self->matrix2, &self->swing_axis, deg_to_rad((int)self->angle[2]));
    self->drift = prim_mat_apply(self->matrix2, 0.0f, self->angle[1], 0.0f);
    self->scale = self->scale_speed + self->scale;
    self->delta.x = self->delta.x - self->velocity.x;
    self->delta.y = self->delta.y - self->velocity.y;
    self->delta.z = self->delta.z - self->velocity.z;
    self->base.pos.x = self->drift.x + (self->master_pos.x + self->delta.x);
    self->base.pos.y = self->drift.y + (self->delta.y + self->master_pos.y);
    self->base.pos.z = self->drift.z + (self->delta.z + self->master_pos.z);
    return finish_frame(self);
}

static void sway_step(float *pair)
{
    float angle = pair[0], target = pair[1];
    int below, pick;

    if (target > angle) {
        angle += 3.0f;
        if (angle > 359.0f)
            angle = 359.0f;
        pair[0] = angle;
        if (angle > target) {
            below = (int)angle;
            if (below == 0) {
                pick = 0;
            } else {
                pick = rand() % below;
                if (pick < 0)
                    pick = 0;
            }
            pair[1] = (float)pick;
        }
    } else {
        angle -= 2.0f;
        if (0.0f > angle)
            angle = 0.0f;
        pair[0] = angle;
        if (target >= angle) {
            below = (int)angle;
            if (below == 0) {
                pick = 0;
            } else {
                pick = rand() % below;
                if (pick < 0)
                    pick = 0;
            }
            pair[1] = (float)(359 - pick);
        }
    }
}

unsigned char prim_ghost_process(struct EffectPrim *self)
{
    struct PrimSpriteMover *ghost = &self->records.sprite_mover;

    self->base.pos = self->master->base.pos;
    if (ghost->active == 1) {
        int prev_age = ghost->age, age = prev_age + 1;
        short wrapped;
        int deg, facing, facing_deg;

        ghost->age = age;
        if (prev_age < 250 && ghost->sway[0][1] > self->alpha)
            self->alpha = self->alpha + 1.0f;
        if (age >= 2600) {
            if (ghost->mode <= 1) {
                if (age % (360 / ghost->style) == 0) {
                    ghost->style++;
                    age = ghost->age;
                }
                ghost->radius = ghost->radius * 1.002f;
            }
            if (age % 15 == 0) {
                float alpha = self->alpha;

                if (ghost->mode == 2)
                    alpha = alpha - 5.0f;
                else
                    alpha = alpha - 1.0f;
                self->alpha = alpha;
                if (0.0f > alpha)
                    self->alpha = 0.0f;
            }
        }
        wrapped = (short)(ghost->bob + 3);
        ghost->bob = wrapped;
        if (wrapped >= 360)
            ghost->bob = (short)(wrapped - 360);
        wrapped = (short)(ghost->style + ghost->angle);
        ghost->angle = wrapped;
        if (wrapped >= 360)
            ghost->angle = (short)(wrapped - 360);

        ghost->pos = ghost->centre;
        deg = ghost->angle;
        ghost->pos.x = g_cosTable[deg] * ghost->radius + ghost->centre.x;
        if (ghost->mode == 2) {
            int twice = deg + deg;

            deg = twice < 360 ? twice : twice - 360;
        }
        ghost->pos.z = g_sinTable[deg] * ghost->radius + ghost->pos.z;
        ghost->pos.y = g_sinTable[ghost->bob] * ghost->speed + ghost->pos.y;
        self->delta = ghost->pos;

        facing = ghost->angle + (int)game_camera_heading();
        facing_deg = facing >= 0 ? facing : facing + 360;
        if (facing_deg >= 360)
            facing_deg -= 360;
        if (ghost->mode == 0) {
            if ((unsigned)(facing_deg - 180) <= 89)
                self->base.action = 6;
            else if ((unsigned)(facing_deg - 90) <= 89)
                self->base.action = 0;
            else
                self->base.action = (unsigned)facing_deg > 89 ? 6 : 0;
        } else {
            if (rand_below(999) == 1)
                self->base.action = 1;
            if (rand_below(1001) == 0)
                self->base.action = 0;
        }
    }
    self->base.pos = self->delta;
    return finish_frame(self);
}

unsigned char prim_petal_process(struct EffectPrim *self)
{
    struct PrimSpriteMover *petal = &self->records.sprite_mover;

    self->base.pos = self->master->base.pos;
    if (petal->active == 1) {
        float alpha;

        sway_step(petal->sway[0]);
        sway_step(petal->sway[1]);
        petal->pos.y = petal->pos.y - petal->speed;
        alpha = self->alpha - 3.0f;
        self->alpha = alpha;
        if (3.0f > alpha)
            self->alpha = 0.0f;
        petal->pos.x = g_sinTable[(int)petal->sway[0][0]] * 0.15f + petal->pos.x;
        petal->pos.z = g_sinTable[(int)petal->sway[1][0]] * 0.15f + petal->pos.z;
        self->delta = petal->pos;
    }
    self->base.pos = self->delta;
    gravity_face_camera(self);
    return finish_frame(self);
}

unsigned char prim_leaf_process(struct EffectPrim *self)
{
    struct PrimSpriteMover *leaf = &self->records.sprite_mover;

    self->base.pos = self->master->base.pos;
    if (leaf->active == 1) {
        float height;

        sway_step(leaf->sway[0]);
        sway_step(leaf->sway[1]);
        height = leaf->speed + leaf->pos.y;
        leaf->pos.y = height;
        if (leaf->style == 0) {
            leaf->pos.x = g_sinTable[(int)leaf->sway[0][0]] * 0.24f + leaf->pos.x;
            leaf->pos.z = g_sinTable[(int)leaf->sway[1][0]] * 0.3f + leaf->pos.z;
        } else {
            leaf->pos.x = g_sinTable[(int)leaf->sway[0][0]] * 0.12f + leaf->pos.x;
            leaf->pos.z = g_sinTable[(int)leaf->sway[1][0]] * 0.15f + leaf->pos.z;
        }
        if (height > self->base.pos.y) {
            leaf->pos.x = (float)rand_below(300) + self->base.pos.x - 150.0f;
            leaf->pos.y = self->base.pos.y - 200.0f;
            leaf->pos.z = (float)rand_below(300) + self->base.pos.z - 150.0f;
        }
        self->delta = leaf->pos;
    }
    self->base.pos = self->delta;
    gravity_face_camera(self);
    return finish_frame(self);
}

static float angel_fade_step(int style, int style2, int age)
{
    switch (style) {
    case 0:  return age > 100 ? 4.0f : 0.0f;
    case 2:  return age > 30 ? 3.0f : 0.0f;
    case 3:  return age > 60 ? 3.0f : 0.0f;
    case 4:  return age > 70 ? 3.0f : 0.0f;
    case 10: return age > 100 ? 6.0f : 0.0f;
    default: return age > 75 ? (style2 != 0 ? 3.0f : 6.0f) : 0.0f;
    }
}

unsigned char prim_angel_process(struct EffectPrim *self)
{
    struct PrimSpriteMover *angel = &self->records.sprite_mover;

    self->base.pos = self->master->base.pos;
    if (angel->active == 1) {
        int age;
        float alpha, ceiling, fade;

        angel->age++;
        angel->centre.y = angel->centre.y - 0.1f;
        angel->pos = angel->centre;
        self->delta = angel->pos;
        age = angel->age;
        if (age < 50) {
            if (angel->style2 == 0) {
                if (angel->style == 1) {
                    ceiling = 200.0f;
                    if (ceiling > self->alpha)
                        self->alpha = self->alpha + 5.0f;
                } else if (angel->style == 10) {
                    alpha = self->alpha;
                    if (255.0f > alpha) {
                        alpha = alpha + 10.0f;
                        self->alpha = alpha;
                    }
                    if (alpha > 255.0f)
                        self->alpha = 255.0f;
                } else if (angel->style == 11) {
                    self->alpha = 0.0f;
                } else if (angel->style >= 2) {
                    if (age < 30) {
                        ceiling = 150.0f;
                        if (ceiling > self->alpha)
                            self->alpha = self->alpha + 5.0f;
                    }
                } else {
                    ceiling = 245.0f;
                    if (ceiling > self->alpha)
                        self->alpha = self->alpha + 5.0f;
                }
            } else {
                ceiling = (float)(40 - angel->style2);
                if (ceiling > self->alpha)
                    self->alpha = self->alpha + 1.0f;
            }
        }

        age = angel->age;
        if (age > 30) {
            if (angel->style == 10 || angel->style == 11) {
                if (age % 4 == 0)
                    self->base.motion++;
                if (self->base.motion > 15)
                    self->base.motion = 15;
            } else if (angel->style >= 0) {
                if (age % 3 == 0)
                    self->base.motion++;
                if (self->base.motion > 29)
                    self->base.motion = 29;
            }
        }

        fade = angel_fade_step(angel->style, angel->style2, angel->age);
        if (fade != 0.0f) {
            alpha = self->alpha - fade;
            self->alpha = alpha;
            if (0.0f > alpha)
                self->alpha = 0.0f;
        }
    }
    self->base.pos = self->delta;
    prim_shift_trail(self);
    return 1;
}

static float clip_edge(int px, float scale, float zoom, float pixel, float screen)
{
    return (float)((double)px * (double)scale * (double)zoom * (double)pixel + (double)screen);
}

void prim_sprite_trail_render(struct EffectPrim *self, const float *view)
{
    void *sprres = render_object_get_spr_res(&self->base);
    void *actres = render_object_get_act_res(&self->base);
    int i, j;

    for (i = 0; i < self->particle_count; i++) {
        struct PrimParticle *part = &self->particles[i];
        float depth, screen_x, screen_y, rhw, pixels_per_unit;
        struct Motion *motion;

        depth = part->pos.y * view[5] + view[2] * part->pos.x + view[8] * part->pos.z + view[11];
        renderer_project_point(g_renderer, &part->pos, view, &screen_x, &screen_y, &rhw);
        pixels_per_unit = renderer_screen_scale(g_renderer, rhw);
        motion = act_res_get_motion(actres, self->base.action, self->base.motion);
        for (j = 0; j < motion->clip_count; j++) {
            struct SprClip *clip = motion_get_clip(motion, j);
            struct SprImg *image;
            struct PrimSpriteQuad quad;
            float top, bottom, left, right;

            if (clip->sprIndex == -1)
                continue;
            image = sprite_resource_image(sprres, clip->clipType, clip->sprIndex);
            if (!image) continue;
            top = clip_edge(clip->y, part->scale, clip->zoomY, pixels_per_unit, screen_y);
            bottom = clip_edge((image->heightHalved + 1) * image->height + clip->y, part->scale, clip->zoomY, pixels_per_unit, screen_y);
            if (top >= (float)g_renderer->height)
                continue;
            if (0.0f >= bottom)
                continue;
            left = clip_edge(clip->x, part->scale, clip->zoomX, pixels_per_unit, screen_x);
            right = clip_edge((image->widthHalved + 1) * image->width + clip->x, part->scale, clip->zoomX, pixels_per_unit, screen_x);
            if (left >= (float)g_renderer->width)
                continue;
            if (0.0f >= right)
                continue;

            quad.top = top;
            quad.bottom = bottom;
            if (clip->flags & CLIP_MIRRORED) {
                quad.left = right;
                quad.right = left;
            } else {
                quad.left = left;
                quad.right = right;
            }
            quad.image = image;
            quad.rhw_bottom = 1.0f / ((bottom - screen_y) * g_spriteRowDepthStep + depth) + 0.0001f;
            quad.rhw_top = 1.0f / ((top - screen_y) * g_spriteRowDepthStep + depth) + 0.0001f;
            if (self->pattern & SPRITE_CLIP_COLORS) {
                unsigned c = clip->color[3];

                c = (c << 8) + clip->color[0];
                c = (c << 8) + clip->color[1];
                c = (c << 8) + clip->color[2];
                quad.color = c;
            } else {
                quad.color = part->argb;
            }
            quad.angle = (float)clip->angle + self->angle[2];
            sprite_quad_submit(&self->base, &quad, SPR_PALETTE(sprres), self->list_flags, 0.0f, 0.0f);
        }
    }
}

static void sprite_tail(struct EffectPrim *self)
{
    gravity_face_camera(self);
    prim_update_alpha(self);
    prim_shift_trail(self);
    prim_step_motion(self);
}

void prim_sprite_at_point_process(struct EffectPrim *self)
{
    struct PrimSpriteMover *mv = &self->records.sprite_mover;

    self->base.pos = self->master->base.pos;
    if (mv->active == 1)
        self->delta = mv->pos;
    self->base.pos = self->delta;
    sprite_tail(self);
}

void prim_sprite_pinned_process(struct EffectPrim *self)
{
    struct PrimSpriteMover *mv = &self->records.sprite_mover;

    self->base.pos = self->master->base.pos;
    if (mv->active == 1) {
        mv->age++;
        if (mv->follow_master == 1)
            self->base.pos = self->master->base.pos;
        else
            self->base.pos = mv->pos;
    }
    sprite_tail(self);
}
