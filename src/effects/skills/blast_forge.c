#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

#define BLAST_FORGE_LIST 5
#define MARK_LIST 0xd
#define BURST_PERIOD_MS 1000

struct BlastForge {
    struct Effect base;
    struct Ez2StrParticle *loop;
    struct Ez2StrParticle *loop_mark;
    struct Ez2StrParticle *burst;
    struct Ez2StrParticle *burst_mark;
    unsigned char loop_parity;
    __int64 last_burst_ms;
};

#define PARTICLE_LIFE(p) ((p)->base.life_seconds)

void blast_forge_init(struct Effect *self);
void blast_forge_process(struct Effect *self);
void blast_forge_render(struct Effect *self);
void blast_forge_release(struct Effect *self);
static void blast_forge_destroy(struct Effect *effect);

static const struct EffectOps blast_forge_ops = {
    .set_submit = effect_set_submit,
    .init = blast_forge_init,
    .update_range = effect_update_range,
    .process = blast_forge_process,
    .render = blast_forge_render,
    .release = blast_forge_release,
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
    .destroy = blast_forge_destroy,
    .fade_out = effect_fade_out,
};

static void load_str(struct Ez2StrParticle *p, const char *str)
{
    ez2_str_particle_load(p, str);
}

static void render_str(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

struct BlastForge *blast_forge_construct(struct BlastForge *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &blast_forge_ops;
    self->loop_parity = 0;
    self->last_burst_ms = (__int64)timeGetTime();
    self->loop = 0;
    self->loop_mark = 0;
    self->burst = 0;
    self->burst_mark = 0;
    return self;
}

struct Effect *blast_forge_new(void)
{
    struct BlastForge *self = mem_try_alloc(sizeof *self);

    return self ? &blast_forge_construct(self)->base : NULL;
}

void blast_forge_destruct(struct BlastForge *self)
{
    self->base.ops = &blast_forge_ops;
    effect_destruct(&self->base);
}

static void blast_forge_destroy(struct Effect *effect)
{
    struct BlastForge *self = (struct BlastForge *)effect;
    blast_forge_destruct(self);
    mem_free(self);
}

void blast_forge_start_loop(struct BlastForge *self)
{
    self->loop_parity = self->loop_parity == 0;
    self->loop = ez2str_particle_new(self->base.pos, BLAST_FORGE_LIST);
    self->loop_mark = ez2str_particle_new(self->base.pos, MARK_LIST);
    if (self->loop && self->loop_mark) {
        load_str(self->loop, "blast_forge\\blast_forge\\blast_forge_5s.str");
        load_str(self->loop_mark, "blast_forge\\blast_forge\\blast_forge_5s_bottom.str");
        self->loop->loop_forever = 1;
        self->loop_mark->loop_forever = 1;
    }
}

void blast_forge_burst(struct BlastForge *self)
{
    if (self->burst) {
        particle_free(&self->burst->base);
        self->burst = 0;
    }
    if (self->burst_mark) {
        particle_free(&self->burst_mark->base);
        self->burst_mark = 0;
    }
    self->burst = ez2str_particle_new(self->base.pos, BLAST_FORGE_LIST);
    self->burst_mark = ez2str_particle_new(self->base.pos, MARK_LIST);
    if (self->burst && self->burst_mark) {
        load_str(self->burst, "blast_forge\\blast_forge\\blast_forge.str");
        load_str(self->burst_mark, "blast_forge\\blast_forge\\blast_forge_bottom.str");
    }
}

void blast_forge_step_str(struct BlastForge *self, struct Ez2StrParticle *p)
{
    static const Vec3 no_offset = { 0.0f, 0.0f, 0.0f };
    float m[12], inv[12], vx, vy, vz;
    Vec3 pos;

    if (!p)
        return;
    p->base.pos = self->base.pos;
    pos = p->base.pos;
    if (!game_copy_view_matrix(m))
        return;
    mat_invert(inv, m);
    vx = m[0] * pos.x + m[3] * pos.y + m[6] * pos.z + m[9];
    vz = m[2] * pos.x + m[5] * pos.y + m[8] * pos.z + m[11];
    vy = m[1] * pos.x + m[4] * pos.y + m[7] * pos.z + m[10];
    vy = vy - 8.0f;
    p->base.pos.x = vx * inv[0] + vy * inv[3] + vz * inv[6] + inv[9];
    p->base.pos.y = vx * inv[1] + vy * inv[4] + vz * inv[7] + inv[10];
    p->base.pos.z = vx * inv[2] + vy * inv[5] + vz * inv[8] + inv[11];
    ez2_str_particle_process(p, no_offset);
}

void blast_forge_step_burst(struct BlastForge *self)
{
    struct Ez2StrParticle *burst = self->burst;

    if (burst && self->burst_mark) {
        if (PARTICLE_LIFE(burst) == 0.0f || PARTICLE_LIFE(burst) * 60.0f > (float)burst->base.age) {
            blast_forge_step_str(self, burst);
            blast_forge_step_str(self, self->burst_mark);
        } else {
            particle_free(&burst->base);
            particle_free(&self->burst_mark->base);
            self->burst = 0;
            self->burst_mark = 0;
        }
    }
    if ((__int64)timeGetTime() - self->last_burst_ms >= BURST_PERIOD_MS) {
        self->last_burst_ms = (__int64)timeGetTime();
        blast_forge_burst(self);
    }
}

void blast_forge_init(struct Effect *effect)
{
    struct BlastForge *self = (struct BlastForge *)effect;
    self->base.has_instance_key = 1;
    blast_forge_start_loop(self);
    blast_forge_burst(self);
}

void blast_forge_process(struct Effect *effect)
{
    struct BlastForge *self = (struct BlastForge *)effect;
    blast_forge_step_str(self, self->loop);
    blast_forge_step_str(self, self->loop_mark);
    blast_forge_step_burst(self);
}

void blast_forge_render(struct Effect *effect)
{
    struct BlastForge *self = (struct BlastForge *)effect;
    if (!self->burst) {
        if (self->loop)
            render_str(self->loop);
        if (self->loop_mark)
            render_str(self->loop_mark);
    }
    if (self->burst)
        render_str(self->burst);
    if (self->burst_mark)
        render_str(self->burst_mark);
}

void blast_forge_release(struct Effect *effect)
{
    struct BlastForge *self = (struct BlastForge *)effect;
    particle_free(&self->loop->base);
    particle_free(&self->loop_mark->base);
    if (self->burst)
        particle_free(&self->burst->base);
    if (self->burst_mark)
        particle_free(&self->burst_mark->base);
}
