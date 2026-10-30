#include "render/texture_cache.h"
#include "session/state.h"
#include "world/actor_lookup.h"
#include "game/context.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define EFFECT_DIR "이팩트\\"

static struct Texture *texture(const char *name)
{
    return texture_cache_get(&g_textureCache, name, 0, 0);
}

static struct EffectPrim *launch_prim(struct RagEffect *self, int kind, int motions)
{
    struct EffectPrim *p = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    self->last_prim = p;
    p->list_flags = 5;
    p->duration = self->duration;
    if (motions != 0)
        p->motion_count = motions;
    p->textures = rag_prim_new_textures(p);
    return p;
}

static void follow_master(struct RagEffect *self)
{
    self->base.pos = self->master->pos;
}

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static int start_frame(int mode)
{
    if ((char)mode == 0)
        return 35;
    return (short)((char)mode == 1 ? 0 : mode);
}

static void light_box_sink_at_end(struct RagEffect *self, struct PrimLightBox *lb, char style)
{
    if ((unsigned char)(style - 10) <= 9)
        lb->sink_frame = (short)(self->duration > 30000 ? 30000 : self->duration - 90);
}

void rag_styled_rings(struct RagEffect *self, const char *tex, char style)
{
    struct EffectPrim *p;
    struct PrimLightBox *lb;
    int tall = style == 0 || style == 10 || style == 20;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_prim(self, 90, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    p->textures = rag_prim_new_textures(p);
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    for (i = 0; i < 2; i++) {
        lb = &p->records.light_boxes[i];
        lb->active = 1;
        lb->frame = 0;
        lb->height_max = (float)i + (tall ? 30.0f : 20.0f);
        if (style == 0 || style == 10) {
            lb->alpha = 65;
            lb->sink_frame = 2415;
            light_box_sink_at_end(self, lb, style);
        } else if (style == 20) {
            lb->alpha = 65;
            lb->sink_frame = 2170;
        } else {
            lb->alpha = 32;
            if (style == 21) {
                lb->sink_frame = 2140;
            } else {
                lb->sink_frame = 2385;
                light_box_sink_at_end(self, lb, style);
            }
        }
        if (style == 10 || style == 20)
            lb->half_x = (float)i * 0.4f + 7.0f;
        else if (style == 11 || style == 21)
            lb->half_x = (float)i * 0.4f + 8.5f;
        else if (style == 0)
            lb->half_x = (float)i * 0.4f + 12.5f;
        else
            lb->half_x = (float)i * 0.4f + 14.0f;
        lb->half_z = lb->half_x;
        lb->style = (float)style;
    }
    for (i = 2; i < 4; i++) {
        lb = &p->records.light_boxes[i];
        lb->active = 1;
        lb->frame = 0;
        lb->height_max = (float)i + (tall ? 30.0f : 20.0f);
        if (style == 10 || style == 20)
            lb->run = (float)(i - 2) * 30.2f;
        else if (style == 11 || style == 21)
            lb->run = (float)(i - 2) * 30.2f + 15.1f;
        else if (style == 0)
            lb->run = (float)(i - 2) * 53.2f;
        else
            lb->run = (float)(i - 2) * 53.2f + 26.6f;
        if (style == 10 || style == 20)
            lb->half_x = 7.8f;
        else if (style == 11 || style == 21)
            lb->half_x = 9.3f;
        else if (style == 0)
            lb->half_x = 13.3f;
        else
            lb->half_x = 14.8f;
        lb->half_z = lb->half_x;
        lb->run_len = style >= 10 ? 18.6f : 26.6f;
        lb->alpha = 15;
        lb->sink_frame = style < 20 ? 2415 : 2170;
        light_box_sink_at_end(self, lb, style);
        lb->style = (float)style;
    }
}

static void stream_turn(struct EffectPrim *p, int angle)
{
    const float *m = p->matrix;

    p->angle[0] = -p->base.roty;
    p->speed = p->size[0].value / (float)p->duration + p->size[0].value / (float)p->duration;
    p->fall_speed = 2.0f;
    p->gravity = p->fall_speed / (float)p->duration * -2.0f;
    p->angle_speed[1] = -2.0f;
    p->angle_accel[1] = p->angle_speed[1] / (float)p->duration * -2.0f;
    p->angle[2] = (float)angle;
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->drift.x = m[3] * 0.0f + m[0] * 0.0f + m[6] + m[9];
    p->drift.y = m[4] * 0.0f + m[1] * 0.0f + m[7] + m[10];
    p->drift.z = m[5] * 0.0f + m[2] * 0.0f + m[8] + m[11];
    p->swing_axis.x = p->drift.x;
    p->swing_axis.y = p->drift.y;
    p->swing_axis.z = p->drift.z;
}

static void stream_sprite(struct EffectPrim *p)
{
    render_object_load_spr_res(&p->base, EFFECT_DIR "particle1.spr");
    render_object_load_act_res(&p->base, EFFECT_DIR "particle1.act");
    p->particle_count = 7;
    prim_sync_particles(p);
}

void rag_particle_streams(struct RagEffect *self)
{
    struct EffectPrim *p;
    int angle;

    if (self->frame != 0)
        return;
    for (angle = -90; angle < 180; angle += 90) {
        p = rag_launch_prim(self, 8, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        self->last_prim = p;
        p->duration = 60;
        p->delta2.x = self->offset.x - self->base.pos.x;
        p->delta2.z = self->offset.z - self->base.pos.z;
        p->delta2.y = -10.0f;
        p->size[0].value = render_object_calc_ground_dist(&p->base,
                                                         self->offset.x - (self->offset.x - self->base.pos.x) * 0.5f,
                                                         self->offset.z - (self->offset.z - self->base.pos.z) * 0.5f);
        render_object_turn_toward(&p->base, self->base.pos.x + self->base.pos.x - self->offset.x,
                   self->base.pos.z + self->base.pos.z - self->offset.z);
        stream_turn(p, angle);
        p->r = rand_below(81) + 160;
        p->g = rand_below(81) + 160;
        p->b = 255;
        p->scale = (float)rand_below(6) * 0.1f + 1.5f;
        stream_sprite(p);
    }
}

void rag_particle_streams_down(struct RagEffect *self, char colour)
{
    struct EffectPrim *p;
    int angle, rx, rz, grey;

    if (self->frame > 4)
        return;
    for (angle = -90; angle < 180; angle += 90) {
        p = rag_launch_prim(self, 8, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        self->last_prim = p;
        p->duration = 60;
        p->delta2.x = 0.0f;
        p->delta2.y = -10.0f;
        p->delta2.z = 0.0f;
        p->size[0].value = render_object_calc_ground_dist(&p->base, self->base.pos.x, self->base.pos.z);
        rz = rand_below(7);
        rx = rand_below(7);
        render_object_turn_toward(&p->base, (float)rx + self->base.pos.x - 3.0f, (float)rz + self->base.pos.z - 3.0f);
        stream_turn(p, angle);
        if (colour == 0) {
            p->r = rand_below(81) + 160;
            p->g = 255;
            p->b = rand_below(81) + 160;
        } else if (colour == 1) {
            grey = rand_below(55) + 200;
            p->r = grey;
            p->g = grey;
            p->b = grey;
        } else if (colour == 2) {
            grey = rand_below(100) + 20;
            p->r = grey;
            p->g = grey;
            p->b = grey;
        }
        p->scale = (float)rand_below(6) * 0.1f + 1.0f;
        stream_sprite(p);
    }
}

static void actor_link(struct RagEffect *self, float radius, float style)
{
    struct EffectPrim *p;
    struct PrimTube *tube;
    void *actor = NULL;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 38, 0);
    if (!p)
        return;
    p->textures[0] = texture("effect\\alpha_center.tga");
    tube = &p->records.tube;
    tube->active = 1;
    tube->age = 0;
    tube->alpha = 0;
    tube->radius = radius;
    tube->style = style;
    tube->start.x = self->base.pos.x;
    tube->start.y = self->base.pos.y;
    tube->start.z = self->base.pos.z;
    tube->target = self->offset.y;
    if (tube->target > 0.0f) {
        if (tube->target == (float)(double)(unsigned)g_session1.ownAid)
            actor = game_current_player();
        else
            actor = game_find_actor((int)(__int64)tube->target);
    }
    if (actor != NULL) {
        tube->end.x = ACTOR_POS(actor)->x;
        tube->end.y = ACTOR_POS(actor)->y;
        tube->end.z = ACTOR_POS(actor)->z;
    } else {
        tube->active = 0;
    }
}

void rag_actor_link(struct RagEffect *self)
{
    actor_link(self, 0.2f, 2.0f);
}

void rag_actor_link_wide(struct RagEffect *self)
{
    actor_link(self, 0.7f, 3.0f);
}

static void dashes_to_offset(struct RagEffect *self, int wide)
{
    struct EffectPrim *p;
    struct PrimPipe *pipe;
    float dz, dx, len, back;
    int i;

    p = launch_prim(self, 69, 0);
    if (!p)
        return;
    p->textures[0] = texture("effect\\alpha_center.tga");
    p->scale = 0.0f;
    p->delta = self->offset;
    dz = self->offset.z - self->base.pos.z;
    dx = self->offset.x - self->base.pos.x;
    len = float_sqrt(dz * dz + dx * dx);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        pipe = &p->records.pipes[i];
        pipe->active = 1;
        pipe->age = wide ? -30 - rand_below(110) : -45 - rand_below(20);
        prim_heading_degrees(dx, dz, &pipe->heading);
        pipe->alpha = 0;
        pipe->start.x = (float)rand_below(100) * 0.08f - 4.0f + self->base.pos.x;
        pipe->start.z = (float)rand_below(100) * 0.08f - 4.0f + self->base.pos.z;
        pipe->start.y = self->base.pos.y;
        back = (float)rand_below(100) * 0.15f;
        pipe->start.x -= g_cosTable[pipe->heading] * back;
        pipe->start.z -= g_sinTable[pipe->heading] * back;
        pipe->end.x = pipe->start.x;
        pipe->end.z = pipe->start.z;
        pipe->end.y = self->base.pos.y;
        if (wide) {
            pipe->radius = (float)rand_below(11) * 0.008f + 0.02f;
            pipe->drop = (float)rand_below(100) * 0.06f + 4.0f;
            pipe->length = (float)rand_below(100) * 0.3f + len;
            pipe->style = 3.0f;
        } else {
            pipe->radius = 0.05f;
            pipe->drop = (float)rand_below(100) * 0.04f + 8.0f;
            pipe->length = (float)rand_below(100) * 0.6f + len;
            pipe->style = 2.0f;
        }
    }
}

void rag_dashes_to_offset(struct RagEffect *self)
{
    follow_master(self);
    if (self->frame > 3)
        return;
    dashes_to_offset(self, 0);
}

void rag_dashes_to_offset_long(struct RagEffect *self)
{
    follow_master(self);
    if (self->frame > 8)
        return;
    dashes_to_offset(self, 1);
}

void rag_cloud_swirl(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    struct PrimRing *ring;
    float fi;
    int i, k;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 62, 0);
    if (!p)
        return;
    p->textures[0] = texture("effect\\cloud11.tga");
    p->scale = look == 1 ? 8.0f : 5.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ring = &p->records.rings[i];
        fi = (float)i;
        ring->active = 1;
        ring->age = 0;
        ring->span = 30;
        if (look == 1)
            ring->height = fi + fi + 3.0f;
        else if (look == 2)
            ring->height = 48.0f - fi * 5.0f;
        else if (look == 3)
            ring->height = 28.0f - fi * 5.0f;
        else
            ring->height = fi + fi + 5.0f;
        ring->turn = (short)(i * 90);
        ring->alpha = 0;
        ring->mode = (short)((look >= 2) + 1);
        if (look == 1)
            ring->radius = (float)rand_below(101) * 0.01f + 2.5f;
        else if (look == 2)
            ring->radius = 14.0f - (fi + fi);
        else if (look == 3)
            ring->radius = 6.0f - fi;
        else
            ring->radius = (float)rand_below(101) * 0.03f + 4.5f;
        ring->tilt = (short)(rand_below(21) + 80);
        for (k = 0; k < PRIM_RING_COLUMNS; k++) {
            ring->column[k] = 1.0f;
            ring->column_flag[k] = 0;
        }
    }
}

void rag_rising_bits(struct RagEffect *self, const char *tex, int angle)
{
    struct EffectPrim *p;
    struct PrimRing *ring;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 91, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 10.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ring = &p->records.rings[i];
        ring->active = 1;
        ring->age = -rand_below(20);
        ring->height = (float)rand_below(11) * 0.2f + 2.0f;
        ring->radius = 5.0f;
        ring->column[0] = -10.0f;
        ring->turn = (short)rand_below(360);
        ring->tilt = (short)(i * 45 + angle);
        ring->alpha = 0;
        ring->span = (short)(rand_below(91) + 180);
        ring->column[1] = (float)(rand_below(4) + 3);
    }
}

void rag_purple_slashes(struct RagEffect *self, int angle)
{
    struct EffectPrim *p;
    struct PrimTurnedSquare *sq;
    int first_age, i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 92, 1);
    if (!p)
        return;
    p->textures[0] = texture("effect\\purpleslash.tga");
    p->scale = 0.0f;
    first_age = -28 - rand_below(6);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.turned_squares[i];
        sq->active = 1;
        sq->age = first_age + i;
        sq->radius = 20.0f;
        sq->alpha = 0;
        sq->pos.x = self->base.pos.x;
        sq->pos.z = self->base.pos.z;
        sq->pos.y = (float)rand_below(5) + (self->base.pos.y - 10.0f);
        sq->turn = (short)angle;
        sq->speed = 0.8f;
        sq->style = 0;
    }
}

void rag_view_facing_sheet(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimRainbow *rb;

    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 93, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    rb = &p->records.rainbows[0];
    rb->view_yaw = (short)(int)game_camera_heading();
    rb->view_yaw = (short)(360 - rb->view_yaw);
    rb->active = 1;
    rb->age = 0;
    rb->alpha = 0;
    rb->radius = 50.0f;
    rb->arc = 180;
    rb->start = 0;
    rb->yaw = rb->view_yaw;
}

void rag_spray_at_angle(struct RagEffect *self, const char *tex, short angle, int mode)
{
    struct EffectPrim *p;
    struct PrimDriftSquare *sq;
    float c, s;
    short heading;
    int i;

    if (self->frame != start_frame(mode))
        return;
    p = launch_prim(self, 94, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.drift_squares[i];
        sq->active = 1;
        sq->size = (float)rand_below(11) * 0.2f + 2.5f;
        sq->alpha = 0;
        heading = (short)(rand_below(90) + angle);
        if (heading >= 360)
            heading = (short)(heading - 360);
        c = g_cosTable[heading];
        s = g_sinTable[heading];
        sq->frame = -rand_below(11);
        sq->heading = heading;
        sq->fall = 0.03f;
        sq->centre.x = c * 4.0f;
        sq->centre.z = s * 4.0f;
        sq->centre.y = 0.0f;
        sq->base_y = -4.0f;
        sq->drift_x = (float)rand_below(360);
        sq->drift_x_to = (float)rand_below(360);
        sq->drift_z = (float)rand_below(360);
        sq->drift_z_to = (float)rand_below(360);
    }
}

void rag_popping_bits(struct RagEffect *self, const char *tex, int mode)
{
    struct EffectPrim *p;
    struct PrimDriftSquare *sq;
    int a, i;

    if (self->frame != start_frame(mode))
        return;
    p = launch_prim(self, 96, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    if ((char)mode == 0) {
        sq = &p->records.drift_squares[0];
        sq->active = 1;
        sq->fall = (float)rand_below(16) * 0.01f + 0.1f;
        sq->size = (float)rand_below(11) * 0.1f + 3.0f;
        sq->frame = -rand_below(26);
        sq->alpha = 0;
        sq->centre.y = (float)-rand_below(8);
        a = rand_below(360);
        sq->centre.x = g_cosTable[a] * 2.5f;
        sq->centre.z = g_sinTable[a] * 2.5f;
        sq->drift_x = (float)rand_below(360);
        sq->drift_x_to = (float)rand_below(360);
        sq->drift_z = (float)rand_below(360);
        sq->drift_z_to = (float)rand_below(360);
        sq->tint = 1;
        return;
    }
    if ((char)mode != 1)
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.drift_squares[i];
        sq->active = 1;
        sq->fall = (float)rand_below(11) * 0.005f + 0.08f;
        sq->size = (float)rand_below(11) * 0.05f + 2.0f;
        sq->frame = -rand_below(26);
        sq->alpha = 0;
        sq->centre.y = (float)-rand_below(8);
        a = rand_below(360);
        sq->centre.x = g_cosTable[a] * 3.5f;
        sq->centre.z = g_sinTable[a] * 3.5f;
        sq->drift_x = (float)rand_below(360);
        sq->drift_x_to = (float)rand_below(360);
        sq->drift_z = (float)rand_below(360);
        sq->drift_z_to = (float)rand_below(360);
        sq->tint = 2;
    }
}

void rag_blue_ivy(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimBillboard *bb;
    float dz, dx;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 97, 0);
    if (!p)
        return;
    p->textures[0] = texture("effect\\blue_ivy.bmp");
    p->scale = 0.0f;
    p->delta = self->offset;
    bb = &p->records.billboards[0];
    dz = self->offset.z - self->base.pos.z;
    dx = self->offset.x - self->base.pos.x;
    bb->active = 1;
    bb->age = -15;
    bb->range = float_sqrt(dz * dz + dx * dx);
    bb->size = (float)self->hit_count * 0.4f + 2.0f;
    bb->phase = 0.0f;
    bb->alpha = 5;
    bb->pos.x = self->base.pos.x;
    bb->pos.z = self->base.pos.z;
    bb->pos.y = self->base.pos.y - 8.0f;
    bb->heading = 0;
    bb->speed = 2.0f;
    bb->climb = bb->speed / bb->range * (self->offset.y - self->base.pos.y);
    bb->spin = 0;
    bb->texture = 0;
    bb->state = 0;
    bb->color_style = 1;
    bb->style = 0.0f;
    p->records.billboards[1].active = 1;
    p->records.billboards[1].alpha = 0;
    p->records.billboards[2].active = 1;
    p->records.billboards[2].alpha = 0;
    p->records.billboards[3].active = 1;
    p->records.billboards[3].alpha = 0;
}

void rag_aimed_texture(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimUprightSquare *sq;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 98, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    sq = &p->records.upright_squares[0];
    sq->active = 1;
    sq->age = 0;
    sq->size = 5.0f;
    sq->alpha = 0;
    sq->centre.x = self->offset.x;
    sq->centre.y = self->offset.y;
    sq->centre.z = self->offset.z;
    rand_below(360);
    prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &sq->yaw);
    sq->yaw += 90;
    if (sq->yaw >= 360)
        sq->yaw -= 360;
    p->records.upright_squares[1].active = 0;
    p->records.upright_squares[1].alpha = 0;
    p->records.upright_squares[2].active = 0;
    p->records.upright_squares[2].alpha = 0;
    p->records.upright_squares[3].active = 0;
    p->records.upright_squares[3].alpha = 0;
}

void rag_super_rings(struct RagEffect *self, char count, char tint)
{
    struct EffectPrim *p;
    struct PrimSparkGroup *g;
    int i, n;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 99, 5);
    if (!p)
        return;
    p->textures[0] = texture("effect\\super1.bmp");
    p->textures[1] = texture("effect\\super2.bmp");
    p->textures[2] = texture("effect\\super3.bmp");
    p->textures[3] = texture("effect\\super4.bmp");
    p->textures[4] = texture("effect\\super5.bmp");
    p->scale = 0.0f;
    n = count;
    for (i = 0; i < n; i++) {
        g = &p->records.spark_groups[i];
        g->frames = 0;
        g->active = 1;
        g->size = 2.0f;
        g->point[2].y = (float)-rand_below(10);
        g->spark[0].age = 10.0f;
        g->spark[0].alpha = 40.0f;
        g->spark[0].texture = (float)rand_below(5);
        g->spark[0].angle = (float)rand_below(360);
        g->point[0].y = (float)-rand_below(10);
        g->spark[1].age = 10.0f;
        g->spark[1].alpha = 80.0f;
        g->spark[1].texture = (float)rand_below(5);
        g->spark[1].angle = (float)rand_below(360);
        g->point[3].y = (float)-rand_below(10);
        g->spark[2].age = 10.0f;
        g->spark[2].alpha = 120.0f;
        g->spark[2].texture = (float)rand_below(5);
        g->spark[2].angle = (float)rand_below(360);
        g->point[1].y = (float)-rand_below(10);
        g->spark[3].age = 10.0f;
        g->spark[3].alpha = 160.0f;
        g->spark[3].texture = (float)rand_below(5);
        g->spark[3].angle = (float)rand_below(360);
        g->tint = (unsigned char)tint;
    }
}

void rag_quake_spin_master(struct RagEffect *self)
{
    if (self->frame == 30)
        game_send_message(0x7a, 0, 0, 0, 0);
    if ((unsigned)(self->frame - 30) <= 24) {
        self->master->roty += 30.0f;
        if (self->master->roty > 360.0f)
            self->master->roty -= 360.0f;
    }
    if ((unsigned)(self->frame - 30) <= 20 && self->frame % 10 < 5)
        self->master->overlay_flags |= 1;
}

void rag_kyrie_guard_flicker(struct RagEffect *self)
{
    if (self->frame == 0) {
        rag_follow_master(self);
        rag_play_wave_here(self, "effect\\kyrie_guard.wav");
    }
    if ((unsigned)self->frame <= 29) {
        self->master->overlay_flags |= 8;
        if (self->frame % 6 < 3)
            render_object_set_argb(self->master, -1, 100, 100, 100);
        else
            render_object_set_argb(self->master, -1, 250, 250, 250);
    }
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
}
