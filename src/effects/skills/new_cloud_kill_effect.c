#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/list.h"

struct Effect;
struct NewCloudKillEffect;
struct String;
void new_cloud_kill_effect_init(struct Effect *self);
void new_cloud_kill_effect_process(struct Effect *self);
void new_cloud_kill_effect_render(struct Effect *self);
void new_cloud_kill_effect_release(struct Effect *self);
static void new_cloud_kill_effect_destroy(struct Effect *effect);

static const struct EffectOps new_cloud_kill_effect_ops = {
    .set_submit = effect_set_submit,
    .init = new_cloud_kill_effect_init,
    .update_range = effect_update_range,
    .process = new_cloud_kill_effect_process,
    .render = new_cloud_kill_effect_render,
    .release = new_cloud_kill_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = new_cloud_kill_effect_destroy,
    .fade_out = effect_fade_out,
};

struct NewCloudKillParticle {
    int alpha_max;
    int life;
    signed char state;
    signed char cell;
    float rise;
};
PARTICLE_EFFECT_DATA_FITS(NewCloudKillParticle);
static __inline struct NewCloudKillParticle *new_cloud_kill_particle(const void *p)
{
    return (struct NewCloudKillParticle *)((const struct Particle *)p)->effect_data;
}
#define PARTICLE_OFFSET(p) ((p)->offset)

static void tag_particle(struct Particle *p, int effect_type, int index)
{
    p->effect_type = effect_type;
    p->index = index;
    p->group = 0;
    p->owner_tag = 1;
}

#define EFFECT_NEW_CLOUD_KILL 0x4ac
#define PARTICLE_LIST 5
#define RESHOW_MS 5000

struct NewCloudKillEffect {
    struct Effect base;
    struct List circle;
    struct List mist;
    struct List clouds;
    struct List lightning;
    struct List puffs;
    unsigned circle_shown_ms;
    unsigned mist_shown_ms;
};

static float deg_to_rad_computed(float deg)
{
    return deg / 180.0f * 3.141592f;
}

static int rand_mod(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static struct Particle *particle_of(const struct ListNode *node)
{
    return *LIST_VALUE(node, struct Particle *);
}

static void free_particles(struct List *l)
{
    struct ListNode *node;

    for (node = l->head->next; node != l->head; node = node->next)
        particle_free(particle_of(node));
    list_clear(l, NULL);
}

static void set_matrix(struct Particle *p, const float m[12])
{
    memcpy(p->matrix, m, 12 * sizeof(float));
}

static void spun_matrix(struct Particle *p, float tilt)
{
    float spin[12], scale[12], m[12];

    mat_set_rotate_z(spin, deg_to_rad_computed(p->roll));
    mat_rotate_x(spin, tilt);
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(m, scale, spin);
    set_matrix(p, m);
}

static void facing_matrix(struct Particle *p)
{
    float scale[12], pitch[12], m[12];

    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_set_rotate_x(pitch, deg_to_rad_computed(game_camera_pitch()));
    mat_multiply(m, scale, pitch);
    set_matrix(p, m);
}

static void spin_one_degree(struct Particle *p)
{
    p->roll = (float)((int)(p->roll + 1.0f) % 360);
}

static void rise_and_place(struct NewCloudKillEffect *self, struct Particle *p)
{
    const float rise = new_cloud_kill_particle(p)->rise * 1.0f;
    Vec3 at;

    PARTICLE_OFFSET(p).x = PARTICLE_OFFSET(p).x + 0.0f;
    PARTICLE_OFFSET(p).y = PARTICLE_OFFSET(p).y - rise;
    PARTICLE_OFFSET(p).z = PARTICLE_OFFSET(p).z + 0.0f;
    at = self->base.pos;
    p->pos.x = at.x + PARTICLE_OFFSET(p).x;
    p->pos.y = at.y + PARTICLE_OFFSET(p).y;
    p->pos.z = at.z + PARTICLE_OFFSET(p).z;
}

struct NewCloudKillEffect *new_cloud_kill_effect_construct(struct NewCloudKillEffect *self)
{
    struct List *lists[5];
    int i;

    effect_construct(&self->base, EFFECT_NEW_CLOUD_KILL);
    self->base.ops = &new_cloud_kill_effect_ops;
    lists[0] = &self->circle;
    lists[1] = &self->mist;
    lists[2] = &self->clouds;
    lists[3] = &self->lightning;
    lists[4] = &self->puffs;
    for (i = 0; i < 5; i++)
        list_init(lists[i]);
    self->circle_shown_ms = 0;
    self->mist_shown_ms = 0;
    return self;
}

struct Effect *new_cloud_kill_effect_new(void)
{
    struct NewCloudKillEffect *self = mem_try_alloc(sizeof *self);

    return self ? &new_cloud_kill_effect_construct(self)->base : NULL;
}

/* Effect types created as NewCloudKillEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow new_cloud_kill_effect_type_rows[] = {
    EFFECT_TYPE(0x4ac, new_cloud_kill_effect_new),
};
const struct EffectTypeTable new_cloud_kill_effect_types = EFFECT_TYPE_TABLE(new_cloud_kill_effect_type_rows);

void new_cloud_kill_effect_destruct(struct NewCloudKillEffect *self)
{
    self->base.ops = &new_cloud_kill_effect_ops;
    list_free(&self->puffs, NULL);
    list_free(&self->lightning, NULL);
    list_free(&self->clouds, NULL);
    list_free(&self->mist, NULL);
    list_free(&self->circle, NULL);
    effect_destruct(&self->base);
}

static void new_cloud_kill_effect_destroy(struct Effect *effect)
{
    struct NewCloudKillEffect *self = (struct NewCloudKillEffect *)effect;
    new_cloud_kill_effect_destruct(self);
    mem_free(self);
}

void new_cloud_kill_effect_reset_circle(struct Particle *p)
{
    p->alpha = 0xff;
    p->roll = 0;
    p->age = 60;
    new_cloud_kill_particle(p)->state = 0;
    particle_quad_unit_corners(p->corners);
}

void new_cloud_kill_effect_respawn_puff(struct Particle *p)
{
    int radius, angle;

    p->alpha = 0;
    new_cloud_kill_particle(p)->alpha_max = rand_mod(50) + 150;
    new_cloud_kill_particle(p)->state = 0;
    p->age = 0;
    particle_quad_unit_corners(p->corners);
    radius = rand_mod(15) + 2;
    angle = rand_mod(360);
    PARTICLE_OFFSET(p).y = 0;
    PARTICLE_OFFSET(p).x = g_cosTable[angle] * (float)radius;
    PARTICLE_OFFSET(p).z = g_sinTable[angle] * (float)radius;
    p->scale = (float)rand_mod(10) * 0.1f + 0.5f;
    new_cloud_kill_particle(p)->rise = (float)rand_mod(30) * 0.01f + 0.05f;
}

void new_cloud_kill_effect_respawn_mist(struct Particle *p)
{
    int angle;

    new_cloud_kill_particle(p)->cell = (signed char)rand_mod(2);
    angle = rand_mod(360);
    PARTICLE_OFFSET(p).y = 0;
    PARTICLE_OFFSET(p).x = g_cosTable[angle] * 18.0f;
    PARTICLE_OFFSET(p).z = g_sinTable[angle] * 18.0f;
    p->alpha = 0xff;
    p->scale = (float)rand_mod(31) * 0.1f + 2.0f;
    new_cloud_kill_particle(p)->state = 0;
    new_cloud_kill_particle(p)->rise = (float)rand_mod(31) * 0.01f + 0.05f;
    particle_quad_unit_corners(p->corners);
}

void new_cloud_kill_effect_respawn_lightning(struct Particle *p)
{
    int dz, dx;

    p->scale = (float)rand_mod(5) + 10.0f;
    new_cloud_kill_particle(p)->state = 10;
    new_cloud_kill_particle(p)->cell = (signed char)rand_mod(4);
    new_cloud_kill_particle(p)->life = rand_mod(50) + 10;
    p->roll = (float)rand_mod(360);
    dz = rand_mod(10);
    dx = rand_mod(10);
    PARTICLE_OFFSET(p).y = -10.0f;
    PARTICLE_OFFSET(p).x = (float)dx - 5.0f;
    PARTICLE_OFFSET(p).z = (float)dz - 5.0f;
    p->age = 0;
    particle_quad_unit_corners(p->corners);
}

void new_cloud_kill_effect_respawn_cloud(struct Particle *p)
{
    int radius, angle;

    p->alpha = 0;
    new_cloud_kill_particle(p)->state = 0;
    p->age = 0;
    particle_quad_unit_corners(p->corners);
    p->scale = (float)rand_mod(51) * 0.1f + 7.0f;
    radius = rand_mod(13) + 2;
    angle = rand_mod(360);
    PARTICLE_OFFSET(p).y = p->scale * -0.5f;
    PARTICLE_OFFSET(p).x = g_cosTable[angle] * (float)radius;
    PARTICLE_OFFSET(p).z = g_sinTable[angle] * (float)radius;
    new_cloud_kill_particle(p)->cell = (signed char)rand_mod(4);
    new_cloud_kill_particle(p)->alpha_max = rand_mod(151) + 50;
    new_cloud_kill_particle(p)->rise = (float)rand_mod(10) * 0.01f + 0.05f;
}

static struct Particle *new_particle(struct NewCloudKillEffect *self, const char *texture, size_t len, int index,
                                      int tag_before_texture)
{
    struct Particle *p = particle_new(self->base.pos, PARTICLE_LIST);
    struct String name;

    if (!p)
        return 0;
    if (tag_before_texture)
        tag_particle(p, self->base.type, index);
    string_init(&name);
    string_assign(&name, texture, len);
    particle_set_texture(p, name);
    if (!tag_before_texture)
        tag_particle(p, self->base.type, index);
    return p;
}
#define TEX_CIRCLE "effect\\new_cloud_kill\\ef_cloud_circle.tga", 0x29
#define TEX_CLOUD "effect\\new_cloud_kill\\ef_cloud_kill.tga", 0x27
#define TEX_LIGHTNING "effect\\new_cloud_kill\\ef_comm_lightning_cloud.tga", 0x31

void new_cloud_kill_effect_add_circle(struct NewCloudKillEffect *self)
{
    struct Particle *p = new_particle(self, TEX_CIRCLE, 1, 0);

    if (!p)
        return;
    p->scale = 20.0f;
    new_cloud_kill_effect_reset_circle(p);
    list_push_back(&self->circle, &p, sizeof p);
}

void new_cloud_kill_effect_add_mist(struct NewCloudKillEffect *self)
{
    int i;

    for (i = 0; i < 100; i++) {
        struct Particle *p = new_particle(self, TEX_CLOUD, i, 1);
        if (!p)
            continue;
        new_cloud_kill_effect_respawn_mist(p);
        list_push_back(&self->mist, &p, sizeof p);
    }
}

void new_cloud_kill_effect_add_clouds(struct NewCloudKillEffect *self)
{
    int i;

    for (i = 0; i < 30; i++) {
        struct Particle *p = new_particle(self, TEX_CLOUD, i, 1);
        if (!p)
            continue;
        new_cloud_kill_effect_respawn_cloud(p);
        p->age = -rand_mod(71);
        list_push_back(&self->clouds, &p, sizeof p);
    }
}

void new_cloud_kill_effect_add_lightning(struct NewCloudKillEffect *self)
{
    struct Particle *p = new_particle(self, TEX_LIGHTNING, 1, 1);

    if (!p)
        return;
    p->alpha = 0xff;
    p->depth_bias = 0.035f;
    new_cloud_kill_effect_respawn_lightning(p);
    new_cloud_kill_particle(p)->life = 50;
    list_push_back(&self->lightning, &p, sizeof p);
}

void new_cloud_kill_effect_add_puffs(struct NewCloudKillEffect *self)
{
    int i;

    for (i = 0; i < 30; i++) {
        struct Particle *p = new_particle(self, TEX_CLOUD, i, 1);
        if (!p)
            continue;
        new_cloud_kill_effect_respawn_puff(p);
        p->age = -rand_mod(80);
        list_push_back(&self->puffs, &p, sizeof p);
    }
}

void new_cloud_kill_effect_init(struct Effect *effect)
{
    struct NewCloudKillEffect *self = (struct NewCloudKillEffect *)effect;
    self->circle_shown_ms = timeGetTime();
    self->mist_shown_ms = timeGetTime();
    new_cloud_kill_effect_add_circle(self);
    new_cloud_kill_effect_add_mist(self);
    new_cloud_kill_effect_add_clouds(self);
    new_cloud_kill_effect_add_lightning(self);
    new_cloud_kill_effect_add_puffs(self);
}

void new_cloud_kill_effect_release(struct Effect *effect)
{
    struct NewCloudKillEffect *self = (struct NewCloudKillEffect *)effect;
    free_particles(&self->circle);
    free_particles(&self->mist);
    free_particles(&self->clouds);
    free_particles(&self->lightning);
    free_particles(&self->puffs);
}

void new_cloud_kill_effect_step_circle(struct NewCloudKillEffect *self,
                                                struct Particle *p)
{
    if (!p)
        return;
    if (p->age == 0) {
        new_cloud_kill_particle(p)->state = 1;
        return;
    }
    spin_one_degree(p);
    p->alpha = (int)((float)p->age * 4.25f);
    spun_matrix(p, 3.141592f * 0.5f);
    p->pos = self->base.pos;
}

void new_cloud_kill_effect_step_mist(struct NewCloudKillEffect *self,
                                              struct Particle *p)
{
    int alpha;

    if (!p)
        return;
    alpha = p->alpha - 3;
    alpha = alpha > 0 ? alpha : 0;
    p->alpha = alpha;
    if (!alpha) {
        new_cloud_kill_particle(p)->state = 1;
        return;
    }
    p->scale = p->scale - p->scale / (float)alpha;
    rise_and_place(self, p);
    spin_one_degree(p);
    spun_matrix(p, deg_to_rad_computed(game_camera_pitch()));
}

void new_cloud_kill_effect_step_cloud(struct NewCloudKillEffect *self,
                                               struct Particle *p)
{
    int alpha;

    if (!p)
        return;
    if (new_cloud_kill_particle(p)->state == 0) {
        alpha = p->alpha + 2;
        if (alpha >= new_cloud_kill_particle(p)->alpha_max)
            alpha = new_cloud_kill_particle(p)->alpha_max;
        p->alpha = alpha;
        if (new_cloud_kill_particle(p)->alpha_max == alpha)
            new_cloud_kill_particle(p)->state = 1;
    } else if (new_cloud_kill_particle(p)->state == 1) {
        alpha = p->alpha - 1;
        if (alpha <= 0)
            alpha = 0;
        p->alpha = alpha;
        if (!alpha) {
            new_cloud_kill_effect_respawn_cloud(p);
            return;
        }
        p->scale = p->scale - p->scale / (float)alpha;
    }
    rise_and_place(self, p);
    facing_matrix(p);
}

void new_cloud_kill_effect_step_puff(struct NewCloudKillEffect *self,
                                              struct Particle *p)
{
    int alpha;

    if (!p)
        return;
    if (new_cloud_kill_particle(p)->state == 0) {
        alpha = p->alpha + 5;
        if (alpha >= new_cloud_kill_particle(p)->alpha_max)
            alpha = new_cloud_kill_particle(p)->alpha_max;
        p->alpha = alpha;
        if (new_cloud_kill_particle(p)->alpha_max == alpha)
            new_cloud_kill_particle(p)->state = 1;
    } else if (new_cloud_kill_particle(p)->state == 1) {
        alpha = p->alpha - 2;
        if (alpha <= 0)
            alpha = 0;
        p->alpha = alpha;
        if (!alpha) {
            new_cloud_kill_effect_respawn_puff(p);
            return;
        }
        p->scale = p->scale / (float)new_cloud_kill_particle(p)->alpha_max * (float)alpha;
    }
    rise_and_place(self, p);
    facing_matrix(p);
}

void new_cloud_kill_effect_step_lightning(struct NewCloudKillEffect *self,
                                                   struct Particle *p)
{
    float yaw[12];
    int flicker;
    Vec3 o, at;

    if (p->age >= new_cloud_kill_particle(p)->life) {
        new_cloud_kill_effect_respawn_lightning(p);
        return;
    }
    flicker = new_cloud_kill_particle(p)->state - 1;
    if (flicker <= 0)
        flicker = 0;
    p->alpha = flicker * 25;
    new_cloud_kill_particle(p)->state = (signed char)flicker;
    spun_matrix(p, deg_to_rad_computed(game_camera_pitch()));

    mat_set_rotate_y(yaw, deg_to_rad_computed(game_camera_heading()));
    o = PARTICLE_OFFSET(p);
    at = self->base.pos;
    p->pos.x = at.x + (yaw[0] * o.x + yaw[3] * o.y + yaw[6] * o.z + yaw[9]);
    p->pos.y = at.y + (yaw[1] * o.x + yaw[4] * o.y + yaw[7] * o.z + yaw[10]);
    p->pos.z = at.z + (yaw[2] * o.x + yaw[5] * o.y + yaw[8] * o.z + yaw[11]);
}

void new_cloud_kill_effect_process(struct Effect *effect)
{
    struct NewCloudKillEffect *self = (struct NewCloudKillEffect *)effect;
    const unsigned now = timeGetTime();
    struct ListNode *n;

    for (n = self->circle.head->next; n != self->circle.head; n = n->next) {
        if (now - self->circle_shown_ms > RESHOW_MS)
            new_cloud_kill_effect_reset_circle(particle_of(n));
        new_cloud_kill_effect_step_circle(self, particle_of(n));
        particle_of(n)->age--;
    }
    if (now - self->circle_shown_ms > RESHOW_MS)
        self->circle_shown_ms = timeGetTime();

    for (n = self->mist.head->next; n != self->mist.head; n = n->next) {
        if (now - self->mist_shown_ms > RESHOW_MS)
            new_cloud_kill_effect_respawn_mist(particle_of(n));
        new_cloud_kill_effect_step_mist(self, particle_of(n));
    }
    if (now - self->mist_shown_ms > RESHOW_MS)
        self->mist_shown_ms = timeGetTime();

    for (n = self->clouds.head->next; n != self->clouds.head; n = n->next) {
        particle_of(n)->age++;
        if (particle_of(n)->age > 0 && particle_of(n))
            new_cloud_kill_effect_step_cloud(self, particle_of(n));
    }

    for (n = self->lightning.head->next; n != self->lightning.head; n = n->next) {
        new_cloud_kill_effect_step_lightning(self, particle_of(n));
        particle_of(n)->age++;
    }

    for (n = self->puffs.head->next; n != self->puffs.head; n = n->next) {
        particle_of(n)->age++;
        if (particle_of(n)->age > 0 && particle_of(n))
            new_cloud_kill_effect_step_puff(self, particle_of(n));
    }
}

static void draw_cell(struct Particle *p, float v_top, float v_bottom, float u_left, float u_right)
{
    struct ParticleRenderArgs args;

    args.facing_fixed = 0;
    args.v_top = v_top;
    args.v_bottom = v_bottom;
    args.u_left = u_left;
    args.u_right = u_right;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void new_cloud_kill_effect_draw_circle(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void new_cloud_kill_effect_draw_mist(struct Particle *p)
{
    if (p)
        draw_cell(p, 0.5f, 0.75f, (float)(new_cloud_kill_particle(p)->cell + 1) * 0.25f, (float)(new_cloud_kill_particle(p)->cell + 2) * 0.25f);
}

void new_cloud_kill_effect_draw_cloud(struct Particle *p)
{
    if (p)
        draw_cell(p, 0.0f, 0.25f, (float)new_cloud_kill_particle(p)->cell * 0.25f, (float)(new_cloud_kill_particle(p)->cell + 1) * 0.25f);
}

void new_cloud_kill_effect_draw_lightning(struct Particle *p)
{
    int cell, column, row;

    if (!p)
        return;
    cell = new_cloud_kill_particle(p)->cell;
    column = cell % 2;
    row = cell / 2;
    draw_cell(p, (float)row * 0.5f, (float)(row + 1) * 0.5f, (float)column * 0.5f, (float)(column + 1) * 0.5f);
}

void new_cloud_kill_effect_draw_puff(struct Particle *p)
{
    if (p)
        draw_cell(p, 0.5f, 0.75f, 0.0f, 0.25f);
}

void new_cloud_kill_effect_render(struct Effect *effect)
{
    struct NewCloudKillEffect *self = (struct NewCloudKillEffect *)effect;
    struct ListNode *n;

    for (n = self->circle.head->next; n != self->circle.head; n = n->next)
        if (new_cloud_kill_particle(particle_of(n))->state == 0 && particle_of(n))
            new_cloud_kill_effect_draw_circle(particle_of(n));
    for (n = self->mist.head->next; n != self->mist.head; n = n->next)
        if (new_cloud_kill_particle(particle_of(n))->state == 0 && particle_of(n))
            new_cloud_kill_effect_draw_mist(particle_of(n));
    for (n = self->clouds.head->next; n != self->clouds.head; n = n->next)
        if (particle_of(n)->age > 0 && particle_of(n))
            new_cloud_kill_effect_draw_cloud(particle_of(n));
    for (n = self->lightning.head->next; n != self->lightning.head; n = n->next)
        if (new_cloud_kill_particle(particle_of(n))->state > 0 && particle_of(n))
            new_cloud_kill_effect_draw_lightning(particle_of(n));
    for (n = self->puffs.head->next; n != self->puffs.head; n = n->next)
        if (particle_of(n)->age > 0 && particle_of(n))
            new_cloud_kill_effect_draw_puff(particle_of(n));
}
