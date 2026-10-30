#include "render/texture_cache.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

static struct EffectPrim *launch(struct RagEffect *self, int kind)
{
    self->last_prim = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!self->last_prim)
        return NULL;
    return self->last_prim;
}

static void textured(struct RagEffect *self, struct EffectPrim *prim, const char *texture)
{
    prim->list_flags = 5;
    prim->duration = self->duration;
    prim->textures = rag_prim_new_textures(prim);
    prim->textures[0] = texture_cache_get(&g_textureCache, texture, 0, 0);
}

static float ground_distance_to_offset(const struct RagEffect *self)
{
    float dz = self->offset.z - self->base.pos.z;
    float dx = self->offset.x - self->base.pos.x;
    return float_sqrt(dz * dz + dx * dx);
}

void rag_swirl_ring(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    struct PrimRing *ring;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 50);
    if (!prim)
        return;
    textured(self, prim, texture);
    prim->scale = (float)(int)style;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        prim->records.rings[i].active = 0;
    ring = &prim->records.rings[0];
    ring->active = 1;
    ring->age = 0;
    ring->span = 315;
    ring->height = (float)rag_rand_mod(10) + 25.0f;
    ring->turn = (short)rag_rand_mod(360);
    ring->alpha = (short)(50 - (short)rag_rand_mod(10));
    ring->tilt = 90;
    ring->mode = (short)rag_rand_mod(360);
    ring->offset_flag = 0.0f;
}

static const char *song_note_texture(char style, const char *texture)
{
    static const char *const spells[8] = {
        "effect\\spell_01.bmp", "effect\\spell_02.bmp", "effect\\spell_03.bmp", "effect\\spell_04.bmp",
        "effect\\spell_05.bmp", "effect\\spell_06.bmp", "effect\\spell_07.bmp", "effect\\spell_08.bmp",
    };
    int r;

    if (style == 5) {
        r = rag_rand_mod(3);
        if (r == 0)
            return "effect\\레드젬스톤.bmp";
        if (r == 1)
            return "effect\\블루젬스톤.bmp";
        return "effect\\옐로우젬스톤.bmp";
    }
    if (style == 2)
        return rag_rand_mod(2) == 0 ? "effect\\melody_a.bmp" : "effect\\melody_b.bmp";
    if (style == 8) {
        r = rag_rand_mod(8);
        return spells[r <= 6 ? r : 7];
    }
    return texture;
}

void rag_song_notes(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    struct PrimBillboard *bb;
    float size;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 47);
    if (!prim)
        return;
    prim->list_flags = 5;
    self->last_prim->duration = self->duration;
    self->last_prim->textures = rag_prim_new_textures(self->last_prim);
    self->last_prim->textures[0] = texture_cache_get(&g_textureCache, song_note_texture(style, texture), 0, 0);
    prim = self->last_prim;
    prim->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        prim->records.billboards[i].active = 0;
    bb = &prim->records.billboards[0];
    bb->active = 1;
    bb->age = 0;
    if (style == 1)
        size = 5.0f;
    else if (style == 4)
        size = 15.0f;
    else if (style == 6)
        size = 8.0f;
    else if (style == 9)
        size = 1.5f;
    else
        size = 3.0f;
    bb->size = size;
    bb->phase = 0.0f;
    bb->alpha = 200;
    bb->pos.x = self->base.pos.x;
    bb->pos.z = self->base.pos.z;
    bb->local.y = self->base.pos.y - 6.0f;
    bb->heading = (short)rag_rand_mod(360);
    bb->spin = 45;
    bb->texture = 0;
    bb->color_style = 2;
    bb->style = 0.0f;

    switch (style) {
    case 4:
        bb->shift = 10.0f;
        break;
    case 2:
        for (i = 1; i < PRIM_RECORD_COUNT; i++) {
            prim->records.billboards[i].active = 1;
            prim->records.billboards[i].alpha = 0;
        }
        break;
    case 3:
        bb->color_style = style;
        break;
    case 5:
    case 6:
        bb->color_style = 4;
        break;
    case 7:
        bb->color_style = style;
        break;
    case 9:
        bb->color_style = style;
        bb->color_substyle = 6;
        break;
    }
}

void rag_song_marks(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    int i, count;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 49);
    if (!prim)
        return;
    textured(self, prim, texture);
    prim = self->last_prim;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        prim->records.billboards[i].active = 0;

    count = style != 1 ? 2 : 1;
    for (i = 0; i < count; i++) {
        struct PrimBillboard *bb = &self->last_prim->records.billboards[i];

        bb->active = 1;
        bb->age = rag_rand_mod(11);
        if (style == 0)
            bb->size = (float)rag_rand_mod(6) + 5.0f;
        else if (style == 1)
            bb->size = (float)rag_rand_mod(20) * 0.1f + 3.0f;
        bb->phase = 0.0f;
        bb->alpha = 0;
        bb->pos.x = self->base.pos.x;
        bb->pos.z = self->base.pos.z;
        if (style == 0)
            bb->local.y = self->base.pos.y;
        else if (style == 1)
            bb->local.y = self->base.pos.y - 5.0f;
        bb->spin = 45;
        bb->texture = 0;
        bb->color_style = 2;
        bb->style = 0.0f;
        bb->state = (unsigned char)style;
    }
}

void rag_song_rings(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 48);
    if (!prim)
        return;
    textured(self, prim, texture);
    self->last_prim->scale = 0.0f;

    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWall *wall = &self->last_prim->records.walls[i];

        if (style == 0) {
            wall->active = 1;
            wall->age = 0;
            wall->phase = (short)rag_rand_mod(360);
            wall->to.x = (float)(rag_rand_mod(40) - 20);
            wall->to.z = (float)(rag_rand_mod(40) - 20);
            wall->from.x = 0.0f;
            wall->from.z = 0.0f;
            wall->alpha = 15;
        } else if (style == 3 || style == 4) {
            wall->active = i < 2 ? 1 : 0;
            wall->age = 0;
            wall->phase = (short)rag_rand_mod(360);
            if (i < 1) {
                wall->to.x = 6.0f;
                wall->to.z = 0.0f;
                wall->from.x = -6.0f;
                wall->from.z = 0.0f;
            } else {
                wall->to.x = 0.0f;
                wall->to.z = -6.0f;
                wall->from.x = 0.0f;
                wall->from.z = 6.0f;
            }
            wall->alpha = 100;
            wall->style = (float)(int)style;
        } else if (style == 1 || style == 2) {
            wall->active = 1;
            wall->age = 0;
            wall->phase = (short)rag_rand_mod(360);
            if (i < 2) {
                wall->to.x = (float)rag_rand_mod(7) - 3.0f;
                wall->to.z = -3.0f;
                wall->from.x = -wall->to.x;
                wall->from.z = 3.0f;
            } else {
                wall->to.x = -3.0f;
                wall->to.z = (float)rag_rand_mod(7) - 3.0f;
                wall->from.x = 3.0f;
                wall->from.z = -wall->to.z;
            }
            wall->alpha = 50;
            wall->style = (float)(int)style;
        }
    }
}

void rag_four_rings(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    short style16 = (short)style;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 31);
    if (!prim)
        return;
    textured(self, prim, texture);
    self->last_prim->scale = 0.0f;
    self->last_prim->accel = 99.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *ring = &self->last_prim->records.rings[i];

        ring->active = 1;
        ring->age = 0;
        ring->span = 360;
        ring->height = 25.0f;
        ring->turn = (short)(i * 90);
        ring->alpha = 0;
        ring->radius = (float)i + 1.0f;
        ring->tilt = 80;
        ring->mode = style16;
        ring->offset_flag = 0.0f;
    }
}

void rag_hit_burst(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 44);
    if (!prim)
        return;
    textured(self, prim, texture);
    self->base.pos.y = self->base.pos.y - 9.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimUprightBar *bar = &self->last_prim->records.upright_bars[i];

        bar->active = 1;
        bar->age = -10 - rag_rand_mod(25);
        bar->length = -10.0f;
        bar->length_speed = 3.0f;
        bar->heading = (short)rag_rand_mod(360);
        bar->angle = (short)rag_rand_mod(360);
        bar->half_width = 0.7f;
        bar->alpha = 0;
        bar->color = (unsigned char)style;
        bar->shift = 24;
    }
}

void rag_hit_burst_wide(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    int i;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    prim = launch(self, 44);
    if (!prim)
        return;
    textured(self, prim, texture);
    self->base.pos.y = self->base.pos.y - 9.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimUprightBar *bar = &self->last_prim->records.upright_bars[i];

        bar->active = 1;
        bar->age = -20 - rag_rand_mod(10);
        bar->length = -10.0f;
        bar->length_speed = 2.0f;
        bar->heading = (short)rag_rand_mod(360);
        bar->angle = (short)rag_rand_mod(360);
        bar->half_width = 1.0f;
        bar->alpha = 0;
        bar->color = (unsigned char)style;
        bar->shift = 16;
    }
}

void rag_hit_flash(struct RagEffect *self)
{
    struct EffectPrim *prim;
    int i;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    prim = launch(self, 45);
    if (!prim)
        return;
    textured(self, prim, "effect\\thunder_center.bmp");
    self->last_prim->scale = 0.0f;
    self->last_prim->delta = self->offset;
    self->last_prim->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &self->last_prim->records.billboards[i];

        bb->active = 1;
        bb->age = rag_rand_mod(10) - 40;
        bb->size = (float)rag_rand_mod(16) * 0.05f + 0.1f;
        bb->phase = 0.0f;
        bb->alpha = 0;
        bb->pos.x = self->offset.x;
        bb->pos.z = self->offset.z;
        bb->pos.y = self->offset.y - 8.0f;
        bb->heading = 0;
        bb->speed = (float)rag_rand_mod(50) * 0.01f + 0.75f;
        bb->climb = (float)rag_rand_mod(200) * 0.01f - 1.0f;
        bb->spin = 0;
        bb->texture = 0;
        bb->state = 0;
        bb->color_style = 2;
        bb->style = 0.0f;
    }
}

void rag_finger_offensive(struct RagEffect *self)
{
    struct EffectPrim *prim;
    struct PrimBillboard *bb;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\mon_탄지신통.wav");
    prim = launch(self, 42);
    if (!prim)
        return;
    textured(self, prim, "effect\\blue_ivy.bmp");
    self->last_prim->scale = 0.0f;
    self->last_prim->delta = self->offset;
    bb = &self->last_prim->records.billboards[0];
    bb->active = 1;
    bb->age = -15;
    bb->range = ground_distance_to_offset(self);
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
    self->last_prim->records.billboards[1].active = 1;
    self->last_prim->records.billboards[1].alpha = 0;
    self->last_prim->records.billboards[2].active = 1;
    self->last_prim->records.billboards[2].alpha = 0;
    self->last_prim->records.billboards[3].active = 1;
    self->last_prim->records.billboards[3].alpha = 0;
}

static int quarter_of(unsigned short heading)
{
    if ((unsigned short)(heading - 180) <= 89)
        return 0;
    if ((unsigned short)(heading - 90) <= 89)
        return 1;
    if (heading <= 89)
        return 2;
    return 3;
}

void rag_directed_sprite(struct RagEffect *self, char style)
{
    struct EffectPrim *prim;
    struct PrimSpriteMover *mv;
    const char *spr = NULL, *act = NULL;
    unsigned short heading;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    if (style == 0)
        rag_play_wave_here(self, "effect\\mon_침투경.wav");
    self->last_prim = rag_launch_prim(self, 46, 0.0f, 0.0f, 0.0f);
    if (!self->last_prim)
        return;
    self->last_prim->duration = self->duration;
    mv = &self->last_prim->records.sprite_mover;
    mv->active = 1;
    mv->speed = 0.0f;
    mv->pos.x = self->offset.x;
    mv->pos.y = self->offset.y - 0.0f;
    mv->pos.z = self->offset.z;

    if (style == 0) {
        self->last_prim->alpha = 200.0f;
        render_object_load_spr_res(&self->last_prim->base, "이팩트\\침투경_.spr");
        render_object_load_act_res(&self->last_prim->base, "이팩트\\침투경_.act");
        self->last_prim->motion_period = 2;
        self->last_prim->scale = 2.5f;
    } else {
        if (style == 1) {
            spr = "이팩트\\wink.spr";
            act = "이팩트\\wink.act";
        } else if (style == 2) {
            spr = "이팩트\\fvoice.spr";
            act = "이팩트\\fvoice.act";
        } else if (style == 12) {
            spr = "이팩트\\헌드레드스피어.spr";
            act = "이팩트\\헌드레드스피어.act";
        } else if (style == 13) {
            spr = "이팩트\\윈드커터.spr";
            act = "이팩트\\윈드커터.act";
        }
        if (spr) {
            render_object_load_spr_res(&self->last_prim->base, spr);
            render_object_load_act_res(&self->last_prim->base, act);
            self->last_prim->motion_period = 6;
            self->last_prim->scale = 1.0f;
            self->last_prim->loop_motion = 0;
            self->last_prim->pattern |= 0x200;
        }
    }

    heading = rag_screen_heading(self);
    prim = self->last_prim;
    prim->list_flags |= 0x200;
    render_object_set_action_base(&prim->base, quarter_of(heading), 1);
    prim_sync_particles(self->last_prim);
}

void rag_coin_burst(struct RagEffect *self, const char *texture)
{
    struct EffectPrim *prim;
    int i;

    if (self->frame != 0)
        return;
    rag_follow_master(self);
    prim = launch(self, 43);
    if (!prim)
        return;
    textured(self, prim, texture);
    self->base.pos.y = self->base.pos.y - 12.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimOrbitSquare *sq = &self->last_prim->records.orbit_squares[i];

        sq->active = 1;
        sq->pos = self->offset;
        sq->age = -18 - rag_rand_mod(30);
        sq->radius = 12.0f;
        sq->radius_speed = 1.5f;
        sq->heading = (short)rag_rand_mod(360);
        sq->orbit_tilt = (short)rag_rand_mod(360);
        sq->spin = (short)rag_rand_mod(360);
        sq->tilt = (float)rag_rand_mod(360);
        sq->size = 3.5f;
        sq->alpha = 0;
        sq->color = 1;
    }
}

void rag_fireworks(struct RagEffect *self)
{
    struct EffectPrim *prim;
    int i;

    const struct RenderObject *player = (const struct RenderObject *)game_current_player();
    if (!player)
        return;
    self->base.pos = player->pos;
    if (self->frame != 0)
        return;
    prim = launch(self, 52);
    if (!prim)
        return;
    prim->list_flags = 5;
    self->last_prim->duration = self->duration;
    self->last_prim->motion_count = 3;
    self->last_prim->textures = rag_prim_new_textures(self->last_prim);
    self->last_prim->textures[0] = texture_cache_get(&g_textureCache, "effect\\pok1.tga", 0, 0);
    self->last_prim->textures[1] = texture_cache_get(&g_textureCache, "effect\\pok3.tga", 0, 0);
    self->last_prim->textures[2] = texture_cache_get(&g_textureCache, "effect\\pok2.tga", 0, 0);
    self->base.pos.x = self->base.pos.x - 15.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSpark *sp = &self->last_prim->records.sparks[i];

        sp->active = 1;
        sp->age = -300 - rag_rand_mod(50);
        sp->spin = (short)rag_rand_mod(360);
        sp->pos = self->base.pos;
        sp->trail_3 = self->base.pos;
        sp->trail_2 = self->base.pos;
        sp->trail_1 = self->base.pos;
        sp->alpha = 0;
        sp->size = 2.0f;
        sp->color = (unsigned char)rag_rand_mod(5);
        sp->state = 0;
        sp->flash = 0;
        sp->flash_shift = (signed char)(rag_rand_mod(4) * 20);
        sp->speed = (float)rag_rand_mod(100) * 0.004f + 0.4f;
        sp->launch = (short)rag_rand_mod(360);
        sp->heading = (short)rag_rand_mod(360);
    }
}

void rag_joke_or_scream(struct RagEffect *self, char which)
{
    void *mode;

    if (self->frame != 0)
        return;
    mode = client_current_mode();
    mode_send_message(mode, 0xa5, which, 0, 0,
                                                                                         0);
}

void rag_throw_item(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *prim;
    struct PrimBillboard *bb;
    int start, i;
    float step, distance;

    rag_follow_master(self);
    if (style == 10)
        start = 5;
    else
        start = style == 11 ? 10 : 0;
    if (self->frame != start)
        return;

    prim = launch(self, 53);
    if (!prim)
        return;
    textured(self, prim, texture);
    self->last_prim->scale = 0.0f;
    self->last_prim->delta = self->offset;
    bb = &self->last_prim->records.billboards[0];
    bb->active = 1;
    bb->age = 0;
    bb->range = ground_distance_to_offset(self);
    step = 2.0f;
    if (style == 2)
        bb->size = 2.0f;
    else if (style == 3)
        bb->size = 4.0f;
    else
        bb->size = 3.0f;
    bb->phase = 0.0f;
    bb->alpha = 255;
    bb->pos.x = self->base.pos.x;
    bb->pos.z = self->base.pos.z;
    bb->pos.y = 0.0f;
    bb->local.y = self->base.pos.y - 8.0f;
    bb->heading = 0;
    if (style != 2) {
        if (style == 3)
            step = 3.0f;
        else
            step = bb->range / 25.0f;
    }
    bb->speed = step;
    bb->climb = bb->speed / bb->range * (self->offset.y - self->base.pos.y);
    bb->spin = style == 3 ? 330 : 0;
    bb->texture = 0;
    bb->style = 0.0f;
    if (style == 0) {
        bb->color_style = 8;
    } else if (style == 1) {
        bb->color_style = 9;
        bb->color_substyle = (unsigned char)self->hit_count;
    } else if (style >= 10) {
        bb->color_style = 9;
        bb->color_substyle = 4;
    } else if (style == 2) {
        bb->color_style = 0;
    } else if (style == 3) {
        bb->color_style = 15;
    }

    step = bb->speed;
    distance = bb->range;
    for (i = 0; i < 100; i++) {
        if ((float)i * step > distance) {
            bb->state = (unsigned char)i;
            break;
        }
    }
    self->last_prim->records.billboards[1].active = 1;
    self->last_prim->records.billboards[1].alpha = 0;
    self->last_prim->records.billboards[2].active = 1;
    self->last_prim->records.billboards[2].alpha = 0;
    self->last_prim->records.billboards[3].active = 1;
    self->last_prim->records.billboards[3].alpha = 0;
}
