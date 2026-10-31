#include "session/party_queries.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "render/math3d.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "session/state.h"
#include "core/memory.h"
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "actor/game_actor.h"
#include "session/session_skills.h"

struct GameObject;
struct RenderObject;

static const struct RenderObjectOps effect_prim_ops = {
    .base = {
        .destroy = effect_prim_delete,
        .on_process = effect_prim_on_process,
        .send_msg = render_object_send_msg,
        .render = effect_prim_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

#define ZERO_AS_INT(field) (*(int *)&(field) = 0)

struct EffectPrim *effect_prim_construct(struct EffectPrim *self)
{
    int i;

    render_object_construct(&self->base);
    self->base.ops = &effect_prim_ops.base;
    memset(&self->alpha_key_frames, 0, sizeof(struct Vec));
    memset(&self->alpha_key_values, 0, sizeof(struct Vec));
    memset(&self->alpha_key_ceilings, 0, sizeof(struct Vec));
    memset(&self->alpha_key_floors, 0, sizeof(struct Vec));

    memset(&self->velocity, 0, sizeof self->velocity);
    memset(&self->master_pos, 0, sizeof self->master_pos);
    memset(&self->delta, 0, sizeof self->delta);
    memset(&self->delta2, 0, sizeof self->delta2);
    memset(&self->drift, 0, sizeof self->drift);
    memset(&self->velocity_accel, 0, sizeof self->velocity_accel);
    memset(self->corners, 0, sizeof self->corners);
    memset(&self->records, 0, sizeof self->records);
    for (i = 0; i < PRIM_PARTICLES; i++)
        prim_particle_construct(&self->particles[i]);
    for (i = 0; i < PRIM_TRIANGLES; i++)
        prim_triangle_construct(&self->triangles[i]);

    self->bounce_clink = 0;
    self->texture_frame = 0;
    self->size2_flip_period = 0;
    self->kind = 0;
    self->pattern = 0;
    self->list_flags = 1;
    self->arrived = 0;
    self->alpha_keys_set_speed = 0;
    self->alpha_keys_set_alpha = 0;
    self->alpha_keys_set_size = 0;
    self->fading_out = 0;
    self->change_reached = 0;
    self->master = NULL;
    self->pattern_arg = 0;
    self->alpha_key = 0;
    self->frame = 0;
    ZERO_AS_INT(self->size[0].value);
    self->arc_offset = 0;
    self->arc_degrees = 36.0f;
    self->base.roty = 0.0f;
    self->spread_degrees = 180;
    self->scale = 1.0f;
    ZERO_AS_INT(self->radius[0]);
    ZERO_AS_INT(self->radius[1]);
    ZERO_AS_INT(self->size[1].value);
    ZERO_AS_INT(self->size[2].value);
    self->size2_max = 100.0f;
    ZERO_AS_INT(self->speed);
    ZERO_AS_INT(self->scale_speed);
    ZERO_AS_INT(self->size[0].speed);
    ZERO_AS_INT(self->radius_speed[1]);
    ZERO_AS_INT(self->radius_speed[0]);
    ZERO_AS_INT(self->angle_speed[0]);
    ZERO_AS_INT(self->angle_speed[1]);
    ZERO_AS_INT(self->angle_speed[2]);
    ZERO_AS_INT(self->size[1].speed);
    ZERO_AS_INT(self->size[2].speed);
    self->alpha_speed = 0.0f;
    ZERO_AS_INT(self->fall_speed);
    ZERO_AS_INT(self->accel);
    ZERO_AS_INT(self->scale_accel);
    ZERO_AS_INT(self->size[0].accel);
    ZERO_AS_INT(self->gravity);
    ZERO_AS_INT(self->angle_accel[0]);
    ZERO_AS_INT(self->angle_accel[1]);
    ZERO_AS_INT(self->angle_accel[2]);
    ZERO_AS_INT(self->radius_accel[1]);
    ZERO_AS_INT(self->radius_accel[0]);
    ZERO_AS_INT(self->size[1].accel);
    ZERO_AS_INT(self->size[2].accel);
    self->r = 255;
    self->g = 255;
    self->b = 255;
    self->r_speed = 0;
    self->g_speed = 0;
    self->b_speed = 0;
    self->change_frame = -1;
    self->next_size1_speed = PRIM_UNSET;
    self->next_size2_speed = PRIM_UNSET;
    self->next_speed = PRIM_UNSET;
    memset(&self->next_velocity, 0, 2 * sizeof(Vec3));
    self->next_size1_accel = PRIM_UNSET;
    self->next_size2_accel = PRIM_UNSET;
    self->next_accel = PRIM_UNSET;
    self->color_hold_count = 0;
    self->color_hold_frames = 0;
    self->endless = 0;
    memset(&self->velocity, 0, sizeof(Vec3));
    memset(&self->delta, 0, 4 * sizeof(Vec3));
    ZERO_AS_INT(self->angle[0]);
    ZERO_AS_INT(self->angle[1]);
    ZERO_AS_INT(self->angle[2]);
    self->duration = 100;
    self->start_frame = -1;
    self->fade_frame = -1;
    self->argb = 0xffffffff;
    self->alpha = 254.0f;
    self->fade_scale = 1.0f;
    self->alpha_max = 254.0f;
    self->alpha_min = 0.0f;
    self->loop_motion = 1;
    self->particle_count = 1;
    self->motion_period = 4;
    self->base.action = 0;
    self->base.motion = 0;
    self->motion_count = 1;
    self->textures = NULL;
    ZERO_AS_INT(self->tex_v_top);
    self->tex_u_left = 0;
    self->tex_v_bottom = 1.0f;
    self->tex_u_right = 1.0f;
    memcpy(self->matrix, g_identityMatrix.m, sizeof(self->matrix));
    memcpy(self->matrix2, g_identityMatrix.m, sizeof(self->matrix2));
    self->use_list_flags = 0;
    self->variant = 0;
    self->camera_yaw = 0;
    self->camera_pitch = 0;
    self->master_sprite_lift = 0;
    return self;
}

struct PrimParticle *prim_particle_construct(struct PrimParticle *p)
{
    memset(&p->pos, 0, sizeof p->pos);
    memset(p->corners, 0, sizeof p->corners);
    return p;
}

struct PrimTriangle *prim_triangle_construct(struct PrimTriangle *t)
{
    int i;

    for (i = 0; i < 3; i++) {
        memset(&t->v[i], 0, sizeof(t->v[i]));
        t->v[i].specular = 0xff000000;
    }
    return t;
}

void effect_prim_destruct(struct EffectPrim *self)
{
    self->base.ops = &effect_prim_ops.base;
    if (self->master)
        rag_effect_send_msg(self->master, &self->base, RAG_MSG_PRIM_GONE, 0, 0, 0);
    if (self->textures)
        mem_free(self->textures);
    vec_free(&self->alpha_key_floors);
    vec_free(&self->alpha_key_ceilings);
    vec_free(&self->alpha_key_values);
    vec_free(&self->alpha_key_frames);
    render_object_dc_render_object(&self->base);
}

void effect_prim_delete(struct GameObject *object)
{
    struct EffectPrim *self = (struct EffectPrim *)object;

    effect_prim_destruct(self);
    mem_free(self);
}

void effect_prim_init(struct EffectPrim *self, struct RagEffect *master, int kind,
                                  Vec3 delta)
{
    self->master = master;
    self->delta.x += delta.x;
    self->delta.y += delta.y;
    self->delta.z += delta.z;
    self->kind = kind;
    self->master_pos = master->base.pos;
    self->base.pos = self->master_pos;
    self->stage = 0;
}

void effect_prim_send_msg(struct EffectPrim *self, RenderMsgArg msg)
{
    if (msg == PRIM_MSG_MASTER_GONE)
        self->master = NULL;
}

unsigned char prim_step_motion(struct EffectPrim *self)
{
    void *actres = render_object_get_act_res(&self->base);
    int motions;

    if (self->frame % self->motion_period != 0)
        return 1;
    self->base.motion++;
    if (self->loop_motion == 0) {
        unsigned hold = self->pattern & PRIM_PATTERN_HOLD_MOTION;
        if (self->base.motion == (int)actres_frame_count(actres, self->base.action)) {
            if (!hold)
                return 0;
            self->base.motion = self->base.motion - 1;
        }
        return 1;
    }
    motions = (int)actres_frame_count(actres, self->base.action);
    self->base.motion = self->base.motion % motions;
    return 1;
}

unsigned char prim_step_motion_counted(struct EffectPrim *self)
{
    int motion;

    if (self->frame % self->motion_period != 0)
        return 1;
    motion = self->base.motion + 1;
    self->base.motion = motion;
    if (self->loop_motion == 0) {
        if (motion == self->motion_count)
            return 0;
    } else {
        self->base.motion = motion % self->motion_count;
    }
    return 1;
}

void prim_master_color(struct EffectPrim *self, unsigned char *r, unsigned char *g,
                                  unsigned char *b)
{
    if (self->master->color_r != -1)
        *r = (unsigned char)self->master->color_r;
    if (self->master->color_g != -1)
        *g = (unsigned char)self->master->color_g;
    if (self->master->color_b != -1)
        *b = (unsigned char)self->master->color_b;
}

void prim_heading_degrees(float x, float z, short *out)
{
    float length = float_sqrt(x * x + z * z);
    float unit_z = z / length;
    float unit_x = x / length;
    short deg;

    for (deg = 0; deg <= 180; deg++) {
        if (unit_x >= g_cosTable[deg]) {
            unsigned short heading = (unsigned short)deg;
            if (!(unit_z >= 0.0f))
                heading = (unsigned short)(360 - heading);
            *out = (short)heading;
            return;
        }
    }
    *out = 180;
}

#define COPY_AS_INT(dst, src) (*(int *)&(dst) = *(const int *)&(src))

void prim_sync_particles(struct EffectPrim *self)
{
    int i;

    for (i = 0; i < self->particle_count; i++) {
        struct PrimParticle *p = &self->particles[i];
        p->pos = self->base.pos;
        memcpy(p->corners, self->corners, sizeof(p->corners));
        COPY_AS_INT(p->size, self->size[0].value);
        p->scale = self->scale;
        COPY_AS_INT(p->heading, self->angle[0]);
        memcpy(p->matrix, self->matrix, sizeof(p->matrix));
        p->alpha = self->alpha;
        p->r = self->r;
        p->g = self->g;
        p->b = self->b;
        p->argb = prim_argb(self->alpha, self->r, self->g, self->b);
    }
}

void prim_shift_trail(struct EffectPrim *self)
{
    struct PrimParticle *head = &self->particles[0];
    int i;

    for (i = self->particle_count - 1; i > 0; i--) {
        struct PrimParticle *p = &self->particles[i];
        const struct PrimParticle *prev = &self->particles[i - 1];
        float back = (float)i;
        float alpha, scale;

        p->pos = prev->pos;
        memcpy(p->corners, prev->corners, sizeof(p->corners));
        COPY_AS_INT(p->size, prev->size);
        COPY_AS_INT(p->heading, prev->heading);
        memcpy(p->matrix, prev->matrix, sizeof(p->matrix));
        alpha = self->alpha;
        alpha = alpha - alpha / (float)self->particle_count * back;
        p->alpha = alpha;
        p->argb = prim_argb(alpha, self->r, self->g, self->b);
        scale = self->scale;
        p->scale = scale - scale / (float)self->particle_count * back * 0.5f;
    }

    head->pos = self->base.pos;
    memcpy(head->corners, self->corners, sizeof(head->corners));
    COPY_AS_INT(head->size, self->size[0].value);
    head->scale = self->scale;
    COPY_AS_INT(head->heading, self->angle[0]);
    memcpy(head->matrix, self->matrix, sizeof(head->matrix));
    head->alpha = self->alpha;
    head->argb = prim_argb(self->alpha, self->r, self->g, self->b);
}

void prim_update_alpha(struct EffectPrim *self)
{
    const float *frames = (const float *)self->alpha_key_frames.first;
    float speed, next;

    if (self->alpha_keys_set_alpha) {
        int key = self->alpha_key;
        int key_count = (int)((const float *)self->alpha_key_frames.last - frames);
        if (key < 0 || (unsigned)key > (unsigned)(key_count - 1)) {
            self->alpha_key = 0;
            key = 0;
        }
        if ((float)self->frame == frames[key]) {
            self->alpha = ((const float *)self->alpha_key_values.first)[key];
            self->alpha_max = ((const float *)self->alpha_key_ceilings.first)[key];
            self->alpha_key = key + 1;
        }
    } else if (self->alpha_keys_set_speed) {
        int key = self->alpha_key;
        int key_count = (int)((const float *)self->alpha_key_frames.last - frames);
        if (key < 0 || (unsigned)key >= (unsigned)key_count) {
            self->alpha_key = 0;
            key = 0;
        }
        if ((float)self->frame == frames[key]) {
            self->alpha_speed = ((const float *)self->alpha_key_values.first)[key];
            self->alpha_key = key + 1;
        }
    }

    if (self->pattern & PRIM_PATTERN_ALPHA_BOUNCE) {
        speed = self->alpha_speed;
        if (0.0f > speed) {
            if (self->alpha <= self->alpha_min)
                self->alpha_speed = speed * -1.0f;
        } else {
            if (self->alpha >= self->alpha_max)
                self->alpha_speed = speed * -1.0f;
        }
    }

    if (!self->fading_out) {
        const struct RagEffect *master = self->master;
        if (self->duration - self->frame > master->duration - master->frame + 1) {
            self->fading_out = 1;
            self->pattern &= ~PRIM_PATTERN_ALPHA_BOUNCE;
            self->alpha_speed = -(self->alpha / (float)(master->duration - master->frame));
        }
    }

    if (self->frame == self->fade_frame) {
        self->pattern &= ~PRIM_PATTERN_ALPHA_BOUNCE;
        speed = -(self->alpha / (float)(self->duration - self->frame) * self->fade_scale);
        self->alpha_speed = speed;
    } else {
        speed = self->alpha_speed;
    }
    next = speed + self->alpha;
    next = self->alpha_min > next ? self->alpha_min : next;
    next = self->alpha_max < next ? self->alpha_max : next;
    self->alpha = next;
    self->argb = prim_argb(next, self->r, self->g, self->b);
}

void prim_quad_turn(Vec3 *p0, Vec3 *p1, Vec3 *p2, Vec3 *p3, short yaw, short pitch)
{
    Vec3 *p[4];
    int i;

    p[0] = p0;
    p[1] = p1;
    p[2] = p2;
    p[3] = p3;
    for (i = 0; i < 4; i++) {
        float y = p[i]->y, z = p[i]->z;
        p[i]->z = g_cosTable[pitch] * z - g_sinTable[pitch] * y;
        p[i]->y = g_cosTable[pitch] * y + g_sinTable[pitch] * z;
    }
    for (i = 0; i < 4; i++) {
        float x = p[i]->x, z = p[i]->z;
        p[i]->x = g_cosTable[yaw] * x - z * g_sinTable[yaw];
        p[i]->z = g_cosTable[yaw] * z + x * g_sinTable[yaw];
    }
}

static int actor_state(struct RenderObject *actor)
{
    return render_object_ops_of(actor)->get_effect_state(actor);
}

unsigned char prim_is_visible(struct EffectPrim *self)
{
    struct RagEffect *effect = self->master;
    struct RenderObject *owner, *actor;
    int aid;

    if (!effect)
        return 1;
    if (g_session1.effectOn == 0 && effect->always_draw == 0)
        return 0;
    owner = effect->master;
    if (!owner)
        return 1;
    aid = owner->aid;
    if (aid == 0 || aid == g_session1.ownAid)
        return 1;
    actor = (struct RenderObject *)game_find_actor(aid);
    if (!actor)
        return 1;
    if (((struct GameActor *)actor)->object_type != 0)
        return 1;
    if (game_map_preserves_party_effects() && session_is_party_member_aid(&g_session1, aid, 1) == 1)
        return 1;
    if (actor->visible == 0 || actor->revealed == 0)
        return 0;
    if (is_effect_state_star2(actor_state(actor)))
        return 0;

    if (self->master->effect_id != 0xe8)
        return 1;
    aid = (int)(unsigned)self->master->offset.y;
    if (aid == g_session1.ownAid)
        return 1;
    actor = (struct RenderObject *)game_find_actor(aid);
    if (!actor)
        return 1;
    if (game_map_preserves_party_effects() && session_is_party_member_aid(&g_session1, aid, 1) == 1)
        return 1;
    if (actor->visible == 0 || actor->revealed == 0)
        return 0;
    return is_effect_state_star2(actor_state(actor)) ? 0 : 1;
}
