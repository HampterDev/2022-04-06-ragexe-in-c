#include "contents/sound.h"
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define EFFECT_DIR "이팩트\\"

static struct EffectPrim *launch(struct RagEffect *self, int kind)
{
    struct EffectPrim *p;

    p = self->last_prim = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;
    p->list_flags = 5;
    p->duration = self->duration;
    return p;
}

void rag_elec_rays_101(struct RagEffect *self, signed char style, int angle_base)
{
    struct EffectPrim *p;
    int i, k;

    self->base.pos = self->master->pos;
    if ((unsigned)(self->frame - 11) > 3)
        return;
    p = self->last_prim = rag_launch_prim(self, 101, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE("effect\\elec1.tga");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimArcTube *tube = &self->last_prim->records.arc_tubes[i];

        tube->active = 1;
        tube->frame = 0;
        tube->radius = 0.1f;
        tube->alpha = 0;
        tube->yaw = (short)(i * 45 + angle_base);
        tube->reach = 0.0f;
        tube->arc[0] = 0.0f;
        tube->height = (float)rag_rand(11) * 0.01f + 0.0f;
        tube->centre = self->base.pos;
        tube->points = 20.0f;
        for (k = 20; k >= 1; k--)
            tube->arc[k] = 0.0f;
        tube->style = style;
        tube->style_jagged = 0;
    }
}

void rag_bubble_rings_158(struct RagEffect *self, int alpha_base,
                                     signed char style)
{
    static const float radius[PRIM_RECORD_COUNT] = { 30.0f, 29.0f, 28.0f, 27.0f };
    static const short angle[PRIM_RECORD_COUNT] = { 180, 270, 0, 90 };
    struct EffectPrim *p;
    struct PrimWaveRing *s;
    char name[64];
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    if (self->duration < 100)
        self->duration = 100;
    p = launch(self, 158);
    if (!p)
        return;
    p->motion_count = 27;
    RAG_NEW_TEXTURES(p);
    for (i = 0; i < 27; i++) {
        sprintf(name, "effect\\W_bubble%02d.tga", i + 1);
        self->last_prim->textures[i] = RAG_TEXTURE(name);
    }
    p = self->last_prim;
    p->scale = style == 1 ? 4.0f : 0.0f;
    s = p->records.wave_rings;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        s[i].active = 1;
        s[i].frame = 0;
        s[i].arc = 360;
        s[i].base_height = radius[i];
        s[i].arc_start = angle[i];
        s[i].alpha = (short)(alpha_base + 45 * (3 - i));
        s[i].radius = 3.5f;
        s[i].tilt = 80;
        s[i].style = style;
    }
}

void rag_lines_to_offset_56(struct RagEffect *self, short sel)
{
    struct EffectPrim *p;
    int which, i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = launch(self, 56);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(sel < 10 ? "effect\\line3.tga" : "effect\\line3y.tga");
    which = sel % 10;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSlideStripe *st = &p->records.slide_stripes[i];

        st->active = 1;
        st->age = 0;
        st->back = 100.0f;
        if (which == 0)
            st->side_shift = -15.0f - (float)rag_rand(11);
        else if (which == 1)
            st->side_shift = (float)rag_rand(11) + 15.0f;
        st->start = (float)rag_rand(200);
        if (sel < 10) {
            st->half_width = (float)rag_rand(4) * 0.1f + 0.3f;
            st->length = (float)rag_rand(31) + 20.0f;
            st->color = (unsigned char)(rag_rand(3) + 30);
            st->speed = (float)rag_rand(21) * 0.1f + 1.5f;
        }
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &st->heading);
        st->alpha = 0;
    }
}

static struct EffectPrim *shockwave(struct RagEffect *self, int kind, const char *tex, int keep_count)
{
    struct EffectPrim *p;

    p = launch(self, kind);
    if (!p)
        return NULL;
    if (!keep_count)
        p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    return self->last_prim;
}

#define WAVE_YAW(self, i, step) ((short)((short)((self)->frame * 4 + (i)) * (step)))

void rag_shockwave_163(struct RagEffect *self, const char *tex)
{
    int i;

    self->base.pos = self->master->pos;
    if (self->frame > 2)
        return;
    if (!shockwave(self, 163, tex, 0))
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimDashLine *line = &self->last_prim->records.dash_lines[i];

        line->active = 1;
        line->frame = 0;
        line->reach = 50.0f;
        line->alpha = 255;
        line->yaw = WAVE_YAW(self, i, 30);
        line->pitch = 0;
        line->y = -5.0f;
        line->texture = 0.0f;
    }
}

void rag_shockwave_164(struct RagEffect *self, const char *tex)
{
    int i;

    self->base.pos = self->master->pos;
    if (self->frame > 2)
        return;
    if (!shockwave(self, 164, tex, 0))
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimDashLine *line = &self->last_prim->records.dash_lines[i];

        line->active = 1;
        line->frame = 0;
        line->reach = 100.0f;
        line->length = 15.0f;
        line->alpha = 255;
        line->yaw = WAVE_YAW(self, i, 30);
        line->pitch = 0;
        line->y = -5.0f;
        line->texture = 0.0f;
    }
}

void rag_shockwave_165(struct RagEffect *self, const char *tex)
{
    int i;

    self->base.pos = self->master->pos;
    if (self->frame > 2)
        return;
    if (!shockwave(self, 165, tex, 0))
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimDashLine *line = &self->last_prim->records.dash_lines[i];

        line->active = 1;
        line->frame = 0;
        line->reach = 0.0f;
        line->trail = 0.0f;
        line->length = 15.0f;
        line->alpha = 0;
        line->yaw = WAVE_YAW(self, i, 30);
        line->pitch = 0;
        line->y = -5.0f;
        line->texture = 0.0f;
    }
}

void rag_shockwave_162(struct RagEffect *self, const char *tex)
{
    int i;

    self->base.pos = self->master->pos;
    if (self->frame > 2)
        return;
    if (!shockwave(self, 162, tex, 1))
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimDashLine *line = &self->last_prim->records.dash_lines[i];

        line->active = 1;
        line->frame = 0;
        line->reach = 100.0f;
        line->alpha = 255;
        line->yaw = WAVE_YAW(self, i, 30);
        line->pitch = 0;
        line->y = -5.0f;
    }
}

void rag_epitree_aura_160(struct RagEffect *self, const char *tex, signed char style)
{
    struct EffectPrim *p;
    struct PrimFanQuad *q;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = launch(self, 160);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p = self->last_prim;
    p->scale = 0.0f;
    q = &self->last_prim->records.fan_quads[0];
    q->active = 1;
    q->frame = 0;
    if (style == 30) {
        q->size = 36.0f;
        q->base_y = self->base.pos.y;
        q->fan_style = 19;
    } else if (style == 31) {
        q->size = 36.0f;
        q->base_y = self->base.pos.y;
        q->fan_style = 21;
    } else {
        q->size = 5.0f;
        q->base_y = self->base.pos.y - 10.0f;
        q->fan_style = 2;
    }
    q->shape = 0.0f;
    q->angle = 135;
    q->spin = 0.0f;
    q->alpha = 0;
    q->pos.x = self->base.pos.x;
    q->pos.z = self->base.pos.z;
    q->bob = (short)rag_rand(360);
    q->texture = 0;
    q->quick = style;
}

void rag_spr_ring_167(struct RagEffect *self, signed char style)
{
    struct EffectPrim *p;
    const char *spr, *act;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame > 1)
        return;
    p = self->last_prim = rag_launch_prim(self, 167, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = self->duration;
    self->last_prim->delta2.y = 0.0f;
    self->last_prim->list_flags = 5;
    self->last_prim->pattern |= 0x200;
    self->last_prim->motion_period = 2;
    self->last_prim->loop_motion = 0;
    p = self->last_prim;
    if (style == 20) {
        act = EFFECT_DIR "m_ef05.act";
        spr = EFFECT_DIR "m_ef05.spr";
    } else {
        act = EFFECT_DIR "m_ef03.act";
        spr = EFFECT_DIR "m_ef03.spr";
    }
    p->motion_period = 4;
    render_object_load_spr_res(&self->last_prim->base, spr);
    render_object_load_act_res(&self->last_prim->base, act);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimFlyingSprite *fs = &self->last_prim->records.flying_sprites[i];

        fs->active = 1;
        fs->heading = (short)((short)(self->frame * 4 + i) * 45);
        fs->speed = 0.3f;
        fs->offset.x = 0.0f;
        fs->offset.y = -5.0f;
        fs->offset.z = 0.0f;
        fs->offset.x = g_cosTable[fs->heading] * 5.0f + fs->offset.x;
        fs->offset.z = g_sinTable[fs->heading] * 5.0f + fs->offset.z;
        if (style == 9)
            fs->tint = 0;
        else if (style == 10)
            fs->tint = 1;
        else if (style == 20)
            fs->tint = 4;
    }
    prim_sync_particles(self->last_prim);
}

static void fireball(struct RagEffect *self, int turned, float turn, int rg, int last_frame)
{
    struct EffectPrim *p;
    float dist;
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = 20;
    p->pattern |= 0x120;
    p->delta2.x = 0.0f;
    p->delta2.y = -15.0f;
    p->delta2.z = 0.0f;
    render_object_turn_toward(&p->base, self->offset.x, self->offset.z);
    if (turned) {
        p->base.roty += turn;
        if (0.0f > p->base.roty)
            p->base.roty += 360.0f;
        if (p->base.roty >= 360.0f)
            p->base.roty -= 360.0f;
    }
    p->angle[0] = -p->base.roty;
    p->base.roty = 180.0f - p->base.roty;
    if (0.0f > p->base.roty)
        p->base.roty += 360.0f;
    dist = render_object_calc_ground_dist(&p->base, self->offset.x, self->offset.z);
    p->speed = -(dist / (float)p->duration);
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->scale = 1.0f;
    p->motion_period = 1;
    p->r = rg;
    p->g = rg;
    p->b = 255;
    p->alpha = (float)((last_frame - self->frame) * 40);
    render_object_load_spr_res(&p->base, EFFECT_DIR "fireball.spr");
    render_object_load_act_res(&p->base, EFFECT_DIR "fireball.act");
    prim_sync_particles(p);
}

static void fireball_volleys(struct RagEffect *self, const int *rg)
{
    if (self->frame == 5)
        sound_play_wave("effect\\EF_FireBall.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
    if (self->frame == 0 || self->frame == 1 || self->frame == 2 || self->frame == 3)
        fireball(self, 0, 0.0f, rg[0], 5);
    if (self->frame == 6 || self->frame == 7 || self->frame == 8 || self->frame == 9)
        fireball(self, 1, -5.0f, rg[1], 11);
    if (self->frame == 12 || self->frame == 13 || self->frame == 14 || self->frame == 15)
        fireball(self, 1, 5.0f, rg[2], 17);
}

void rag_blue_fireballs(struct RagEffect *self)
{
    static const int rg[3] = { 55, 30, 5 };

    fireball_volleys(self, rg);
}

void rag_pale_fireballs(struct RagEffect *self)
{
    static const int rg[3] = { 155, 130, 105 };

    fireball_volleys(self, rg);
}

void rag_three_rings_60(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimRing *s;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = launch(self, 60);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    self->last_prim->scale = 16.0f;
    s = self->last_prim->records.rings;
    prim_sub_ring(&s[0], 0, 74.0f, (short)rag_rand(360), 1.3f, 90, 0xc08, 0);
    prim_sub_ring(&s[1], 0, 77.0f, (short)rag_rand(360), 1.0f, 90, 0xc08, 0);
    prim_sub_ring(&s[2], 0, 80.0f, (short)rag_rand(360), 0.7f, 90, 0xc08, 0);
    s[3].active = 0;
}

void rag_begin_spell_rings(struct RagEffect *self)
{
    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    rag_play_wave(self, "effect\\EF_BeginSpell.wav", -100.0f);
    rag_casting_circle(self, 45, "effect\\ring_white.tga", 15);
    rag_casting_circle(self, 25, "effect\\ring_white.tga", 15);
}

void rag_sphere_motes_159(struct RagEffect *self, const char *tex,
                                     signed char style)
{
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = launch(self, 159);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWhirlDisc *disc = &self->last_prim->records.whirl_discs[i];
        float ring;

        disc->active = 1;
        disc->frame = -rand() % 101;
        disc->size = 4.0f;
        disc->alpha = 0;
        disc->yaw = (short)rag_rand(360);
        disc->pitch = (short)rag_rand(360);
        disc->lift = -5.0f;
        disc->speed = 0.5f;
        disc->phase = 0;
        disc->spins = 0;
        disc->tint = style;
        ring = g_cosTable[disc->yaw] * 70.0f;
        disc->pos.z = g_sinTable[disc->yaw] * 70.0f;
        disc->pos.x = g_cosTable[disc->pitch] * ring;
        disc->pos.y = g_sinTable[disc->pitch] * ring + disc->lift;
    }
}

static void wave_166(struct RagEffect *self, const char *tex, float half_width, short alpha, float scroll_speed,
                     float tone)
{
    int i;

    if (!shockwave(self, 166, tex, 0))
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRibbon *rb = &self->last_prim->records.ribbons[i];

        rb->active = 1;
        rb->frame = 0;
        rb->half_width = half_width;
        rb->alpha = alpha;
        rb->angle = WAVE_YAW(self, i, 45);
        rb->y = -5.0f;
        rb->texture = 0.0f;
        rb->scroll = 0.0f;
        rb->scroll_speed = scroll_speed;
        rb->tone = tone;
    }
}

void rag_double_shockwave_166(struct RagEffect *self, const char *tex)
{
    self->base.pos = self->master->pos;
    if (self->frame > 1)
        return;
    wave_166(self, tex, 6.0f, 205, 0.01f, 0.0f);
    wave_166(self, tex, 10.5f, 255, 0.015f, 1.0f);
}

void rag_cloud_dash_69(struct RagEffect *self)
{
    struct EffectPrim *p;
    float dx, dz;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame <= 1) {
        p = launch(self, 69);
        if (!p)
            return;
        RAG_NEW_TEXTURES(p);
        p->textures[0] = RAG_TEXTURE("effect\\cloud11.tga");
        p = self->last_prim;
        p->scale = 0.0f;
        p->delta = self->offset;
        dx = self->offset.x - self->base.pos.x;
        dz = self->offset.z - self->base.pos.z;
        for (i = 0; i < PRIM_RECORD_COUNT; i++) {
            struct PrimPipe *pipe = &p->records.pipes[i];
            float back;

            pipe->active = 1;
            pipe->age = -rag_rand(20);
            prim_heading_degrees(dx, dz, &pipe->heading);
            pipe->alpha = 0;
            pipe->start.x = (float)rag_rand(100) * 0.06f - 3.0f + self->base.pos.x;
            pipe->start.z = (float)rag_rand(100) * 0.06f - 3.0f + self->base.pos.z;
            pipe->start.y = self->base.pos.y;
            back = (float)rag_rand(100) * 0.15f;
            pipe->start.x = pipe->start.x - g_cosTable[pipe->heading] * back;
            pipe->start.z = pipe->start.z - g_sinTable[pipe->heading] * back;
            pipe->end.x = pipe->start.x;
            pipe->end.z = pipe->start.z;
            pipe->end.y = self->base.pos.y;
            pipe->radius = 0.25f;
            pipe->drop = (float)rag_rand(100) * 0.04f + 8.0f;
            pipe->length = (float)rag_rand(100) * 0.2f + 20.0f;
            pipe->style = 4.0f;
        }
    }
    if (self->frame == 10)
        rag_play_wave_here(self, "effect\\EF_hit2.wav");
    if (self->frame == 20)
        rag_play_wave_here(self, "effect\\EF_hit4.wav");
}

void rag_master_whirl(struct RagEffect *self)
{
    int f = self->frame;

    if (f <= 7 || (f > 37 && f <= 45))
        self->master->roty += 12.0f;
    else if (f <= 15 || (f > 32 && f <= 37))
        self->master->roty += 18.0f;
    else if (f <= 20 || (f > 25 && f <= 32))
        self->master->roty += 24.0f;
    else if (f <= 25)
        self->master->roty += 48.0f;
    if (self->master->roty > 360.0f)
        self->master->roty -= 360.0f;
}
