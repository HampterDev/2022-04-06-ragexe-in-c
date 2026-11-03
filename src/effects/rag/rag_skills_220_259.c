#include "contents/sound.h"
#include "session/state.h"
#include "world/actor_lookup.h"
#include "game/context.h"
#include "effects/rag/rag_prim_setup.h"

unsigned char rag_potion_berserk(struct RagEffect *self)
{
    if (self->frame == 0) {
        render_object_send(self->master, NULL, 0x7f, 250, 120, 120, 0);
        rag_potion_pillar(self, 1.0f, 50);
    }
    if (self->frame == 18) {
        game_send_message(GAME_MSG_QUAKE, 1, 0, 0, 0);
        sound_play_wave("effect\\ac_concentration.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
    }
    rag_follow_master(self);
    if ((unsigned)(self->frame - 13) <= 96 && self->frame % 4 == 0) {
        struct EffectPrim *p = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
        if (!p)
            return 0;

        self->last_prim = p;
        p->duration = 40;
        mat_set_rotate_y(p->matrix, deg_to_rad(rand_below(360)));
        mat_apply(p->matrix, 0.0f, 0.0f, (float)(rand_below(7) + 2), &p->delta2);
        mat_set_rotate_x(p->matrix, deg_to_rad(90));
        p->speed = (float)(rand_below(50) + 20) / 60.0f;
        p->size[1].value = (float)(rand_below(60) + 30) / 10.0f;
        p->size[2].value = 0.2f;
        p->r = 240;
        p->g = 0;
        p->b = 0;
        p->alpha = 0.0f;
        p->alpha_max = 180.0f;
        p->alpha_speed = p->alpha_max / 20.0f;
        p->fade_frame = p->duration - 20;
        prim_set_texture(p, "effect\\ac_center2.tga");
        p->variant = 0;
    }
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
    if (self->frame > 150) {
        render_object_send(self->master, NULL, 0x80, 0, 0, 0, 0);
        return 0;
    }
    return 1;
}

void rag_bottom_ring(struct RagEffect *self, const char *texture, char scale)
{
    struct EffectPrim *p;
    struct PrimRing *ring;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x24, texture);
    if (!p)
        return;
    p->scale = (float)scale;
    ring = p->records.rings;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        ring[i].active = 0;
    ring->active = 1;
    ring->age = 0;
    ring->span = 360;
    ring->height = 15.0f;
    ring->turn = (short)rand_below(360);
    ring->alpha = (short)rand_below(100);
    ring->radius = 2.6f;
    ring->tilt = 80;
}

void rag_ground_disc(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    struct PrimTexRecord *rec;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x27, texture);
    if (!p)
        return;
    rec = &p->records.tex_records[0];
    rec->active = 1;
    p->records.tex_records[1].active = 0;
    p->records.tex_records[2].active = 0;
    p->records.tex_records[3].active = 0;
    if (style == 3) {
        rec->size = 10.0f;
        rec->angle = 225;
        rec->alpha = 250;
        p->scale = 111.0f;
    } else if (style == 4) {
        rec->size = 12.0f;
        rec->angle = 0;
        rec->alpha = 200;
        p->scale = 4.0f;
    } else if (style == 10 || style == 11 || style == 12) {
        rec->size = style == 10 ? 30.0f : style == 11 ? 28.0f : 20.0f;
        rec->angle = 0;
        rec->alpha = 200;
        p->scale = 10.0f;
    } else {
        rec->size = style == 2 ? 12.0f : 7.0f;
        rec->angle = 0;
        rec->alpha = 100;
        p->scale = 5.0f;
    }
    rec->bob = (short)rand_below(360);
    rec->y = -2.0f;
    rec->ground_style = (float)style;
    if (style == 10 || style == 11 || style == 12)
        rec->bob = 360;
}

void rag_defender(struct RagEffect *self, const char *texture)
{
    static const unsigned short turn[PRIM_RECORD_COUNT] = { 180, 270, 0, 90 };
    static const float height[PRIM_RECORD_COUNT] = { 40.0f, 39.0f, 38.0f, 37.0f };
    static const float radius[PRIM_RECORD_COUNT] = { 8.0f, 7.9f, 7.8f, 7.7f };
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x18, texture);
    if (!p)
        return;
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = 0;
        ring->span = 360;
        ring->height = height[i];
        ring->turn = turn[i];
        ring->alpha = 0;
        ring->radius = radius[i];
        ring->tilt = 90;
        ring->offset_flag = 0.0f;
    }
}

void rag_grand_cross_rings(struct RagEffect *self, unsigned short alpha,
                                      char black)
{
    static const unsigned short start[PRIM_RECORD_COUNT] = { 180, 90, 0, 270 };
    static const signed char centre[PRIM_RECORD_COUNT][2] = { { 23, 23 }, { 23, -23 }, { -23, -23 }, { -23, 23 } };
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\cru_grand cross.wav");
    p = rag_launch_prim(self, 0x22, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    if (!black) {
        prim_set_texture(p, "effect\\ring_white.tga");
        p->scale = 1.0f;
    } else {
        prim_set_texture(p, "effect\\ring_black.tga");
        p->scale = 12.0f;
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRingWall *w = &p->records.ring_walls[i];

        w->active = 1;
        w->age = 0;
        w->span = 90;
        w->full_height = 120.0f;
        w->alpha = alpha;
        w->radius = 19.9f;
        w->tilt = 90;
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRingWall *w = &p->records.ring_walls[i];

        w->start = start[i];
        w->offset_x = centre[i][0];
        w->offset_z = centre[i][1];
    }
}

void rag_grand_cross_beams(struct RagEffect *self, char black)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_prim(self, 0x25, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    prim_set_texture(p, black ? "effect\\ring_black.tga" : "effect\\ring_yellow.tga");
    p->scale = 1.0f;
    for (i = 0; i < 2; i++) {
        struct PrimBox *box = &p->records.boxes[i];

        box->active = 1;
        box->age = 0;
        box->full_height = 60.0f;
        box->angle = (short)(i * 90);
        box->alpha = 0;
        box->half_x = 24.0f;
        box->half_z = 3.2f;
        box->height = 0.0f;
    }
    p->records.boxes[2].active = 0;
    p->records.boxes[3].active = 0;
}

void rag_ground_rings(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x1f, texture);
    if (!p)
        return;
    p->scale = 0.0f;
    if (style == 3)
        p->scale = 17.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = 0;
        ring->span = 360;
        if (style == 2) {
            ring->height = 15.0f;
            ring->tilt = 70;
        } else {
            ring->height = 25.0f;
            ring->tilt = 80;
        }
        ring->turn = (short)(i * 90);
        ring->alpha = 0;
        ring->radius = (float)i + 1.0f;
        ring->mode = (short)style;
        ring->offset_flag = 0.0f;
    }
}

void rag_providence(struct RagEffect *self, const char *texture, char no_rings)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x29, texture);
    if (!p)
        return;
    p->scale = 5.0f;
    if (no_rings)
        return;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = 200;
        ring->span = 360;
        ring->height = 120.0f;
        ring->turn = (short)(i * 90);
        ring->alpha = 40;
        ring->radius = (float)i * 0.5f + 6.0f;
        ring->tilt = (short)(83 - i);
        ring->mode = 1;
        ring->offset_flag = 0.0f;
    }
}

void rag_map_pillar(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    p = rag_launch_textured(self, 0x29, texture);
    if (!p)
        return;
    if (style <= 1)
        p->scale = 4.0f;
    else if (style == 2)
        p->scale = 7.0f;
    else if (style == 3)
        p->scale = 6.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = i * 30;
        ring->span = 360;
        ring->height = 120.0f;
        ring->turn = (short)(i * 90);
        ring->alpha = 50;
        ring->offset_flag = 0.0f;
        if (style != 0 && style <= 1) {
            ring->alpha = 70;
            ring->radius = (float)i * 0.5f + 11.0f;
        } else {
            ring->alpha = 50;
            ring->radius = (float)i * 0.5f + 2.0f;
        }
        ring->mode = 0;
        ring->tilt = 89;
    }
}

void rag_heal_bubbles(struct RagEffect *self, unsigned char with_pok3, float size, float rise)
{
    struct EffectPrim *p = rag_launch_prim(self, 0x3b, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    int i;

    self->last_prim = p;
    p->list_flags = 5;
    p->duration = self->duration;
    if (with_pok3)
        p->motion_count = 2;
    prim_new_textures(p);
    p->textures[0] = rag_texture("effect\\pok1.tga");
    if (with_pok3)
        p->textures[1] = rag_texture("effect\\pok3.tga");
    p->scale = 0.0f;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &p->records.billboards[i];

        bb->active = 1;
        bb->age = 0;
        bb->size = (float)rand_below(10) * 0.1f + size;
        bb->phase = 0.0f;
        bb->alpha = 0;
        bb->pos.x = (float)rand_below(13) + self->base.pos.x - 6.0f;
        bb->pos.z = (float)rand_below(13) + self->base.pos.z - 6.0f;
        bb->pos.y = self->base.pos.y;
        bb->heading = 0;
        bb->speed = 0.0f;
        bb->climb = (float)rand_below(40) * 0.01f + rise;
        bb->spin = 0;
        bb->texture = 0;
        bb->state = with_pok3;
        bb->color_style = 11;
        bb->style = 0.0f;
    }
}

void rag_heal_rings(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    struct PrimRing *ring;

    rag_follow_master(self);
    if (self->frame == 0) {
        unsigned short fade_age = style ? 40 : 60;
        float outer = style ? 18.0f : 12.0f;

        p = rag_launch_textured(self, 0x3c, texture);
        if (!p)
            return;
        p->scale = style == 2 ? 4.0f : 9.0f;
        ring = p->records.rings;
        ring[0].active = 1;
        ring[0].age = 0;
        ring[0].span = 360;
        ring[0].height = 60.0f;
        ring[0].turn = 0;
        ring[0].alpha = 0;
        ring[0].radius = 4.6f;
        ring[0].tilt = 90;
        ring[0].offset_flag = 0.0f;
        ring[0].mode = fade_age;

        ring[1].active = 1;
        ring[1].age = 0;
        ring[1].span = 360;
        ring[1].height = 60.0f;
        ring[1].turn = 180;
        ring[1].alpha = 0;
        ring[1].radius = 5.0f;
        ring[1].tilt = 90;
        ring[1].offset_flag = 0.0f;
        ring[1].mode = fade_age;

        ring[2].active = 1;
        ring[2].age = 0;
        ring[2].span = 360;
        ring[2].height = outer;
        ring[2].turn = (short)rand_below(360);
        ring[2].alpha = 0;
        ring[2].radius = 5.0f;
        ring[2].tilt = 50;
        ring[2].offset_flag = 0.0f;
        ring[2].mode = fade_age;

        ring[3].active = 1;
        ring[3].age = 0;
        ring[3].span = 360;
        ring[3].height = outer;
        ring[3].turn = (short)rand_below(360);
        ring[3].alpha = 0;
        ring[3].radius = 5.2f;
        ring[3].tilt = 48;
        ring[3].offset_flag = 0.0f;
        ring[3].mode = fade_age;

        if (style < 2)
            rag_play_wave_here(self, "_heal_effect.wav");
    } else if (self->frame <= 50 && self->frame % 6 == 0 && style < 2) {
        rag_heal_bubbles(self, 1, 2.0f, style == 0 ? 0.4f : 0.6f);
    }
    if (style != 1)
        return;
    if (self->frame == 1) {
        p = rag_launch_textured(self, 0x3c, "effect\\alpha_down.tga");
        if (!p)
            return;
        p->scale = 9.0f;
        ring = p->records.rings;
        ring[0].active = 1;
        ring[0].age = 0;
        ring[0].span = 360;
        ring[0].height = 90.0f;
        ring[0].turn = 0;
        ring[0].alpha = 0;
        ring[0].radius = 4.2f;
        ring[0].tilt = 90;
        ring[0].mode = 40;
        ring[0].offset_flag = 0.0f;

        ring[1].active = 1;
        ring[1].age = 0;
        ring[1].span = 360;
        ring[1].height = 90.0f;
        ring[1].turn = 90;
        ring[1].alpha = 0;
        ring[1].radius = 4.3f;
        ring[1].tilt = 90;
        ring[1].mode = 40;
        ring[1].offset_flag = 0.0f;

        ring[2].active = 1;
        ring[2].age = 0;
        ring[2].span = 360;
        ring[2].height = 10.0f;
        ring[2].turn = (short)rand_below(360);
        ring[2].alpha = 0;
        ring[2].radius = 5.3f;
        ring[2].tilt = 46;
        ring[2].mode = 30;
        ring[2].offset_flag = 0.0f;

        ring[3].active = 1;
        ring[3].age = 0;
        ring[3].span = 360;
        ring[3].height = 10.0f;
        ring[3].turn = (short)rand_below(360);
        ring[3].alpha = 0;
        ring[3].radius = 5.4f;
        ring[3].tilt = 44;
        ring[3].mode = 30;
        ring[3].offset_flag = 0.0f;
    }
    if (self->frame <= 50 && self->frame % 9 == 2)
        rag_heal_bubbles(self, 1, 2.4f, 0.4f);
}

void rag_wind(struct RagEffect *self)
{
    static const unsigned short span[PRIM_RECORD_COUNT] = { 90, 70, 50, 30 };
    static const float radius[PRIM_RECORD_COUNT] = { 3.0f, 3.2f, 3.4f, 3.6f };
    static const float column[PRIM_RECORD_COUNT] = { 3.0f, 2.5f, 2.0f, 1.5f };
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x3e, "effect\\cloud11.tga");
    if (!p)
        return;
    p->scale = 4.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &p->records.rings[i];

        ring->active = 1;
        ring->age = 0;
        ring->span = span[i];
        ring->height = 8.0f;
        ring->turn = (short)(i * 10);
        ring->alpha = 0;
        ring->radius = radius[i];
        ring->tilt = 90;
        prim_ring_fill_columns(ring, column[i], 0);
        ring->mode = 0;
    }
    rag_play_wave_here(self, "_heal_effect.wav");
}

void rag_intimidate(struct RagEffect *self, char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    p = rag_launch_textured(self, 0x2b, style == 4 ? "effect\\white03.bmp" : "effect\\white01.bmp");
    if (!p)
        return;
    self->base.pos.y -= 9.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimOrbitSquare *sq = &p->records.orbit_squares[i];

        sq->active = 1;
        sq->pos = self->offset;
        if (style == 2) {
            int r = rand_below(26);
            sq->age = -r + -r;
            sq->color = 2;
            sq->size = 2.5f;
        } else if (style == 3) {
            sq->age = -70 - rand_below(30);
            sq->color = (unsigned char)(rand() % 3 + 3);
            sq->size = 4.0f;
        } else if (style == 4) {
            sq->age = -30 - rand_below(20);
            sq->color = (unsigned char)(rand() % 3 + 3);
            sq->size = 3.0f;
        } else {
            sq->age = -70 - rand_below(30);
            sq->color = 0;
            sq->size = 3.5f;
        }
        sq->radius = 12.0f;
        sq->radius_speed = 1.5f;
        sq->heading = (short)rand_below(360);
        sq->orbit_tilt = (short)rand_below(360);
        sq->spin = (short)rand_below(360);
        sq->tilt = (float)rand_below(360);
        sq->alpha = 0;
    }
}

void rag_line_link(struct RagEffect *self, char style)
{
    struct EffectPrim *p;
    struct PrimTube *tube;
    const void *actor = NULL;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_textured(self, 0x26, "effect\\cloud11.tga");
    if (!p)
        return;
    tube = &p->records.tube;
    tube->active = 1;
    tube->age = 0;
    tube->alpha = 45;
    tube->radius = 0.4f;
    tube->style = style == 2 ? 4.0f : 0.0f;
    tube->start = self->base.pos;
    tube->target = self->offset.y;
    if (tube->target > 0.0f) {
        if (tube->target == (float)(unsigned)g_session1.ownAid)
            actor = game_current_player();
        else
            actor = game_find_actor((int)(unsigned)tube->target);
    }
    if (!actor)
        tube->active = 0;
}

void rag_shield_boomerang(struct RagEffect *self, const char *texture)
{
    struct EffectPrim *p;
    struct PrimBillboard *bb;
    float dz, dx;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\cru_shield boomerang.wav");
    p = rag_launch_textured(self, 0x2a, texture);
    if (!p)
        return;
    p->scale = 0.0f;
    p->delta = self->offset;
    bb = p->records.billboards;
    dz = self->offset.z - self->base.pos.z;
    dx = self->offset.x - self->base.pos.x;
    bb->active = 1;
    bb->age = 0;
    bb->range = float_sqrt(dz * dz + dx * dx);
    bb->size = 7.0f;
    bb->phase = 0.0f;
    bb->alpha = 255;
    bb->pos.x = self->base.pos.x;
    bb->pos.z = self->base.pos.z;
    bb->pos.y = self->base.pos.y - 8.0f;
    bb->heading = 0;
    bb->speed = 2.0f;
    bb->climb = bb->speed / bb->range * (self->offset.y - self->base.pos.y);
    bb->spin = 0;
    bb->texture = 0;
    bb->state = 0;
    bb->color_style = 0;
    bb[1].active = 1;
    bb[1].alpha = 0;
    bb[2].active = 1;
    bb[2].alpha = 0;
    bb[3].active = 1;
    bb[3].alpha = 0;
}

void rag_spirit_spheres(struct RagEffect *self, char style)
{
    int count, i, place, full;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    count = (int)self->offset.x;
    if (count > 5)
        count = 5;
    for (i = 0, place = 0, full = 0; i < count; i++, full += 360, place += 72) {
        struct EffectPrim *p = rag_launch_prim(self, 0x20, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        struct PrimGlowOrb *o;
        const char *texture;
        float n;

        self->last_prim = p;
        p->list_flags = 5;
        p->duration = self->duration;
        o = &p->records.glow_orb;
        o->active = 1;
        o->sway[0].angle = (float)rand_below(360);
        o->sway[0].target = (float)rand_below(360);
        o->sway[1].angle = (float)rand_below(360);
        o->sway[1].target = (float)rand_below(360);
        o->sway[2].angle = (float)rand_below(360);
        o->sway[2].target = (float)rand_below(360);
        if (style == 1) {
            o->spin = (float)place;
            o->height = -14.0f;
            o->size = 1.3f;
            o->close_orbit = 1.0f;
            o->style = 0;
        } else if (style == 3 || style == 4) {
            n = self->offset.x;
            if (n <= 5.0f)
                o->spin = (float)full / n;
            else
                o->spin = (float)place;
            o->height = -15.0f;
            o->size = 1.0f;
            n = self->offset.x;
            if (n > 5.0f && n - (float)i >= 6.0f)
                o->size = 2.0f;
            o->close_orbit = 0.0f;
            o->style = 1;
        } else {
            o->spin = (float)place;
            o->height = -15.0f;
            o->size = 2.0f;
            o->close_orbit = 0.0f;
            o->style = 0;
            n = self->offset.x;
            if (n > 10.0f) {
                o->size = 2.0f;
                o->style = (unsigned char)((n - (float)i >= 11.0f) + 20);
            } else if (n > 5.0f && n - (float)i >= 6.0f) {
                o->size = 2.0f;
                o->style = 20;
            }
            if (style >= 10) {
                o->spin += (float)(rand() % 360);
                if (o->spin >= 360.0f)
                    o->spin -= 360.0f;
                o->style = (unsigned char)style;
                o->size = 6.0f;
                p->motion_count = 1;
            }
        }
        o->shimmer = (float)rand_below(360);
        o->brightness = 0;
        o->frames = 0;
        prim_new_textures(p);
        switch (style) {
        case 4:  texture = "effect\\RL_richs_coin_B\\rcoin1.bmp"; break;
        case 10: texture = "effect\\fire-1.bmp"; break;
        case 11: texture = "effect\\lightning-1.bmp"; break;
        case 12: texture = "effect\\water-1.bmp"; break;
        case 13: texture = "effect\\stone-1.bmp"; break;
        default: texture = "effect\\thunder_center.bmp"; break;
        }
        p->textures[0] = rag_texture(texture);
    }
}

static __inline void flip_half(float *v)
{
    if (rand_below(2) == 0)
        *v *= -1.0f;
}

void rag_clouds(struct RagEffect *self, char style)
{
    struct EffectPrim *p;
    int i, pick;

    const struct RenderObject *player = (const struct RenderObject *)game_current_player();
    if (!player || (style == 3 && !game_current_map_attr()))
        return;
    self->base.pos = player->pos;
    if (!rag_restart_forever(self))
        return;
    p = rag_launch_prim(self, 0x21, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    if (style == 3) {
        p->motion_count = 3;
        prim_new_textures(p);
        pick = rand_below(3);
        p->textures[0] = rag_texture((char)pick == 0 ? "effect\\fog1.tga"
                                     : (char)pick == 1 ? "effect\\fog2.tga" : "effect\\fog3.tga");
    } else if (style == 9 || style == 15) {
        p->motion_count = 6;
        prim_new_textures(p);
        p->textures[0] = rag_texture("effect\\star01.bmp");
        p->textures[1] = rag_texture("effect\\star02.bmp");
        p->textures[2] = rag_texture("effect\\star03.bmp");
        p->textures[3] = rag_texture("effect\\star04.bmp");
        p->textures[4] = rag_texture("effect\\star05.bmp");
        p->textures[5] = rag_texture("effect\\star06.bmp");
    } else {
        p->motion_count = 3;
        prim_new_textures(p);
        pick = rand_below(3);
        p->textures[0] = rag_texture((char)pick == 0 ? "effect\\cloud4.tga"
                                     : (char)pick == 1 ? "effect\\cloud1.tga" : "effect\\cloud2.tga");
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &p->records.billboards[i];
        Vec3 *v = &bb->pos;

        bb->active = 1;
        bb->age = 0;
        bb->style = (float)style;
        bb->spin = 45;
        if (style == 0) {
            bb->height_gate = -152.0f;
            if (self->base.pos.y > bb->height_gate)
                bb->age = 300;
        }
        bb->alpha = 0;
        bb->texture = 0;
        bb->heading = (short)((unsigned short)rand_below(200) + 300);
        if (style == 0 || style == 2 || style == 5) {
            if (style == 0)
                v->y = (float)rand_below(10) - 125.0f;
            else if (style == 2)
                v->y = (float)rand_below(10) + 0.0f;
            else
                v->y = (float)rand_below(10) + 20.0f;
            v->x = (float)(rand_below(300) - 150);
            v->z = (float)(rand_below(300) - 150);
        } else if (style == 3) {
            struct C3dAttr *ground;
            int sink;

            v->x = (float)(rand_below(300) - 150);
            v->z = (float)(rand_below(300) - 150);
            ground = game_current_map_attr();
            if (!ground) {
                bb->active = 0;
                continue;
            }
            sink = rand_below(5);
            v->y = c3d_attr_get_height(ground, v->x + self->base.pos.x, v->z + self->base.pos.z)
                   - 20.0f - (float)sink;
        } else if (style == 9 || style == 15) {
            v->y = (float)rand_below(10) + 80.0f;
            v->x = (float)(rand_below(200) + 25);
            v->z = (float)(rand_below(200) + 25);
            if (strcmp(game_current_map_name(), "ecl_fild01.rsw") == 0) {
                v->x = (float)(rand_below(400) + 25);
                v->z = (float)(rand_below(400) + 25);
            }
            flip_half(&v->x);
            flip_half(&v->z);
        } else {
            v->y = (float)rand_below(10) + 40.0f;
            v->x = (float)(rand_below(200) + 25);
            flip_half(&v->x);
            v->z = (float)(rand_below(200) + 25);
            flip_half(&v->z);
        }
        v->x = self->base.pos.x + v->x;
        v->z = self->base.pos.z + v->z;
        bb->wander[0] = (float)rand_below(360);
        bb->wander[1] = (float)rand_below(360);
        bb->wander[2] = (float)rand_below(360);
        bb->wander[3] = (float)rand_below(360);
        if (style == 3)
            bb->size = (float)rand_below(10) + 35.0f;
        else
            bb->size = (float)rand_below(20) + 30.0f;
        bb->phase = (float)rand_below(360);
        switch (style) {
        case 3:  bb->color_style = 12; break;
        case 5:  bb->color_style = 14; break;
        case 7:  bb->color_style = 16; break;
        case 8:  bb->color_style = 17; break;
        case 9:  bb->color_style = 23; break;
        case 15: bb->color_style = 25; break;
        case 11: bb->color_style = 14; break;
        case 12: bb->color_style = 16; break;
        case 13: bb->color_style = 17; break;
        case 14: bb->color_style = 24; break;
        default: bb->color_style = 0; break;
        }
    }
}
