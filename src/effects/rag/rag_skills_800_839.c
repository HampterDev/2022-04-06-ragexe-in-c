#include "render/projection.h"
#include "render/rp_pool.h"
#include "contents/sound.h"
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/actor_common.h"
#include "core/object_type.h"

#define EFFECT_DIR "이팩트\\"

#define RAG_MSG_SIZE(self)  ((self)->msg_params[0])
#define RAG_MSG_ANGLE(self) ((self)->msg_params[1])

static void textures_by_number(struct EffectPrim *p, int slot, const char *fmt, int first, int count)
{
    char name[64];
    int i;

    for (i = 0; i < count; i++) {
        sprintf(name, fmt, first + i);
        p->textures[slot + i] = RAG_TEXTURE(name);
    }
}

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

void rag_hallucination_double(struct RagEffect *self)
{
    struct World *world = game_current_world();
    struct RenderObject *m = self->master;
    struct RenderObject *dbl;
    void *mem;

    if (!world || !self->master)
        return;
    if (MASTER_TINTED(m)) {
        render_object_set_argb(m, -1, HALF_TINT(self, m, 0, self->base.pos.x),
                               HALF_TINT(self, m, 1, self->base.pos.y), HALF_TINT(self, m, 2, self->base.pos.z));
        if (self->frame == 9998)
            self->frame = 0;
        else if (self->frame != 0)
            return;
        if (!object_as_type(self->master, OBJECT_PC))
            return;
        mem = mem_alloc(sizeof(struct HalluPC));
        dbl = mem ? hallu_pc_construct(mem, self->master) : NULL;
        if (!dbl)
            return;
        m = self->master;
        render_object_set_argb(dbl, -1, HALF_TINT(self, m, 0, self->base.pos.x),
                               HALF_TINT(self, m, 1, self->base.pos.y), HALF_TINT(self, m, 2, self->base.pos.z));
    } else {
        render_object_set_argb(m, -1, (int)self->base.pos.x, (int)self->base.pos.y, (int)self->base.pos.z);
        if (self->frame == 9998)
            self->frame = 0;
        else if (self->frame != 0)
            return;
        if (!object_as_type(self->master, OBJECT_PC))
            return;
        mem = mem_alloc(sizeof(struct HalluPC));
        dbl = mem ? hallu_pc_construct(mem, self->master) : NULL;
        if (!dbl)
            return;
        render_object_send(dbl, &self->base, 0, (int)self->base.pos.x, (int)self->base.pos.y,
                           (int)self->base.pos.z, 0);
    }
    effect_add_world_object(world, dbl);
}

static void stacked_rings(struct RagEffect *self, int alpha_base, signed char style, const float *height)
{
    static const short turn[PRIM_RECORD_COUNT] = { 180, 270, 0, 90 };
    struct PrimRing *s = self->last_prim->records.rings;
    int i;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        s[i].active = 1;
        s[i].age = 0;
        s[i].span = 360;
        s[i].height = height[i];
        s[i].turn = turn[i];
        s[i].alpha = (short)(alpha_base + 45 * (3 - i));
        s[i].radius = 4.1f;
        s[i].tilt = 80;
        s[i].mode = style;
        s[i].offset_flag = 0.0f;
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        if (self->duration <= 60) {
            s[i].alpha = 0;
            s[i].age = -5 * i;
        }
    }
}

void rag_bubble_rings_144(struct RagEffect *self, int alpha_base,
                                     signed char style)
{
    static const float height[PRIM_RECORD_COUNT] = { 30.0f, 29.0f, 28.0f, 27.0f };
    static const short arc_start[PRIM_RECORD_COUNT] = { 180, 270, 0, 90 };
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    if (self->duration < 100)
        self->duration = 100;
    p = launch(self, 144);
    if (!p)
        return;
    if (style <= 1) {
        p->motion_count = 27;
        RAG_NEW_TEXTURES(p);
        textures_by_number(p, 0, "effect\\W_bubble%02d.tga", 1, 27);
        p->scale = style == 1 ? 4.0f : 0.0f;
    } else if (style <= 3) {
        p->motion_count = 29;
        RAG_NEW_TEXTURES(p);
        textures_by_number(p, 0, "effect\\W_A_top%02d.tga", 1, 29);
        p->scale = style == 3 ? 4.0f : 0.0f;
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWaveRing *ring = &self->last_prim->records.wave_rings[i];

        ring->active = 1;
        ring->frame = 0;
        ring->arc = 360;
        ring->base_height = height[i];
        ring->arc_start = arc_start[i];
        ring->alpha = (unsigned short)(alpha_base + 45 * (3 - i));
        ring->radius = 4.1f;
        ring->tilt = 80;
        ring->style = style;
    }
}

void rag_four_rings_139(struct RagEffect *self, int alpha_base, const char *tex,
                                   signed char style)
{
    static const float wide[PRIM_RECORD_COUNT] = { 25.0f, 24.0f, 23.0f, 22.0f };
    static const float narrow[PRIM_RECORD_COUNT] = { 15.0f, 14.0f, 13.0f, 12.0f };
    static const float usual[PRIM_RECORD_COUNT] = { 20.0f, 19.0f, 18.0f, 17.0f };
    struct EffectPrim *p;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    if (self->duration < 56)
        self->duration = 56;
    p = launch(self, 139);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 4.0f;
    stacked_rings(self, alpha_base, style, style == 2 ? wide : style == 22 ? narrow : usual);
}

void rag_cast_bubbles_149(struct RagEffect *self,
                                     signed char style)
{
    static const short arc_start[PRIM_RECORD_COUNT] = { 180, 270, 0, 90 };
    static const short tilt[PRIM_RECORD_COUNT] = { 75, 63, 52, 40 };
    struct EffectPrim *p;
    struct PrimWaveRing *s;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    rag_play_wave(self, "effect\\EF_BeginSpell.wav", -100.0f);
    if (self->duration < 120)
        self->duration = 120;
    p = launch(self, 149);
    if (!p)
        return;
    if (style == 10) {
        p->motion_count = 4;
        RAG_NEW_TEXTURES(p);
        textures_by_number(p, 0, "effect\\YBfire_%02d.bmp", 1, 4);
    } else {
        p->motion_count = 27;
        RAG_NEW_TEXTURES(p);
        textures_by_number(p, 0, "effect\\W_bubble%02d.tga", 1, 27);
    }
    p->scale = 0.0f;
    s = p->records.wave_rings;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        s[i].active = 1;
        s[i].frame = 0;
        s[i].arc = 360;
        s[i].base_height = 23.0f;
        s[i].arc_start = arc_start[i];
        s[i].alpha = 0;
        s[i].radius = 4.3f;
        s[i].tilt = tilt[i];
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        s[i].style = style;
}

static void square_from_message(struct RagEffect *self, struct PrimSpinSquare *sq)
{
    float a;

    sq->size = RAG_MSG_SIZE(self) < 5.0f ? 20.0f : RAG_MSG_SIZE(self);
    a = RAG_MSG_ANGLE(self);
    sq->angle = (short)(a < 0.0f || a >= 360.0f ? 45 : (int)a);
}

void rag_torch_ring_143(struct RagEffect *self, signed char style,
                                   short angle)
{
    struct EffectPrim *p;
    struct PrimSpinSquare *s;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = launch(self, 143);
    if (!p)
        return;
    if (style == 1) {
        p->motion_count = 13;
        RAG_NEW_TEXTURES(p);
        textures_by_number(p, 0, "effect\\torch_red%02d.bmp", 1, 13);
    } else if (style == 2) {
        p->motion_count = 51;
        RAG_NEW_TEXTURES(p);
        textures_by_number(p, 0, "effect\\W_c_%02d.tga", 0, 51);
    } else {
        p->motion_count = 9;
        RAG_NEW_TEXTURES(p);
        textures_by_number(p, 0, "effect\\dust%02d.bmp", 1, 9);
    }
    s = p->records.spin_squares;
    s[0].active = 1;
    s[1].active = 0;
    s[2].active = 0;
    s[3].active = 0;
    s[0].frame = 0;
    if (style == 2) {
        square_from_message(self, &s[0]);
        s[0].alpha = 200;
        s[0].still = 1.0f;
    } else {
        s[0].size = 18.0f;
        s[0].angle = angle;
        s[0].alpha = 200;
        s[0].still = 0.0f;
    }
    s[0].pulse = (short)rag_rand(360);
    s[0].y = -2.0f;
    s[0].texture = 0.0f;
}

void rag_two_frame_ring_148(struct RagEffect *self, const char *tex0, const char *tex1)
{
    struct EffectPrim *p;
    struct PrimSpinSquare *s;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = launch(self, 148);
    if (!p)
        return;
    p->motion_count = 2;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex0);
    p->textures[1] = RAG_TEXTURE(tex1);
    s = p->records.spin_squares;
    s[0].active = 1;
    s[1].active = 0;
    s[2].active = 0;
    s[3].active = 0;
    s[0].frame = 0;
    square_from_message(self, &s[0]);
    s[0].pulse = (short)rag_rand(360);
    s[0].y = -2.0f;
    s[0].texture = 0.0f;
}

void rag_throw_lines(struct RagEffect *self, short side, const char *tex)
{
    struct EffectPrim *p;
    struct PrimSlideStripe *st;
    int which, i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 56, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 0x201;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    which = side % 10;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        st = &p->records.slide_stripes[i];
        st->active = 1;
        st->age = 0;
        st->back = 100.0f;
        if (which == 0)
            st->side_shift = -15.0f - (float)rag_rand(11);
        else if (which == 1)
            st->side_shift = (float)rag_rand(11) + 15.0f;
        st->start = (float)rag_rand(200);
        st->half_width = (float)rag_rand(6) * 0.1f + 0.5f;
        st->length = (float)rag_rand(31) + 40.0f;
        st->color = (unsigned char)(rag_rand(3) + 10);
        rag_rand(11);
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &st->heading);
        st->alpha = 0;
    }
}

void rag_thunder_spheres(struct RagEffect *self)
{
    int n, turn;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    n = (int)self->offset.x;
    if (n > 5)
        n = 5;
    else if (n <= 0)
        return;
    for (turn = 0; n != 0; n--, turn += 360) {
        struct EffectPrim *p;
        struct PrimDriftOrb *orb;
        int i;

        p = self->last_prim = rag_launch_prim(self, 145, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->list_flags = 5;
        p->duration = self->duration;
        orb = &p->records.drift_orbs[0];
        orb->active = 1;
        orb->drift[0][0] = (float)rag_rand(360);
        orb->drift[0][1] = (float)rag_rand(360);
        orb->drift[1][0] = (float)rag_rand(360);
        orb->drift[1][1] = (float)rag_rand(360);
        orb->drift[2][0] = (float)rag_rand(360);
        orb->drift[2][1] = (float)rag_rand(360);
        orb->spin = (float)turn / self->offset.x;
        orb->lift = -15.0f;
        orb->size = 1.0f;
        orb->wide = 0.0f;
        orb->style = 1;
        orb->shimmer = (float)rag_rand(360);
        orb->alpha = 0;
        p->motion_count = 2;
        RAG_NEW_TEXTURES(p);
        p->textures[0] = RAG_TEXTURE("effect\\thunder_center.bmp");
        p->textures[1] = RAG_TEXTURE("effect\\freeze_a.bmp");
        for (i = 0; i < PRIM_RECORD_COUNT; i++) {
            struct PrimDriftOrb *blade = &self->last_prim->records.drift_orbs[i];

            blade->frame = -20 * i;
            blade->sway = (short)rag_rand(360);
            blade->grow = 3.0f;
            blade->angle = (short)rag_rand(360);
            blade->blade_alpha = 200;
        }
    }
}

void rag_cloaking_fade(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\assasin_cloaking.wav");
    render_object_set_argb(self->master, 50, 0, 0, 0);
    self->base.has_shadow = 0;
}

void rag_project_master(struct RagEffect *self)
{
    float view[12];
    Vec3 at;
    struct TexDrawVertex screen;

    if (!self->master || !game_copy_view_matrix(view))

        return;
    at = self->master->pos;
    memset(&screen, 0, sizeof(screen));
    screen.specular = 0xff000000;
    renderer_project_vertex(g_renderer, &at, view, &screen);
    self->screen_pos.x = screen.x;
    self->screen_pos.y = screen.y;
    self->base.pos = self->master->pos;
}

struct EffectPrim *rag_spark_launch(struct RagEffect *self, float rise, float accel)
{
    struct EffectPrim *p;
    float dist;

    p = self->last_prim = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;
    p->duration = 50;
    mat_set_rotate_y(p->matrix, deg_to_rad(rag_rand(360)));
    dist = (float)(rag_rand(7) + 3);
    p->delta2 = rag_mat_apply(p->matrix, 0.0f, 0.0f, dist);
    p->delta2.y -= rise;
    mat_set_rotate_x(p->matrix, deg_to_rad(-90));
    p->accel = accel;
    p->size[1].value = (float)(rag_rand(60) + 30) / 10.0f;
    p->size[2].value = 0.18f;
    p->alpha = 0.0f;
    p->alpha_max = 200.0f;
    return p;
}

void rag_spark_finish(struct EffectPrim *p)
{
    p->alpha_speed = p->alpha_max / 20.0f;
    p->fade_frame = p->duration - 20;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE("effect\\ac_center2.tga");
    p->variant = 0;
}

static int spark_channel(void)
{
    return rand() % 3 == 0 ? 255 : 140;
}

void rag_rising_sparks(struct RagEffect *self)
{
    struct EffectPrim *p;

    rag_project_master(self);
    if (self->frame & 1)
        return;
    p = rag_spark_launch(self, 20.0f, 0.015f);
    if (!p)
        return;
    p->r = spark_channel();
    p->g = spark_channel();
    p->b = spark_channel();
    rag_spark_finish(p);
}

void rag_tetra_vortex(struct RagEffect *self, signed char style)
{
    static const char *const tv[8] = {
        "effect\\tv-1.bmp", "effect\\tv-2.bmp", "effect\\tv-3.bmp", "effect\\tv-4.bmp",
        "effect\\tv-5.bmp", "effect\\tv-6.bmp", "effect\\tv-7.bmp", "effect\\tv-8.bmp",
    };
    struct EffectPrim *p;
    struct PrimTexRecord *recs;
    int i;

    self->base.pos = self->master->pos;
    switch (self->frame) {
    case 0:
        rag_play_wave_here(self, "effect\\wizard_meteo.wav");
        rag_play_wave_here(self, "effect\\hunter_blastmine.wav");
        break;
    case 10:
    case 20:
    case 50:
    case 80:
        rag_play_wave_here(self, "effect\\wl_tetravortex.wav");
        break;
    }
    if (self->frame > 80 || self->frame % 25 != 0)
        return;
    p = launch(self, 140);
    if (!p)
        return;
    if (style == 4) {
        p->motion_count = 8;
        RAG_NEW_TEXTURES(p);
        for (i = 0; i < 8; i++)
            p->textures[i] = RAG_TEXTURE(tv[i]);
    }
    p->scale = 0.0f;
    recs = p->records.tex_records;
    recs[0].active = 0;
    recs[1].active = 0;
    recs[2].active = 0;
    recs[3].active = 0;
    recs[0].age = 0;
    recs[0].active = 1;
    recs[0].size = RAG_MSG_SIZE(self) <= 0.0f ? 30.0f : RAG_MSG_SIZE(self);
    recs[0].point[2].y = self->base.pos.y - 10.0f;
    if (style == 4) {
        switch (self->effect_id) {
        case 970: recs[0].look = 20; break;
        case 971: recs[0].look = 18; break;
        case 972: recs[0].look = 17; break;
        case 973: recs[0].look = 19; break;
        }
        recs[0].spin = 120;
        recs[0].shape = 1.0f;
    } else {
        recs[0].look = 17;
        recs[0].spin = 135;
        recs[0].shape = 0.0f;
    }
    recs[0].pulse = 0.0f;
    recs[0].alpha = 200;
    recs[0].point[0].x = self->offset.x;
    recs[0].point[0].z = self->offset.z;
    recs[0].point[0].y = self->offset.y - 35.0f;
    recs[0].angle = (short)rag_rand(360);
    recs[0].texture = 0;
    recs[0].style = style;
}

void rag_ring_scatter_137(struct RagEffect *self, const char *tex, signed char style)
{
    struct EffectPrim *p;
    int i, step;

    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = launch(self, 137);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    if (style <= 1)
        p->scale = 4.0f;
    else if (style == 2)
        p->scale = 7.0f;
    else if (style == 3)
        p->scale = 6.0f;
    else if (style == 10)
        p->scale = (float)(rand() % 5 + 4);
    else if (style == 11)
        p->scale = 5.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        self->last_prim->records.crown_rings[i].active = 0;
    for (i = 0, step = 0; step < 80; i++, step += 20) {
        struct PrimCrownRing *crown = &self->last_prim->records.crown_rings[i];
        float dist;
        short deg;

        crown->active = 1;
        crown->arc = 360;
        crown->arc_start = (short)(i * 90);
        if (style == 11) {
            crown->alpha = 50;
            crown->base_height = (float)step + 60.0f;
        } else {
            crown->alpha = 150;
            crown->base_height = 120.0f;
        }
        crown->pos.y = 0.0f;
        if (style == 10) {
            crown->frame = rand() % 100;
            dist = (float)(rand() % 41);
        } else {
            crown->frame = rand() % 40 + 60;
            if (style == 11)
                dist = (float)(rand() % 31) * 0.1f + 6.0f;
            else
                dist = 5.0f;
        }
        deg = (short)rag_rand(360);
        crown->pos.x = g_cosTable[deg] * dist;
        crown->pos.z = g_sinTable[deg] * dist;
        crown->radius = (float)(rand() % 11) * 0.015f + 0.15f;
        crown->tilt = 90;
        crown->offset.x = self->offset.x - self->base.pos.x;
        crown->offset.y = self->offset.y - self->base.pos.y;
        crown->offset.z = self->offset.z - self->base.pos.z;
    }
}

void rag_rocket_punches(struct RagEffect *self)
{
    struct EffectPrim *p;
    float dist;
    short heading, a;

    if (self->frame != 0 && self->frame != 2 && self->frame != 4 && self->frame != 6)
        return;
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    p->pattern |= 0x100;
    p->delta2.x = 0.0f;
    p->delta2.y = -15.0f;
    p->delta2.z = 0.0f;
    render_object_turn_toward(&p->base, self->offset.x, self->offset.z);
    p->angle[0] = -p->base.roty;
    if (0.0f > p->base.roty)
        p->base.roty += 360.0f;
    dist = render_object_calc_ground_dist(&p->base, self->offset.x, self->offset.z);
    p->speed = -(dist / (float)p->duration);
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->scale = 1.0f;
    p->motion_period = 1;
    if (self->frame == 0)
        p->pattern |= 0x200;
    else
        p->alpha = (float)((8 - self->frame) * 10);
    render_object_load_spr_res(&p->base, EFFECT_DIR "남_로켓트펀치.spr");
    render_object_load_act_res(&p->base, EFFECT_DIR "남_로켓트펀치.act");
    prim_heading_degrees(self->base.pos.x - self->offset.x, self->base.pos.z - self->offset.z, &heading);
    a = (short)((int)game_camera_heading() + heading);
    a = (short)(a % 360);
    if (a < 0)
        a += 360;
    if (a >= 180 && a < 270)
        render_object_set_action_base(&p->base, 4, 1);
    else if (a >= 90 && a < 180)
        render_object_set_action_base(&p->base, 6, 1);
    else if ((unsigned short)a <= 89)
        render_object_set_action_base(&p->base, 0, 1);
    else
        render_object_set_action_base(&p->base, 2, 1);
    prim_sync_particles(p);
}

void rag_bubble_spiral_146(struct RagEffect *self,
                                      unsigned char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = launch(self, 146);
    if (!p)
        return;
    p->motion_count = 13;
    RAG_NEW_TEXTURES(p);
    for (i = 0; i < 13; i++) {
        char name[64];

        sprintf(name, "effect\\W_bubble%02d.tga", 2 + 2 * i);
        p->textures[i] = RAG_TEXTURE(name);
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWaveRing *s = &self->last_prim->records.wave_rings[i];

        s->active = 1;
        s->frame = -8 * i;
        s->base_height = 30.0f;
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &s->arc_start);
        s->arc = 315;
        s->arc_start += 195;
        if (s->arc_start >= 360)
            s->arc_start -= 360;
        s->alpha = 0;
        s->radius = 3.0f - (float)i * 0.1f;
        s->tilt = (short)((14 - i) * 5);
        s->walked.x = 0.0f;
        s->walked.z = 0.0f;
        s->strip_style = style == 1 ? 7.0f : 0.0f;
    }
}

void rag_two_rings_161(struct RagEffect *self, const char *tex, signed char style)
{
    struct EffectPrim *p;
    struct PrimTexRecord *recs;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 161, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    if (style == 1)
        p->scale = 15.0f;
    else if (style == 2)
        p->scale = 17.0f;
    else if (style == 3)
        p->scale = 18.0f;
    else
        p->scale = 4.0f;
    recs = p->records.tex_records;
    recs[0].active = 1;
    recs[1].active = 1;
    recs[2].active = 0;
    recs[3].active = 0;
    recs[0].size = style == 3 ? 24.0f : 15.0f;
    recs[1].size = recs[0].size;
    recs[0].age = 0;
    recs[1].age = 0;
    recs[0].bob = 0;
    recs[1].bob = 180;
    recs[0].alpha = 200;
    recs[1].alpha = 200;
    recs[0].y = -1.0f;
    recs[1].y = -1.0f;
}

void rag_white_rings_scaled(struct RagEffect *self, signed char style)
{
    struct EffectPrim *p;
    struct PrimRing *s;

    if (self->frame % 100 != 0)
        return;
    p = launch(self, 63);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE("effect\\ring_white.tga");
    if (style == 1)
        p->scale = 8.0f;
    else if (style == 2)
        p->scale = 6.0f;
    else
        p->scale = 4.0f;
    s = p->records.rings;
    prim_sub_ring(&s[0], 0, 6.001f, 0, 0.0f, 2, 100, 1);
    prim_sub_ring(&s[1], -10, 6.001f, 25, 0.0f, 3, 100, 1);
    prim_sub_ring(&s[2], -20, 6.001f, 50, 0.0f, 4, 100, 1);
    s[3].active = 0;
    s[3].age = 0;
    s[3].mode = 4;
}

void rag_freezing_columns(struct RagEffect *self, const char *tex, unsigned char style)
{
    static const float drop[PRIM_RECORD_COUNT] = { 30.0f, 20.0f, 10.0f, 0.0f };
    struct EffectPrim *p;
    struct PrimStretchRay *ray;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = launch(self, 141);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    ray = p->records.stretch_rays;
    ray[0].active = 0;
    ray[1].active = 0;
    ray[2].active = 0;
    ray[3].active = 0;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ray = &self->last_prim->records.stretch_rays[i];
        ray->active = 1;
        ray->frame = -10 * i;
        ray->size = 20.0f;
        ray->height = self->base.pos.y - drop[i];
        ray->angle = 215;
        ray->alpha = 255;
        ray->pos.x = self->base.pos.x;
        ray->pos.z = self->base.pos.z;
        ray->stretch = 0;
        ray->texture = 0;
    }
}

void rag_water_ring(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimAnimatedPanel *panel;
    float a;

    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = launch(self, 142);
    if (!p)
        return;
    p->motion_count = 117;
    RAG_NEW_TEXTURES(p);
    textures_by_number(p, 0, "effect\\W_C_water_%02d.tga", 0, 30);
    textures_by_number(p, 30, "effect\\W_B_water_%02d.tga", 0, 30);
    textures_by_number(p, 60, "effect\\W_bubble%02d.tga", 1, 27);
    textures_by_number(p, 87, "effect\\W_A_water_%02d.tga", 0, 30);
    panel = p->records.animated_panels;
    panel[0].active = 0;
    panel[1].active = 0;
    panel[2].active = 0;
    panel[3].active = 0;
    panel[0].active = 1;
    panel[0].frame = 0;
    panel[0].alpha = 230;
    panel[0].size = RAG_MSG_SIZE(self) < 20.0f ? 40.0f : RAG_MSG_SIZE(self);
    a = RAG_MSG_ANGLE(self);
    panel[0].angle = (short)(a >= 360.0f ? 0 : (int)a);
}

#define UI_ITEM "유저인터페이스\\item\\"

void rag_random_tint_rings(struct RagEffect *self)
{
    int r, g, b;

    if (self->frame == 0)
        sound_play_wave("effect\\EF_BeginSpell.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
    if (self->frame % 5 <= 1)
        self->master->overlay_flags |= 1;
    b = rand() % 256;
    g = rand() % 256;
    r = rand() % 256;
    render_object_set_argb(self->master, -1, r, g, b);
    rag_four_rings_139(self, 25, "effect\\ring_white.tga", 0);
    rag_four_rings_139(self, 45, "effect\\ring_white.tga", 0);
}

void rag_detecting_blink(struct RagEffect *self)
{
    struct RenderObject *m;

    if ((unsigned)(self->frame - 6) <= 54) {
        if (self->frame & 1)
            render_object_set_argb(self->master, -1, 255, 255, 255);
        else
            render_object_set_argb(self->master, -1, 0, 0, 0);
    } else if (self->frame >= 61) {
        m = self->master;
        if (m->is_pc != 0 && (((struct GameActor *)m)->health_state & 1)) {
            m->argb_locked = 1;
            render_object_set_argb(self->master, -1, 255, 128, 255);
        }
    }
    rag_detecting(self, 6);
}

void rag_trinket_toss(struct RagEffect *self)
{
    rag_toss_wave(self);
    rag_tumbling_items(self, UI_ITEM "갈색반지.bmp", 5.0f, 1);
    rag_tumbling_items(self, UI_ITEM "금반지.bmp", 5.0f, 1);
    rag_tumbling_items(self, UI_ITEM "금방울.bmp", 5.0f, 1);
    rag_tumbling_items(self, UI_ITEM "꽃반지.bmp", 5.0f, 1);
    rag_tumbling_items(self, UI_ITEM "낡은펜던트.bmp", 5.0f, 1);
    rag_tumbling_items(self, UI_ITEM "눈구슬반지.bmp", 5.0f, 1);
    rag_tumbling_items(self, UI_ITEM "상자.bmp", 4.0f, 1);
    rag_tumbling_items(self, UI_ITEM "수제초콜렛.bmp", 5.0f, 1);
}
