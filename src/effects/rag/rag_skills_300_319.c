#include "contents/sound.h"
#include "session/state.h"
#include "effects/rag/rag_prim_setup.h"

void rag_bottom_column(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    struct PrimOpenBox *box;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_prim(self, 0x40, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    prim_set_texture(p, texture);
    p->scale = 1.0f;
    box = p->records.open_boxes;
    box->active = 1;
    box->age = 0;
    box->phase = (short)rand_below(360);
    if (style == 0)
        box->full_height = g_session1.map_rule_flags[0] ? 10.0f : 50.0f;
    else if (style == 1 || style == 3 || style == 4)
        box->full_height = 16.0f;
    else if (style == 2)
        box->full_height = 32.0f;
    box->angle = 0;
    if (style == 0) {
        box->alpha = 30;
        box->fade_age = 864;
        box->half_x = 5.0f;
        box->half_z = 5.0f;
    } else {
        if (style == 1 || style == 4) {
            box->alpha = 120;
            box->fade_age = 1880;
        } else if (style == 2) {
            box->alpha = 30;
            box->fade_age = 29999;
        }
        box->half_x = 2.5f;
        box->half_z = 2.5f;
    }
    box->height = 0.0f;
    box->style = (float)style;
    if (style == 4)
        box->blue = (float)rand_below(50);
    box->blue_add = 0.0f;
    box->blue_falling = 0.0f;
    box[1].active = 0;
    box[2].active = 0;
    box[3].active = 0;
}

void rag_chemical_protection(struct RagEffect *self, int turn)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x36, "effect\\shockwave_c.bmp");
    if (!p)
        return;
    self->base.pos.y -= 9.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimStripe *st = &p->records.stripes[i];

        st->active = 1;
        st->age = 0;
        st->length = 100.0f;
        st->heading = (short)(i * 90 + turn);
        st->alpha = 0;
    }
}

void rag_chemical_slash(struct RagEffect *self, short side)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 40)
        return;
    p = rag_launch_prim(self, 0x37, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags |= 0x200;
    p->duration = self->duration;
    prim_set_texture(p, "effect\\slash01.tga");
    self->base.pos.y -= 9.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        p->records.stripes[i].active = 0;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimStripe *st = &p->records.stripes[i];

        st->active = 1;
        st->age = 0;
        st->length = 100.0f;
        st->half_width = 1.0f;
        if (side == 0)
            st->side_shift = -15.0f - (float)i * 2.5f;
        else
            st->side_shift = (float)i * 2.5f + 15.0f;
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z,
                             &st->heading);
        if (st->heading >= 360)
            st->heading -= 360;
        st->alpha = 0;
    }
}

void rag_exit_rings(struct RagEffect *self, const char *texture)
{
    static const float height[PRIM_RECORD_COUNT] = { 180.0f, 70.0f, 45.0f, 20.0f };
    static const unsigned short turn[PRIM_RECORD_COUNT] = { 270, 180, 90, 0 };
    static const float radius[PRIM_RECORD_COUNT] = { 1.8f, 2.0f, 2.2f, 2.4f };
    static const unsigned short tilt[PRIM_RECORD_COUNT] = { 90, 88, 86, 84 };
    static const unsigned short fade_age[PRIM_RECORD_COUNT] = { 58, 60, 60, 60 };
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_Teleportation.wav");
    p = rag_launch_textured(self, 0x3c, texture);
    if (!p)
        return;
    p->scale = 4.0f;
    for (i = PRIM_RECORD_COUNT - 1; i >= 0; i--) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = 0;
        ring->span = 360;
        ring->height = height[i];
        ring->turn = turn[i];
        ring->alpha = 0;
        ring->radius = radius[i];
        ring->tilt = tilt[i];
        ring->mode = fade_age[i];
        ring->offset_flag = 0.0f;
    }
}

unsigned char rag_glass_wall_textured(struct RagEffect *self, const char *texture)
{
    static const float height[3] = { 34.0f, 37.0f, 40.0f };
    static const float radius[3] = { 3.6f, 3.3f, 3.0f };
    static const unsigned short fade_age[3] = { 5500, 3080, 3080 };
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame == 0) {
        rag_play_wave_here(self, "effect\\EF_GlassWall.wav");
        p = rag_launch_textured(self, 0x3c, texture);
        if (!p)
            return 0;
        p->scale = 10.0f;
        for (i = 0; i < 3; i++) {
            struct PrimRing *ring = &p->records.rings[i];

            ring->active = 1;
            ring->age = 0;
            ring->span = 360;
            ring->height = height[i];
            ring->turn = (short)rand_below(360);
            ring->alpha = 0;
            ring->radius = radius[i];
            ring->tilt = 90;
            ring->mode = fade_age[i];
            ring->offset_flag = 0.0f;
        }
        p->records.rings[3].active = 0;
    }
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
    return self->frame != self->duration;
}

void rag_teleportation(struct RagEffect *self, const char *texture, short n)
{
    static const float height[PRIM_RECORD_COUNT] = { 100.0f, 70.0f, 40.0f, 15.0f };
    static const unsigned short turn[PRIM_RECORD_COUNT] = { 180, 270, 0, 90 };
    static const unsigned short alpha[PRIM_RECORD_COUNT] = { 100, 80, 60, 40 };
    static const float radius[PRIM_RECORD_COUNT] = { 1.5f, 3.0f, 4.0f, 5.0f };
    struct EffectPrim *p;
    float taller, narrower;
    int i;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    rag_play_wave_here(self, "effect\\EF_Teleportation.wav");
    p = rag_launch_textured(self, 0x39, texture);
    if (!p)
        return;
    p->scale = 4.0f;
    taller = (float)(n * 60);
    narrower = (float)(n * 2);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = 0;
        ring->span = 360;
        ring->height = taller + height[i];
        ring->turn = turn[i];
        ring->alpha = alpha[i];
        ring->radius = radius[i] - narrower;
        ring->tilt = (short)(90 - i);
        ring->offset_flag = 0.0f;
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->column_flag[0] = 0;
        ring->column_flag[1] = (unsigned char)(int)ring->height;
    }
}

void rag_ready_portal(struct RagEffect *self)
{
    struct EffectPrim *p;
    int i;

    if (self->frame % 14 == 0) {
        rag_follow_master(self);
        rag_play_wave_here(self, "effect\\EF_ReadyPortal.wav");
    }
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x3f, "effect\\ring_blue.tga");
    if (!p)
        return;
    p->scale = 5.0f;
    for (i = 0; i < 3; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = -10 * i;
        ring->span = 360;
        ring->height = 6.001f;
        ring->turn = (short)(25 * i);
        ring->alpha = 0;
        ring->radius = 0.0f;
        ring->tilt = (short)(2 + i);
        ring->mode = 100;
        ring->offset_flag = 0.0f;
        prim_ring_fill_columns(ring, 0.0f, 1);
    }
    p->records.rings[3].active = 0;
    p->records.rings[3].age = 0;
    p->records.rings[3].mode = 1;
}

void rag_portal_styled(struct RagEffect *self, char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame == 0) {
        static const float radius[3] = { 4.0f, 3.0f, 2.0f };
        if (style == 3) {
            p = rag_launch_textured(self, 0x3c, "effect\\ring_red.tga");
            if (!p)
                return;
            p->scale = 10.0f;
        } else {
            p = rag_launch_textured(self, 0x3c, "effect\\magic_violet.tga");
            if (!p)
                return;
            p->scale = 4.0f;
        }
        for (i = 0; i < 3; i++) {
            struct PrimRing *ring = &p->records.rings[i];

            ring->active = 1;
            ring->age = 0;
            ring->span = 360;
            ring->height = 50.0f;
            ring->turn = (short)rand_below(360);
            ring->alpha = 0;
            ring->radius = radius[i];
            ring->tilt = 90;
            ring->mode = 1400;
            ring->offset_flag = 0.0f;
            prim_ring_fill_columns(ring, 0.0f, 1);
        }
        p->records.rings[3].active = 0;
    }
    if (self->frame == 1) {
        rag_follow_master(self);
        if (style == 3) {
            p = rag_launch_textured(self, 0x3f, "effect\\ring_red.tga");
            if (!p)
                return;
            p->scale = 10.0f;
        } else {
            p = rag_launch_textured(self, 0x3f, "effect\\ring_blue.tga");
            if (!p)
                return;
            p->scale = 4.0f;
        }
        for (i = 0; i < 3; i++) {
            struct PrimRing *ring = &p->records.rings[i];

            ring->active = 1;
            ring->age = i == 0 ? 0 : style != 3 ? -10 * i : -20 * i;
            ring->span = 360;
            ring->height = 6.001f;
            ring->turn = (short)(25 * i);
            ring->alpha = 0;
            ring->radius = 0.0f;
            ring->tilt = (short)(2 + i);
            ring->mode = 3111;
            ring->offset_flag = 0.0f;
            prim_ring_fill_columns(ring, 0.0f, 1);
        }
        p->records.rings[3].active = 0;
        p->records.rings[3].age = 0;
        p->records.rings[3].mode = (short)style;
    }
    if (self->frame == 2) {
        static const float height[PRIM_RECORD_COUNT] = { 1.5f, 3.5f, 5.5f, 7.5f };
        static const float radius[PRIM_RECORD_COUNT] = { 9.0f, 7.5f, 6.0f, 4.5f };

        p = rag_launch_textured(self, 0x3e, "effect\\cloud11.tga");
        if (!p)
            return;
        p->scale = style == 3 ? 10.0f : 4.0f;
        for (i = 0; i < PRIM_RECORD_COUNT; i++) {
            struct PrimRing *ring = &p->records.rings[i];

            ring->active = 1;
            ring->age = 0;
            ring->span = 30;
            ring->height = height[i];
            ring->turn = (short)(90 * i);
            ring->alpha = 0;
            ring->radius = radius[i];
            ring->tilt = 90;
            ring->mode = 0;
            prim_ring_fill_columns(ring, 1.5f, 0);
        }
    }
}

void rag_forest_light(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    p = rag_launch_textured(self, 0x42, texture);
    if (!p)
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimPrism *pr = &p->records.prisms[i];
        int thin = style == 2 || style == 12 || style == 13;
        int wide = style == 1 || style == 11;

        pr->active = 1;
        pr->age = 0;
        if (thin)
            pr->alpha = 30;
        else if (style == 10)
            pr->alpha = 25;
        else if (style == 11)
            pr->alpha = 0;
        else
            pr->alpha = 40;
        if (i == 3) {
            pr->age = 180;
            pr->radius = wide ? 4.0f : thin ? 1.0f : 2.0f;
        } else if (wide) {
            pr->radius = (float)(i * 2) + 4.0f;
        } else if (thin) {
            pr->radius = (float)i * 0.5f + 1.0f;
        } else {
            pr->radius = (float)i + 2.0f;
        }
        pr->base_radius = pr->radius;
        pr->turn = (short)(i * 25);
        pr->base = self->base.pos;
        self->offset = self->base.pos;
        if (style == 10) {
            self->offset.y -= 60.0f;
            self->offset.z -= 15.0f;
            self->offset.x -= 15.0f;
        } else {
            self->offset.y -= 300.0f;
            self->offset.z -= 70.0f;
            self->offset.x -= 70.0f;
        }
        pr->top = self->offset;
        pr->color = (unsigned char)style;
    }
}

void rag_heal(struct RagEffect *self, const char *texture, char style)
{
    int last = style ? 25 : 50;
    struct EffectPrim *p;
    struct PrimRing *ring;

    rag_follow_master(self);
    if (self->frame != 0) {
        if (self->frame > last || self->frame % 6 != 0)
            return;
        if (style == 0)
            rag_heal_bubbles(self, 0, 2.0f, 0.3f);
        else if (style == 1)
            rag_heal_bubbles(self, 1, 2.0f, 0.4f);
        return;
    }
    p = rag_launch_textured(self, 0x3c, texture);
    if (!p)
        return;
    if (style == 0)
        p->scale = 9.0f;
    else if (style == 1)
        p->scale = 5.0f;
    ring = p->records.rings;
    ring[0].active = 1;
    ring[0].age = 0;
    ring[0].span = 360;
    ring[0].turn = (short)rand_below(360);
    ring[0].alpha = 0;
    ring[0].tilt = 90;
    ring[0].offset_flag = 0.0f;
    if (style == 0) {
        ring[0].mode = 60;
        ring[0].height = 40.0f;
        ring[0].radius = 4.6f;
    } else if (style == 1) {
        ring[0].mode = 30;
        ring[0].height = (float)rand_below(20) + 40.0f;
        ring[0].radius = 4.5f;
        ring[0].radius = (float)rand_below(100) * 0.005f + ring[0].radius;
    }

    ring[1].active = 1;
    ring[1].age = 0;
    ring[1].span = 360;
    ring[1].turn = (short)rand_below(360);
    ring[1].alpha = 0;
    ring[1].tilt = 90;
    ring[1].offset_flag = 0.0f;
    if (style == 0) {
        ring[1].mode = 60;
        ring[1].height = 40.0f;
        ring[1].radius = 4.8f;
    } else if (style == 1) {
        ring[1].mode = 30;
        ring[1].height = ring[0].height;
        ring[1].radius = ring[0].radius + 0.2f;
    }
    ring[2].active = 0;
    ring[3].active = 0;
    rag_play_wave_here(self, "_heal_effect.wav");
}

void rag_first_aid(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimBillboard *bb;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave(self, "_heal_effect.wav", 150.0f);
    p = rag_launch_textured(self, 0x3a, "effect\\pikapika2.bmp");
    if (!p)
        return;
    p->scale = 0.0f;
    bb = p->records.billboards;
    bb->active = 1;
    bb->age = 0;
    bb->size = 10.0f;
    bb->phase = 0.0f;
    bb->alpha = 0;
    bb->pos.x = self->base.pos.x;
    bb->pos.z = self->base.pos.z;
    bb->pos.y = self->base.pos.y - 12.0f;
    bb->height_gate = 0.02f;
    bb->spin = (short)rand_below(360);
    bb->texture = 0;
    bb->color_style = 1;
    bb->phase = (float)rand_below(360);
    for (i = 1; i < PRIM_RECORD_COUNT; i++) {
        bb[i].active = 0;
        bb[i].alpha = 0;
    }
}

void rag_sprinkle_sand(struct RagEffect *self)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x3b, "effect\\thunder_center.bmp");
    if (!p)
        return;
    p->scale = 0.0f;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &p->records.billboards[i];

        bb->active = 1;
        bb->age = rand_below(10) - 10;
        bb->size = (float)rand_below(16) * 0.03f + 0.1f;
        bb->phase = 0.0f;
        bb->alpha = 0;
        bb->pos.x = (float)rand_below(9) + self->base.pos.x - 4.0f;
        bb->pos.z = (float)rand_below(9) + self->base.pos.z - 4.0f;
        bb->pos.y = self->base.pos.y;
        bb->heading = 0;
        bb->speed = (float)rand_below(10) * 0.01f;
        bb->climb = (float)rand_below(40) * 0.01f + 0.2f;
        bb->spin = 0;
        bb->texture = 0;
        bb->state = 0;
        bb->color_style = 10;
        bb->style = 0.0f;
    }
}

void rag_demonstration(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimSpriteMover *mv;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    p = rag_launch_prim(self, 0x33, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = self->duration;
    mv = &p->records.sprite_mover;
    mv->active = 1;
    mv->age = 0;
    mv->speed = 0.0f;
    p->scale = 1.2f;
    mv->pos = self->base.pos;
    mv->pos.y -= 1.0f;
    mv->follow_master = 0;
    p->alpha = 200.0f;
    render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "데몬스트레이션.spr");
    render_object_load_act_res(&p->base, RAG_SPRITE_DIR "데몬스트레이션.act");
    p->motion_period = 4;
    render_object_set_action_base(&p->base, 0, 0);
    prim_sync_particles(p);
}

void rag_firework_sound(struct RagEffect *self)
{
    if (self->frame == 300)
        sound_play_wave("effect\\폭죽.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
    if (self->frame > 2300)
        self->frame = 299;
}
