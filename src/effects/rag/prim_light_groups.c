#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/game_actor.h"

#define RECORD_COUNT 4

static const int k_light_slot[4] = {2, 0, 3, 1};

static void keep_camera_angles(struct EffectPrim *self)
{
    short pitch;

    self->camera_yaw = (unsigned short)(360 - (int)game_camera_heading());
    pitch = (short)(450 - (int)game_camera_pitch());
    self->camera_pitch = (unsigned short)pitch;
    if (pitch >= 360)
        self->camera_pitch = (unsigned short)(pitch - 360);
    self->base.pos = self->master->base.pos;
}

static void wander_step(struct PrimSway *sway, float up, float down)
{
    float angle = sway->angle, target = sway->target;
    int range, pick;

    if (target > angle) {
        angle += up;
        if (angle > 359.0f)
            angle = 359.0f;
        sway->angle = angle;
        if (angle > target) {
            range = (int)angle;
            if (range == 0) {
                pick = 0;
            } else {
                pick = rand() % range;
                if (pick < 0)
                    pick = 0;
            }
            sway->target = (float)pick;
        }
    } else {
        angle -= down;
        if (0.0f > angle)
            angle = 0.0f;
        sway->angle = angle;
        if (target >= angle) {
            range = (int)angle;
            if (range == 0) {
                pick = 0;
            } else {
                pick = rand() % range;
                if (pick < 0)
                    pick = 0;
            }
            sway->target = (float)(359 - pick);
        }
    }
}

static int rand_below(int n)
{
    int r = rand() % n;

    return r < 0 ? 0 : r;
}

static Vec3 vec3_add(Vec3 a, Vec3 b)
{
    Vec3 r;

    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static void facing_quad(struct EffectPrim *self, float s, Vec3 *q)
{
    q[0].x = g_cosTable[45] * s;
    q[0].y = 0.0f;
    q[0].z = g_sinTable[45] * s;
    q[1].x = g_cosTable[135] * s;
    q[1].y = 0.0f;
    q[1].z = g_sinTable[135] * s;
    q[2].x = g_cosTable[225] * s;
    q[2].y = 0.0f;
    q[2].z = g_sinTable[225] * s;
    q[3].x = g_cosTable[315] * s;
    q[3].y = 0.0f;
    q[3].z = g_sinTable[315] * s;
    prim_quad_turn(&q[0], &q[1], &q[2], &q[3], self->camera_yaw, self->camera_pitch);
}

#define AURA_WHITE_TWICE  0
#define AURA_BLUE_TWICE   1
#define AURA_YELLOW       3
#define AURA_WHITE        10
#define AURA_GREY_ONE     11
#define AURA_SNOW         14
#define AURA_DRIFT        15
#define AURA_RAIN         16
#define AURA_TINT_PINK    7
#define AURA_TINT_YELLOW  8

static void aura_particle_step(struct PrimAuraGroup *g, int k, int x_up, int z_up)
{
    Vec3 *p = &g->point[k_light_slot[k]];
    int ix = (int)g->sway[2 * k].angle;
    int iz = (int)g->sway[2 * k + 1].angle;
    float y;

    if (g->style == AURA_SNOW) {
        y = p->y;
        if (y > -40.0f) {
            if (k != 3) {
                p->x = g_sinTable[ix] * 0.1f + p->x;
                p->z = p->z - g_sinTable[iz] * 0.1f;
            } else {
                p->x = p->x - g_sinTable[ix] * 0.1f;
                p->z = g_sinTable[iz] * 0.1f + p->z;
            }
        }
        if (y > 0.0f) {
            p->x = 0.0f;
            y = -40.0f;
            p->z = 0.0f;
        }
        p->y = g->speed + y;
    } else if (g->style == AURA_RAIN) {
        y = p->y;
        if (y > 0.0f) {
            int spot = rand_below(35);
            p->z = 0.0f;
            y = -40.0f;
            p->x = (float)(spot - 17);
        }
        p->y = g->speed + y;
    } else if (g->style == AURA_DRIFT) {
        short c;

        p->x = x_up ? g_sinTable[ix] * 0.1f + p->x : p->x - g_sinTable[ix] * 0.1f;
        p->z = z_up ? g_sinTable[iz] * 0.1f + p->z : p->z - g_sinTable[iz] * 0.1f;
        c = (short)(int)p->x;
        if (c < 0)
            c = (short)-c;
        if (c < 10) {
            c = (short)(int)p->z;
            if (c < 0)
                c = (short)-c;
        }
        if (c > 10) {
            g->drift_count[k] += 5;
            if (g->drift_count[k] >= 120) {
                g->drift_count[k] = 0;
                p->x = 0.0f;
                p->z = 0.0f;
            }
        }
    } else {
        y = p->y;
        if (0.0f > y) {
            float top = (k == 0 && g->style == AURA_GREY_ONE) ? -50.0f : -30.0f;

            p->x = x_up ? g_sinTable[ix] * 0.15f + p->x : p->x - g_sinTable[ix] * 0.15f;
            p->z = z_up ? g_sinTable[iz] * 0.15f + p->z : p->z - g_sinTable[iz] * 0.15f;
            if (top > y) {
                if (g->style == AURA_GREY_ONE) {
                    p->x = (float)(rand_below(15) - 8);
                    p->z = (float)(rand_below(15) - 8);
                } else {
                    p->x = 0.0f;
                    p->z = 0.0f;
                }
                y = (float)rand_below(100);
            }
        }
        p->y = y - g->speed;
    }
}

unsigned char prim_aura_particles_process(struct EffectPrim *self)
{
    int i, j;

    keep_camera_angles(self);
    for (i = 0; i < RECORD_COUNT; i++) {
        struct PrimAuraGroup *g = &self->records.aura_groups[i];

        if (g->active != 1)
            continue;
        for (j = 0; j < 8; j++)
            wander_step(&g->sway[j], 3.0f, 2.0f);
        aura_particle_step(g, 0, 1, 1);
        aura_particle_step(g, 1, 0, 0);
        aura_particle_step(g, 2, 1, 0);
        aura_particle_step(g, 3, 0, 1);
    }
    return 1;
}

void prim_aura_particles_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i, k;

    for (i = 0; i < RECORD_COUNT; i++) {
        struct PrimAuraGroup *g = &self->records.aura_groups[i];
        Vec3 q[4];

        if (g->active != 1)
            continue;
        facing_quad(self, g->size, q);
        for (k = 0; k < 4; k++) {
            Vec3 c = g->point[k_light_slot[k]];
            Vec3 p0 = vec3_add(c, q[0]);
            Vec3 p1 = vec3_add(c, q[1]);
            Vec3 p2 = vec3_add(c, q[2]);
            Vec3 p3 = vec3_add(c, q[3]);
            signed char style = g->style;
            float y = p0.y;
            int alpha;

            if (style <= AURA_BLUE_TWICE || (k != 0 && style == AURA_GREY_ONE)) {
                alpha = 250;
                if (-20.0f > y)
                    alpha = (int)((y + 20.0f) * 25.0f + 250.0f);
            } else if (style == AURA_GREY_ONE) {
                alpha = 250;
                if (-40.0f > y)
                    alpha = (int)((y + 40.0f) * 25.0f + 250.0f);
            } else if (style == AURA_SNOW || style == AURA_RAIN) {
                alpha = 100;
                if (y > -10.0f)
                    alpha = (int)(150.0f - (y + 10.0f) * 15.0f);
                else if (-40.0f > y)
                    alpha = 0;
            } else if (style == AURA_DRIFT) {
                alpha = 255 - 2 * g->drift_count[k];
            } else {
                alpha = 50;
                if (-4.0f > y)
                    alpha = (int)((y + 4.0f) * 15.0f + 50.0f);
                if (y > -0.5f)
                    alpha = (int)(y * -100.0f);
            }
            if (alpha < 0)
                alpha = 0;

#define AURA_QUAD(r, g_, b) \
    prim_draw_quad(self, &count, (short)alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0, (r), (g_), (b), \
                   0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED)
            if (style == AURA_DRIFT) {
                if (g->tint == AURA_TINT_PINK)
                    AURA_QUAD(0xfa, 0x46, 0x64);
                else if (g->tint == AURA_TINT_YELLOW)
                    AURA_QUAD(0xfa, 0xfa, 0x0a);
                else
                    AURA_QUAD(0x46, 0x78, 0x64);
            } else if (0.0f >= y) {
                if (style == AURA_WHITE_TWICE) {
                    AURA_QUAD(0xff, 0xff, 0xff);
                    AURA_QUAD(0xff, 0xff, 0xff);
                } else if (style == AURA_BLUE_TWICE) {
                    AURA_QUAD(0x50, 0x50, 0xff);
                    AURA_QUAD(0x50, 0x50, 0xff);
                } else if (style == AURA_YELLOW) {
                    AURA_QUAD(0xff, 0xff, 0x37);
                } else if (style == AURA_WHITE || style == AURA_SNOW) {
                    AURA_QUAD(0xff, 0xff, 0xff);
                } else if (style == AURA_GREY_ONE) {
                    if (k == 0)
                        AURA_QUAD(0x9b, 0x9b, 0x9b);
                } else {
                    AURA_QUAD(0x46, 0x78, 0x64);
                }
            }
#undef AURA_QUAD
        }
    }
}

#define ORB_BROWN       1
#define ORB_STAGED      10
#define ORB_RED         10
#define ORB_YELLOW      11
#define ORB_BLUE        12
#define ORB_WHITE       13
#define ORB_HALO        20
#define ORB_DOUBLE_HALO 21

static void orb_spin(struct PrimGlowOrb *o, float step)
{
    float s = o->spin + step;

    o->spin = s;
    if (s >= 360.0f)
        o->spin = s - 360.0f;
}

unsigned char prim_glow_orb_process(struct EffectPrim *self)
{
    struct PrimGlowOrb *o = &self->records.glow_orb;
    signed char style;
    float shimmer;
    int j;

    keep_camera_angles(self);
    if (o->active != 1)
        return 1;
    if (o->frames++ < 200)
        o->brightness++;
    for (j = 0; j < 3; j++)
        wander_step(&o->sway[j], 1.0f, 1.0f);

    style = o->style;
    if (style != 0 && style < ORB_HALO) {
        if (style >= ORB_STAGED) {
            if (self->stage == 0) {
                if (o->frames % (style - 9) == 0)
                    orb_spin(o, 1.0f);
                o->size = 6.0f;
                o->height = -15.0f;
            } else if (self->stage == 1) {
                if (o->frames % (style - 9) == 0)
                    orb_spin(o, 15.0f);
                o->height = o->height - 0.1f;
                if (-30.0f >= o->height)
                    o->height = -30.0f;
                o->size = o->size + 0.02f;
                if (o->size >= 10.0f)
                    o->size = 10.0f;
            }
        } else if (!(o->frames & 1)) {
            orb_spin(o, 1.0f);
        }
    } else {
        orb_spin(o, 1.0f);
    }

    if (o->close_orbit == 1.0f) {
        float x = g_sinTable[(int)o->sway[0].angle] * 0.7f;
        float z = g_sinTable[(int)o->sway[1].angle] * 0.7f;
        o->point.x = x;
        o->point.z = z;
        o->point.y = g_sinTable[(int)o->sway[2].angle] * 0.7f;
        o->point.x = g_sinTable[(int)o->spin] * 5.5f + x;
        o->point.z = g_cosTable[(int)o->spin] * 5.5f + z;
    } else {
        if (o->style != 0 && o->style < ORB_HALO) {
            o->point.x = g_sinTable[(int)o->sway[0].angle] * 0.5f;
            o->point.z = g_sinTable[(int)o->sway[1].angle] * 0.5f;
            o->point.y = g_sinTable[(int)o->sway[2].angle] * 0.5f;
        } else {
            o->point.x = g_sinTable[(int)o->sway[0].angle];
            o->point.z = g_sinTable[(int)o->sway[1].angle];
            o->point.y = g_sinTable[(int)o->sway[2].angle];
        }
        o->point.x = g_sinTable[(int)o->spin] * 7.0f + o->point.x;
        o->point.z = g_cosTable[(int)o->spin] * 7.0f + o->point.z;
    }
    o->point.y = o->height + o->point.y;

    shimmer = (float)(rand_below(5) + 5) + o->shimmer;
    o->shimmer = shimmer;
    if (shimmer >= 360.0f)
        o->shimmer = shimmer - 360.0f;
    return 1;
}

static void orb_ring(struct EffectPrim *self, const float *view, int *count, float r, short alpha, short blend,
                     unsigned char red, unsigned char green, unsigned char blue)
{
    struct PrimGlowOrb *o = &self->records.glow_orb;
    Vec3 q[4];

    q[0].x = g_cosTable[0] * r;
    q[0].y = 0.0f;
    q[0].z = g_sinTable[0] * r;
    q[1].x = g_cosTable[90] * r;
    q[1].y = 0.0f;
    q[1].z = g_sinTable[90] * r;
    q[2].x = g_cosTable[180] * r;
    q[2].y = 0.0f;
    q[2].z = g_sinTable[180] * r;
    q[3].x = g_cosTable[270] * r;
    q[3].y = 0.0f;
    q[3].z = g_sinTable[270] * r;
    prim_quad_turn(&q[0], &q[1], &q[2], &q[3], self->camera_yaw, self->camera_pitch);
    prim_draw_quad(self, count, alpha, vec3_add(o->point, q[0]), vec3_add(o->point, q[1]),
                   vec3_add(o->point, q[2]), vec3_add(o->point, q[3]), view, blend, 0, 0, red, green, blue, 0.0f,
                   1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
}

static float orb_ring_size(const struct PrimGlowOrb *o, float scale)
{
    return g_sinTable[(int)o->shimmer] * o->size * 0.05f + o->size * scale;
}

void prim_glow_orb_render(struct EffectPrim *self, const float *view)
{
    struct PrimGlowOrb *o = &self->records.glow_orb;
    int count = 0;
    short alpha;
    short blend;

    if (o->active != 1)
        return;
    alpha = (short)o->brightness;
    if (alpha <= 0)
        return;

    if (o->style == ORB_DOUBLE_HALO) {
        orb_ring(self, view, &count, orb_ring_size(o, 3.0f), (short)(alpha / 2), PRIM_BLEND_ADDITIVE, 0, 0, 0xff);
        orb_ring(self, view, &count, orb_ring_size(o, 1.7f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0, 0, 0xff);
    } else if (o->style == ORB_HALO) {
        orb_ring(self, view, &count, orb_ring_size(o, 1.3f), alpha, PRIM_BLEND_ADDITIVE, 0, 0, 0xff);
    }

    blend = self->stage == 1 ? PRIM_BLEND_ADDITIVE : PRIM_BLEND_OPAQUE;
    switch (o->style) {
    case ORB_BROWN:
        orb_ring(self, view, &count, orb_ring_size(o, 1.0f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0x44, 0x2a,
                 0x1e);
        break;
    case ORB_RED:
        orb_ring(self, view, &count, orb_ring_size(o, 1.0f), (short)o->brightness, blend, 0xff, 0x32, 0x32);
        break;
    case ORB_YELLOW:
        orb_ring(self, view, &count, orb_ring_size(o, 1.0f), (short)o->brightness, blend, 0xc8, 0xc8, 0x32);
        break;
    case ORB_BLUE:
        orb_ring(self, view, &count, orb_ring_size(o, 1.0f), (short)o->brightness, blend, 0x32, 0x32, 0xff);
        break;
    case ORB_WHITE:
        orb_ring(self, view, &count, orb_ring_size(o, 1.0f), (short)o->brightness, blend, 0xff, 0xff, 0xff);
        break;
    case ORB_HALO:
    case ORB_DOUBLE_HALO:
        orb_ring(self, view, &count, orb_ring_size(o, 1.0f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0x28, 0x28,
                 0xff);
        break;
    default:
        orb_ring(self, view, &count, orb_ring_size(o, 1.0f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0, 0, 0xff);
        break;
    }

    switch (o->style) {
    case ORB_BROWN:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0xff, 0xff,
                 0xff);
        break;
    case ORB_RED:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0xff, 0x32,
                 0x32);
        break;
    case ORB_YELLOW:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0xc8, 0xc8,
                 0x32);
        break;
    case ORB_BLUE:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0x32, 0x32,
                 0xff);
        break;
    case ORB_WHITE:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0x64, 0x96,
                 0x64);
        break;
    case ORB_HALO:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0x96, 0x96,
                 0xff);
        break;
    case ORB_DOUBLE_HALO:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0xfa, 0xfa,
                 0xff);
        break;
    default:
        orb_ring(self, view, &count, orb_ring_size(o, 0.5f), (short)o->brightness, PRIM_BLEND_ADDITIVE, 0xff, 0xff,
                 0);
        break;
    }
}

void prim_ring_wall_render(struct EffectPrim *self, const float *view)
{
    int count = 0;
    int i;

    for (i = 0; i < RECORD_COUNT; i++) {
        struct PrimRingWall *w = &self->records.ring_walls[i];
        int step, around, column;

        if (w->active != 1)
            continue;
        step = (short)w->span / 9;
        column = 0;
        around = 0;
        if ((short)w->span <= 0)
            continue;
        do {
            int degree = (w->start + around) % 360;
            int lean_dir;
            float foot_x, foot_z, lean_out, lean_up;
            float ox, oz;
            Vec3 p0, p1, p2, p3;

            if (w->span == 360 && column == RING_WALL_STRIPS - 1)
                degree = w->start;
            foot_x = g_cosTable[degree] * w->radius;
            foot_z = w->radius * g_sinTable[degree];
            w->foot.x = foot_x;
            w->foot.y = 0.0f;
            w->foot.z = foot_z;
            lean_out = g_cosTable[w->tilt] * w->height[column];
            lean_up = g_sinTable[w->tilt] * w->height[column];
            if (i == 2)
                lean_dir = 90 - degree;
            else if (i == 1)
                lean_dir = 270 - degree;
            else if (i == 0)
                lean_dir = 450 - degree;
            else
                lean_dir = 630 - degree;
            if (lean_dir >= 360)
                lean_dir -= 360;
            w->top.y = -lean_up;
            w->top.x = g_cosTable[lean_dir] * lean_out + foot_x;
            w->top.z = g_sinTable[lean_dir] * lean_out + foot_z;

            ox = (float)w->offset_x;
            oz = (float)w->offset_z;
            p2.x = ox + w->top.x;
            p2.y = -lean_up;
            p2.z = oz + w->top.z;
            p1.x = ox + foot_x;
            p1.y = 0.0f;
            p1.z = oz + foot_z;
            p3.x = ox + w->last_top.x;
            p3.y = w->last_top.y;
            p3.z = w->last_top.z + oz;
            p0.x = ox + w->last_foot.x;
            p0.y = w->last_foot.y;
            p0.z = w->last_foot.z + oz;
            if (column > 0)
                prim_draw_strip(self, &count, column - 1, (short)w->alpha, p0, p1, p2, p3, view, 0);

            column++;
            if (column >= RING_WALL_STRIPS)
                column = RING_WALL_STRIPS - 1;
            around += step;
            w->last_top = w->top;
            w->last_foot = w->foot;
        } while (around < (short)w->span);
    }
}

void prim_ring_wall_process(struct EffectPrim *self)
{
    int i, j;

    for (i = 0; i < RECORD_COUNT; i++) {
        struct PrimRingWall *w = &self->records.ring_walls[i];

        if (w->active != 1)
            continue;
        w->age++;
        w->radius = w->radius - 0.01f;
        if (w->age >= 10) {
            w->alpha--;
            if ((short)w->alpha <= 0) {
                w->alpha = 0;
                if (w->age > 30)
                    w->active = 0;
                w->radius = 20.0f;
                w->tilt = 90;
            }
        } else {
            w->alpha += 10;
        }
        for (j = 0; j < RING_WALL_STRIPS; j++)
            w->height[j] = g_sinTable[w->age] * w->full_height;
    }
}

#define SPARK_RADIUS 4.0f

unsigned char prim_spark_groups_process(struct EffectPrim *self)
{
    int i, k;

    keep_camera_angles(self);
    for (i = 0; i < RECORD_COUNT; i++) {
        struct PrimSparkGroup *g = &self->records.spark_groups[i];

        if (g->active != 1)
            continue;
        g->frames++;
        for (k = 0; k < 4; k++) {
            float age = g->spark[k].age + 1.0f;

            g->spark[k].age = age;
            if (10.0f > age) {
                g->spark[k].alpha = g->spark[k].alpha + 25.0f;
            } else {
                float alpha = g->spark[k].alpha - 12.0f;

                g->spark[k].alpha = alpha;
                if (12.0f > alpha) {
                    g->spark[k].age = 0.0f;
                    g->spark[k].alpha = 0.0f;
                    g->spark[k].angle = (float)rand_below(360);
                    g->spark[k].texture = (float)rand_below(5);
                    g->point[k_light_slot[k]].y = (float)(-3 - rand_below(8));
                }
            }
        }
        if (self->master->master != NULL)
            g->lift = game_actor_get_mount_lift((struct GameActor *)self->master->master);
    }
    return 1;
}

void prim_spark_groups_render(struct EffectPrim *self, const float *view, float scale,
                                         unsigned char r, unsigned char g, unsigned char b)
{
    int count = 0;
    int i, k;

    for (i = 0; i < RECORD_COUNT; i++) {
        struct PrimSparkGroup *grp = &self->records.spark_groups[i];
        Vec3 q[4];

        if (grp->active != 1)
            continue;
        if (grp->tint == 1) {
            r = 0xff;
            g = 0x14;
            b = 0x14;
        }
        for (k = 0; k < 4; k++) {
            Vec3 *pt = &grp->point[k_light_slot[k]];
            pt->x = g_cosTable[(int)grp->spark[k].angle] * SPARK_RADIUS;
            pt->z = g_sinTable[(int)grp->spark[k].angle] * SPARK_RADIUS;
        }
        facing_quad(self, grp->size * scale, q);

        for (k = 0; k < 4; k++) {
            Vec3 c = grp->point[k_light_slot[k]];
            float texture = grp->spark[k].texture;
            int off_yaw = (int)(grp->spark[k].angle - (float)self->camera_yaw);
            int far_side = !(off_yaw < 70 && off_yaw > -80) && (unsigned)(off_yaw + 270) <= 540;
            int straight = (texture == 0.0f || texture == 2.0f) ? !far_side : far_side;
            Vec3 p0, p1, p2, p3;

            if (straight) {
                p0 = vec3_add(c, q[0]);
                p1 = vec3_add(c, q[1]);
                p2 = vec3_add(c, q[2]);
                p3 = vec3_add(c, q[3]);
            } else {
                p0 = vec3_add(c, q[1]);
                p1 = vec3_add(c, q[0]);
                p2 = vec3_add(c, q[3]);
                p3 = vec3_add(c, q[2]);
            }
            p0.y -= grp->lift;
            p1.y -= grp->lift;
            p2.y -= grp->lift;
            p3.y -= grp->lift;
            prim_draw_quad(self, &count, (short)(int)grp->spark[k].alpha, p0, p1, p2, p3, view,
                           PRIM_BLEND_ADDITIVE, (char)(unsigned char)(int)texture, 0, r, g, b, 0.0f, 1.0f, 0.0f,
                           1.0f, PRIM_QUAD_DEPTH_BIAS);
        }
    }
}
