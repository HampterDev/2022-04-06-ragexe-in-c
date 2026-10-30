#include "render/texture_cache.h"
#include "resources/animation.h"
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

void rag_ring_at_frame_100(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimRing *ring;

    if (self->frame != 100)
        return;
    follow_master(self);
    p = launch_prim(self, 74, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    self->base.pos.y -= 10.0f;
    p->records.rings[0].active = 0;
    p->records.rings[1].active = 0;
    p->records.rings[2].active = 0;
    p->records.rings[3].active = 0;
    ring = &p->records.rings[0];
    ring->active = 1;
    ring->age = 0;
    ring->radius = 8.0f;
    ring->alpha = 0;
    ring->span = 360;
}

void rag_spell_casting_rings(struct RagEffect *self, const char *tex)
{
    follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
    rag_casting_circle(self, 45, tex, 3);
    rag_casting_circle(self, 25, tex, 3);
}

void rag_casting_rings_white_yellow(struct RagEffect *self)
{
    follow_master(self);
    if (self->frame != 0)
        return;
    rag_casting_circle(self, 45, "effect\\ring_white.tga", 22);
    rag_casting_circle(self, 25, "effect\\ring_yellow.tga", 22);
}

static struct PrimRing *ring_record(struct EffectPrim *p, int i, float height, float radius, short tilt,
                                    short fade_from, unsigned char style)
{
    struct PrimRing *r = &p->records.rings[i];
    int k;

    r->active = 1;
    r->age = 0;
    r->span = 360;
    r->height = height;
    r->turn = (short)rand_below(360);
    r->alpha = 0;
    r->radius = radius;
    r->tilt = tilt;
    r->mode = fade_from;
    for (k = 0; k < PRIM_RING_COLUMNS; k++) {
        r->column[k] = 0.0f;
        r->column_flag[k] = style;
    }
    return r;
}

void rag_two_rings(struct RagEffect *self, const char *tex, float offset_x,
                              float offset_z)
{
    struct EffectPrim *p;
    struct PrimRing *r;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 110, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 10.0f;
    r = ring_record(p, 0, 12.0f, 2.3f, 86, (short)(self->duration - 25), 0x21);
    r->offset_x = offset_x;
    r->offset_z = offset_z;
    r->offset_flag = 1234.5f;
    r = ring_record(p, 1, 25.0f, 2.0f, 90, (short)(self->duration - 25), 0x21);
    r->offset_x = offset_x;
    r->offset_z = offset_z;
    r->offset_flag = 1234.5f;
    p->records.rings[2].active = 0;
    p->records.rings[3].active = 0;
}

void rag_four_blue_rings(struct RagEffect *self)
{
    struct EffectPrim *p;

    follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave(self, "effect\\EF_Portal.wav", -150.0f);
    p = launch_prim(self, 60, 0);
    if (!p)
        return;
    p->textures[0] = texture("effect\\ring_blue.tga");
    p->scale = 4.0f;
    ring_record(p, 0, 30.0f, 3.7f, 90, 1400, 2)->offset_flag = 0.0f;
    ring_record(p, 1, 30.0f, 3.4f, 90, 1400, 2)->offset_flag = 0.0f;
    ring_record(p, 2, 4.0f, 3.6f, 10, 1400, 2)->offset_flag = 0.0f;
    ring_record(p, 3, 4.0f, 3.7f, 5, 1400, 2)->offset_flag = 0.0f;
}

void rag_two_flat_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimArch *arch;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 76, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 4.0f;
    p->records.arches[3].active = 0;
    p->records.arches[2].active = 0;
    for (i = 0; i < 2; i++) {
        arch = &p->records.arches[i];
        arch->active = 1;
        arch->age = 0;
        arch->radius = 10.0f;
        arch->half_width = 2.0f;
        arch->lift = -8.0f;
        arch->turn = 0;
        arch->phase = 0;
        arch->alpha = 200;
        arch->wobble = 0;
    }
}

void rag_four_tilted_rings(struct RagEffect *self, const char *tex, int angle,
                                      unsigned char wide)
{
    struct EffectPrim *p;
    struct PrimRing *ring;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 77, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = wide == 1 ? 10.0f : 4.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        p->records.rings[i].active = 0;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ring = &p->records.rings[i];
        ring->active = 1;
        ring->age = 0;
        ring->height = (float)rand_below(11) * 0.2f + 4.0f;
        ring->radius = 14.0f;
        ring->column[0] = -14.0f;
        ring->turn = (short)rand_below(360);
        ring->tilt = (short)(i * 45 + angle);
        ring->alpha = 100;
        ring->span = (short)(rand_below(91) + 180);
        ring->column[1] = (float)(rand_below(4) + 3);
    }
}

void rag_wedding_paper(struct RagEffect *self, const char *tex, unsigned char silent)
{
    struct EffectPrim *p;
    struct PrimTiltedSquare *sq;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    if (silent == 0)
        rag_play_wave_here(self, "effect\\wedding.wav");
    p = launch_prim(self, 78, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.tilted_squares[i];
        sq->active = 1;
        sq->age = 0;
        sq->radius = (float)rand_below(31) * 0.01f + 0.3f;
        sq->turn = (short)rand_below(360);
        sq->tilt = (short)(i * 45);
        sq->alpha = 0;
        sq->red = (float)rand_below(360);
        sq->green = (float)rand_below(360);
        sq->blue = (float)rand_below(360);
        sq->pos.x = (float)(rand_below(101) - 50);
        sq->pos.y = (float)(-110 - rand_below(50));
        sq->pos.z = (float)(rand_below(101) - 50);
        sq->fall = (float)rand_below(26) * 0.01f + 0.25f;
    }
}

void rag_hit_sparks(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimSpike *sp;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 81, 2);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sp = &p->records.spikes[i];
        sp->active = 1;
        sp->age = 0;
        sp->alpha = 40;
        sp->length = 1.0f;
        sp->depth = 10.0f;
        sp->tilt = (short)rand_below(360);
        sp->turn = (short)rand_below(360);
        sp->swell = (short)rand_below(360);
        sp->swell_speed = (float)(rand_below(2) + 1);
        if (rand_below(2) == 0) {
            sp->red = 105.0f;
            sp->green = 105.0f;
            sp->blue = 255.0f;
        } else {
            sp->red = 55.0f;
            sp->green = 55.0f;
            sp->blue = 225.0f;
        }
    }
}

void rag_water_fall(struct RagEffect *self, unsigned char lengthwise,
                               unsigned char size, unsigned char look)
{
    struct EffectPrim *p;
    struct PrimScrollColumn *col;
    int i, k, steps;

    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 79, 3);
    if (!p)
        return;
    if (look == 0) {
        p->textures[0] = texture("effect\\waterfall11.tga");
        p->textures[1] = texture("effect\\waterfall12.tga");
        p->textures[2] = texture("effect\\waterfall13.tga");
    } else if (look == 1) {
        p->textures[0] = texture("effect\\waterfall31.tga");
        p->textures[1] = texture("effect\\waterfall32.tga");
        p->textures[2] = texture("effect\\waterfall33.tga");
    }
    for (i = 0, steps = 80; i < PRIM_RECORD_COUNT; i++, steps -= 13) {
        col = &p->records.scroll_columns[i];
        col->active = 1;
        col->age = 0;
        col->alpha = 80;
        col->lengthwise = lengthwise;
        if (size == 0)
            col->width = (float)(i + 36);
        else if (size == 1)
            col->width = (float)(i + 18);
        col->squash = (signed char)(int)RAG_SIZE_PARAM(self);
        for (k = 0; k < 6; k++)
            col->edge[k] = (float)(k * 40);
        col->depth = (float)i - 1.0f;
        col->steps = (float)steps;
        col->style = 0;
    }
}

static void spray_drop(const struct PrimAuraGroup *g, Vec3 *drop, struct PrimSway *sway)
{
    int i, x;

    drop->x = (float)(g->narrow == 0 ? rand_below(45) - 22 : rand_below(23) - 11);
    drop->y = (float)rand_below(30);
    drop->z = (float)(rand_below(11) - 5);
    for (i = 0; i < 2; i++) {
        sway[i].angle = (float)rand_below(360);
        sway[i].target = (float)rand_below(360);
    }
    if (g->swap_xz == 1) {
        x = (int)drop->x;
        drop->x = drop->z;
        drop->z = (float)x;
    }
}

void rag_water_fall_spray(struct RagEffect *self, unsigned char sideways,
                                     unsigned char narrow)
{
    struct EffectPrim *p;
    struct PrimAuraGroup *g;
    int i, small;

    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 80, 1);
    if (!p)
        return;
    small = rand_below(10) > 0;
    if (small)
        p->textures[0] = texture("effect\\freeze_a_small.bmp");
    else
        p->textures[0] = texture("effect\\waterp1.tga");
    p->scale = 4.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        g = &p->records.aura_groups[i];
        g->active = 1;
        g->speed = 0.05f;
        g->swap_xz = sideways;
        g->narrow = narrow;
        g->size = small ? 4.5f : 7.0f;
        spray_drop(g, &g->point[2], &g->sway[0]);
        spray_drop(g, &g->point[0], &g->sway[2]);
        spray_drop(g, &g->point[3], &g->sway[4]);
        spray_drop(g, &g->point[1], &g->sway[6]);
        g->style = 2;
    }
}

void rag_ghost_or_bat(struct RagEffect *self, char mode)
{
    struct EffectPrim *p;
    struct PrimSpriteMover *mv;

    follow_master(self);
    if (self->frame >= 10)
        return;
    p = rag_launch_prim(self, 84, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = self->duration;
    mv = &p->records.sprite_mover;
    mv->age = 0;
    mv->active = 1;
    p->scale = 1.5f;
    mv->centre.x = self->base.pos.x;
    mv->centre.y = self->base.pos.y;
    mv->centre.z = self->base.pos.z;
    mv->angle = 0;
    mv->bob = 0;
    if (mode == 2) {
        mv->style = 2;
        if (self->frame == 0)
            mv->sway[0][1] = 250.0f;
        else
            mv->sway[0][1] = (float)((10 - self->frame) * 3);
    } else {
        mv->style = 1;
        mv->sway[0][1] = (float)((10 - self->frame) * 3);
    }
    mv->mode = (signed char)mode;
    mv->radius = 16.0f;
    mv->speed = 4.0f;
    p->alpha = 0.0f;
    if (mode == 0) {
        render_object_load_spr_res(&p->base, EFFECT_DIR "유령.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "유령.act");
    } else if (mode >= 1) {
        render_object_load_spr_res(&p->base, EFFECT_DIR "박쥐.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "박쥐.act");
    }
    p->motion_period = 2;
    p->list_flags |= 0x200;
    render_object_set_action_base(&p->base, 0, 1);
    prim_sync_particles(p);
}

void rag_slash_waves(struct RagEffect *self, char sonic)
{
    struct EffectPrim *p;
    struct PrimTurnedSquare *sq;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 86, sonic == 1 ? 4 : 1);
    if (!p)
        return;
    if (sonic == 1) {
        p->textures[0] = texture("effect\\sonic_wave1.tga");
        p->textures[1] = texture("effect\\sonic_wave2.tga");
        p->textures[2] = texture("effect\\sonic_wave3.tga");
        p->textures[3] = texture("effect\\sonic_wave4.tga");
    } else {
        p->textures[0] = texture("effect\\purpleslash.tga");
    }
    p->scale = 0.0f;
    p->delta = self->offset;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.turned_squares[i];
        sq->active = 1;
        sq->alpha = 0;
        sq->age = i - 28;
        sq->radius = 20.0f;
        sq->pos.x = self->base.pos.x;
        sq->pos.z = self->base.pos.z;
        sq->pos.y = (float)rand_below(5) + (self->base.pos.y - 10.0f);
        sq->turn = 0;
        sq->speed = 0.8f;
        sq->style = (unsigned char)sonic;
    }
}

void rag_valentine_sprite(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    int action = 0;

    follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\vallentine.wav");
    p = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->pattern |= 0x200;
    p->duration = self->duration;
    p->delta2.y = 0.0f;
    p->motion_period = 2;
    p->loop_motion = 0;
    if (look == 2) {
        render_object_load_spr_res(&p->base, EFFECT_DIR "05vallentine.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "05vallentine.act");
    } else if (look == 3) {
        render_object_load_spr_res(&p->base, EFFECT_DIR "fast.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "fast.act");
        p->motion_period = 4;
    } else if (look == 4) {
        p->list_flags |= 0x200;
        render_object_load_spr_res(&p->base, EFFECT_DIR "ro2effect.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "ro2effect.act");
        p->motion_period = 4;
    } else {
        render_object_load_spr_res(&p->base, EFFECT_DIR "vallentine.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "vallentine.act");
        action = look;
    }
    render_object_set_action_base(&p->base, action, 1);
    prim_sync_particles(p);
}

void rag_billboard(struct RagEffect *self, const char *tex, char style)
{
    struct EffectPrim *p;
    struct PrimBillboard *bb;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 87, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    bb = &p->records.billboards[0];
    bb->active = 1;
    bb->age = 0;
    if (style == 0)
        bb->size = 12.0f;
    else if (style == 4)
        bb->size = 14.0f;
    else
        bb->size = 8.0f;
    bb->alpha = 0;
    bb->local.x = 0.0f;
    bb->local.z = 0.0f;
    if (style == 0)
        bb->local.y = -115.0f;
    else if (style == 4)
        bb->local.y = -115.0f - (float)(rand() % 56);
    else
        bb->local.y = -80.0f;
    bb->heading = 0;
    bb->speed = 3.0f;
    bb->spin = (short)rand_below(360);
    bb->texture = 0;
    bb->color_style = 7;
    if (style == 1) {
        bb->color_style = 9;
        bb->color_substyle = (unsigned char)style;
    } else if (style == 2) {
        bb->color_style = 9;
        bb->color_substyle = 3;
    } else if (style == 3) {
        bb->color_style = 9;
        bb->color_substyle = 4;
    }
    bb->circle_state = (unsigned char)style;
    p->records.billboards[1].active = 1;
    p->records.billboards[1].alpha = 0;
    p->records.billboards[2].active = 1;
    p->records.billboards[2].alpha = 0;
    p->records.billboards[3].active = 1;
    p->records.billboards[3].alpha = 0;
}

void rag_four_turned_rings(struct RagEffect *self, const char *tex, char turn,
                                      char style)
{
    struct EffectPrim *p;
    struct PrimFanWedge *fw;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 89, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        fw = &p->records.fan_wedges[i];
        fw->active = 1;
        fw->style = (float)style;
        if (style == 2 || style == 3 || style == 6)
            fw->age = 0;
        else
            fw->age = -24;
        fw->length = 2.0f;
        fw->heading = (short)(i * 90 + (style == 3 ? turn * 7 : turn * 22));
        fw->turn = (short)rand_below(360);
        fw->alpha = 0;
        fw->height = -12.0f;
    }
}

void rag_glow(struct RagEffect *self, const char *tex, int age, char style)
{
    struct EffectPrim *p;
    struct PrimTexRecord *recs;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 88, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    if (style == 1)
        p->scale = 101.0f;
    else if (style == 2)
        p->scale = 102.0f;
    else if (style == 3)
        p->scale = 111.0f;
    else if (style == 4)
        p->scale = 113.0f;
    else if (style == 10)
        p->scale = 114.0f;
    recs = p->records.tex_records;
    recs[0].active = 1;
    recs[0].age = age;
    if (style == 3) {
        recs[0].size = 13.0f;
        recs[0].y = -5.0f - (float)(rand() % 6);
    } else {
        recs[0].size = 15.0f;
        recs[0].y = -5.0f;
    }
    recs[0].bob = 0;
    recs[0].alpha = 0;
    recs[1].active = 0;
    recs[2].active = 0;
    recs[3].active = 0;
}

static float master_clip_height(struct RagEffect *self)
{
    void *act, *motion;
    const struct SprClip *clip;
    float h;

    if (self->master && (act = render_object_get_act_res(self->master)) != NULL
        && (motion = act_res_get_motion(act, 0, 0)) != NULL
        && (clip = motion_get_clip(motion, 0)) != NULL) {
        h = (float)clip->y * 0.29f;
        if (h != 0.0f)
            return h - 4.0f;
    }
    return -30.0f;
}

void rag_effect_texture(struct RagEffect *self, const char *tex, char style)
{
    struct EffectPrim *p;
    struct PrimTexRecord *rec;
    float size, y;
    float shape = 0.0f;
    int spin = 135;
    int random_spin = 0;

    follow_master(self);
    if (self->frame != 0)
        return;
    follow_master(self);
    p = launch_prim(self, 82, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    p->records.tex_records[0].active = 0;
    p->records.tex_records[1].active = 0;
    p->records.tex_records[2].active = 0;
    p->records.tex_records[3].active = 0;
    rec = &p->records.tex_records[0];
    rec->active = 1;
    rec->age = 0;
    size = RAG_SIZE_PARAM(self);
    y = self->base.pos.y;
    switch (style) {
    case 1:
        rec->size = 8.0f;
        rec->point[0].y = y - 30.0f;
        rec->look = 0;
        shape = 1.0f;
        spin = 120;
        break;
    case 2:
        rec->size = 10.0f;
        rec->point[0].y = y - 10.0f;
        rec->look = 2;
        break;
    case 3:
        rec->size = 8.0f;
        rec->point[0].y = y - 30.0f;
        rec->look = 3;
        shape = 2.0f;
        spin = 155;
        break;
    case 4:
        rec->size = 6.0f;
        rec->point[0].y = y - 30.0f;
        rec->look = 0;
        break;
    case 10:
    case 11:
        rec->size = size <= 0.0f ? 70.0f : size * 0.7f;
        rec->point[0].y = y;
        rec->look = 10;
        random_spin = 1;
        break;
    case 12:
        rec->size = size <= 0.0f ? 120.0f : size * 1.2f;
        rec->point[0].y = y;
        rec->look = 12;
        random_spin = 1;
        break;
    case 13:
        rec->size = size <= 0.0f ? 300.0f : size * 3.0f;
        rec->point[0].y = y;
        rec->look = 12;
        random_spin = 1;
        break;
    case 14:
        rec->size = size <= 0.0f ? 30.0f : size;
        rec->point[0].y = y;
        rec->look = 4;
        random_spin = 1;
        break;
    case 15:
        rec->size = size <= 0.0f ? 30.0f : size;
        rec->point[0].y = y;
        rec->look = 15;
        random_spin = 1;
        break;
    case 16:
    case 17:
        rec->size = size <= 0.0f ? 9.5f : size * 0.9f;
        if (style == 17)
            rec->size = rec->size * 0.85f;
        rec->point[0].y = y;
        rec->look = 5;
        random_spin = 1;
        break;
    case 18:
        rec->size = size <= 0.0f ? 9.0f : size * 0.9f;
        rec->point[0].y = y;
        rec->look = 5;
        random_spin = 1;
        break;
    case 19:
        rec->size = 8.0f;
        rec->point[0].y = y - 30.0f;
        rec->look = 15;
        random_spin = 1;
        break;
    case 20:
        rec->size = 2.4f;
        rec->point[0].y = y - 27.0f;
        rec->look = 0;
        break;
    case 21:
        rec->size = 24.0f;
        rec->point[0].y = y - 10.0f;
        rec->look = 4;
        spin = 90;
        break;
    case 22:
        rec->size = 4.0f;
        rec->point[0].y = master_clip_height(self) + y;
        rec->look = 16;
        break;
    case 30:
        rec->size = 24.0f;
        rec->point[0].y = y - 30.0f;
        rec->look = 19;
        break;
    case 31:
        rec->size = 2.0f;
        rec->point[0].y = y - 30.0f;
        rec->look = 0;
        break;
    case 32:
        rec->size = 8.0f;
        rec->point[0].y = y - 30.0f;
        rec->look = 32;
        random_spin = 1;
        break;
    default:
        rec->size = 5.0f;
        rec->point[0].y = y - 10.0f;
        rec->look = 2;
        break;
    }
    rec->shape = shape;
    if (random_spin)
        spin = rand_below(360);
    rec->spin = (short)spin;
    rec->pulse = 0.0f;
    rec->alpha = 0;
    rec->point[0].x = self->base.pos.x;
    rec->point[0].z = self->base.pos.z;
    rec->angle = (short)rand_below(360);
    rec->texture = 0;
    rec->style = style == 4 ? 1 : (signed char)style;
    rec->height = self->base.pos.y - rec->point[0].y;
}

void rag_heart_rings(struct RagEffect *self)
{
    static const float heart[20][2] = {
        {0.0f, 9.990001f}, {-3.5100002f, 11.88f}, {-7.8300004f, 12.960001f}, {-12.42f, 11.34f},
        {-15.120001f, 7.0200005f}, {-15.39f, 1.6200001f}, {-13.77f, -3.2400002f}, {-10.8f, -7.0200005f},
        {-6.7500005f, -10.26f}, {-2.7f, -12.690001f}, {0.0f, -16.2f}, {2.97f, -12.690001f},
        {7.0200005f, -10.26f}, {11.070001f, -7.0200005f}, {14.040001f, -3.2400002f}, {15.660001f, 1.6200001f},
        {15.39f, 7.0200005f}, {12.690001f, 11.34f}, {8.1f, 12.960001f}, {3.7800002f, 11.88f},
    };
    int i;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
    for (i = 0; i < 20; i++)
        rag_two_rings(self, "effect\\ring_red.tga", heart[i][0], heart[i][1]);
}
