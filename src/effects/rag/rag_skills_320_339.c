#include "effects/rag/rag_prim_setup.h"

static void launch_blade_fans(struct RagEffect *self, const char *texture, float scale, int turn)
{
    struct EffectPrim *p = rag_launch_textured(self, 0x4b, texture);
    if (!p)
        return;
    int i;

    p->scale = scale;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBladeFan *fan = &p->records.blade_fans[i];

        fan->active = 1;
        fan->age = 0;
        fan->height = 2.0f;
        fan->turn = (short)(i * 90 + turn);
        fan->alpha = 0;
        fan->length = 2.0f;
        fan->inner = 1.0f;
        fan->style = 0;
    }
}

unsigned char rag_job_level_up50(struct RagEffect *self, int turn)
{
    rag_follow_master(self);
    if (self->frame == 5)
        launch_blade_fans(self, "effect\\magic_green.tga", 9.0f, turn);
    if (turn == 0 && rag_str_step(self, 0.0f, -80.0f, 0.0f) == 0)
        return 0;
    return 1;
}

static void warp_ring(struct PrimRing *ring, short turn, float radius, short tilt)
{
    ring->active = 1;
    ring->age = 0;
    ring->span = 360;
    ring->turn = turn;
    ring->alpha = 0;
    ring->radius = radius;
    ring->height = ring->radius;
    ring->tilt = tilt;
    ring->offset_flag = 0.0f;
}

void rag_warp_zone_styled(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    struct PrimRing *ring;

    if (self->frame == RAG_FOREVER - 1)
        self->frame = 0;
    if (self->frame == 1) {
        rag_follow_master(self);
        p = rag_launch_textured(self, 0x41, texture);
        if (!p)
            return;
        if (style == 0)
            p->scale = 11.0f;
        else if (style == 1)
            p->scale = 18.0f;
        ring = p->records.rings;
        warp_ring(&ring[0], 270, 2.5f, 53);
        warp_ring(&ring[1], 0, 5.0f, 60);
        warp_ring(&ring[2], 90, 7.5f, 55);
        warp_ring(&ring[3], 180, 10.0f, 50);
    }
    if (self->frame == 2) {
        rag_follow_master(self);
        p = rag_launch_textured(self, 0x41, texture);
        if (!p)
            return;
        if (style == 0)
            p->scale = 4.0f;
        else if (style == 1)
            p->scale = 18.0f;
        ring = p->records.rings;
        warp_ring(&ring[0], 271, 2.7f, 52);
        warp_ring(&ring[1], 1, 5.2f, 59);
        warp_ring(&ring[2], 91, 7.7f, 54);
        warp_ring(&ring[3], 181, 10.2f, 49);
    }
    if (self->frame == 0) {
        p = rag_launch_prim(self, 0x0c, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        self->last_prim = p;
        p->duration = RAG_FOREVER;
        p->pattern |= 1;
        p->size[0].value = 15.0f;
        p->delta2.y -= 1.5f;
        p->angle[1] = 90.0f;
        p->alpha = 0.0f;
        p->alpha_max = 128.0f;
        p->alpha_speed = p->alpha_max / 10.0f;
        p->fade_frame = p->duration - 10;
        prim_sync_particles(p);
        prim_set_texture(p, "effect\\alpha_down.tga");
    }
    if (self->frame % 10 == 0) {
        p = rag_launch_prim(self, 7, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        self->last_prim = p;
        p->size[0].value = (float)(rand_below(200) / 100) * 0.75f + 4.5f;
        p->size[0].speed = 0.0005f;
        p->duration = 70;
        p->angle[0] = (float)rand_below(360);
        if (rand_below(2) == 0)
            p->angle_speed[0] = 2.5f - (float)rand_below(11) * 0.1f;
        else
            p->angle_speed[0] = (float)rand_below(11) * 0.1f - 2.5f;
        p->fall_speed = -0.3f;
        p->gravity = p->fall_speed / (float)p->duration / -1.5f;
        prim_matrix_yaw(p, p->angle[0]);
        mat_apply(p->matrix, 0.0f, 0.0f, p->size[0].value, &p->delta2);
        p->scale = (float)(rand_below(10) / 10) + 0.7f;
        p->fade_frame = p->duration - p->duration / 3;
        render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "particle1.spr");
        render_object_load_act_res(&p->base, RAG_SPRITE_DIR "particle1.act");
        prim_sync_particles(p);
    }
}

void rag_magnum_break2(struct RagEffect *self, const char *texture)
{
    int k, i;

    rag_follow_master(self);
    if (self->frame == 0)
        self->id_state = 0;
    else if (self->frame == 20)
        rag_play_wave_here(self, "permeter_attack.wav");
    else if (self->frame == 30)
        rag_play_wave_here(self, "effect\\EF_MagnumBreak.wav");
    k = self->frame - 25;
    if ((unsigned)k <= 30 && self->frame % 3 == 0 && ++self->id_state < 11) {
        struct EffectPrim *p = rag_launch_prim(self, 0x49, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        float height = 20.0f - (float)k * 0.7f;
        float radius = (float)k * 0.2f + 5.0f;

        self->last_prim = p;
        p->list_flags = 5;
        p->duration = self->duration;
        p->motion_count = 1;
        prim_set_texture(p, texture);
        p->scale = 5.0f;
        for (i = 0; i < PRIM_RECORD_COUNT; i++)
            p->records.rings[i].active = 0;
        for (i = 0; i < 2; i++) {
            struct PrimRing *ring = &p->records.rings[i];

            ring->active = 1;
            ring->age = 0;
            ring->span = 360;
            ring->height = height;
            ring->turn = (short)(k * 6 + i * 9);
            ring->alpha = 0;
            ring->radius = radius;
            ring->tilt = (short)((35 - i * 4 - k) * 2);
            ring->mode = (short)i;
        }
    }
}

static void launch_asura(struct RagEffect *self, char style, const char *t0, const char *t1, const char *t2, int dir)
{
    struct EffectPrim *p = rag_launch_prim(self, 0x44, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    int i, j;

    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    p->motion_count = 3;
    prim_new_textures(p);
    p->textures[0] = rag_texture(t0);
    p->textures[1] = rag_texture(t1);
    p->textures[2] = rag_texture(t2);
    p->scale = 0.0f;
    for (i = 0; i < 3; i++) {
        struct PrimHalo *halo = &p->records.halos[i];

        halo->active = 1;
        halo->age = -40 - dir * 20 * i;
        halo->texture = (unsigned char)i;
        if (style == 0) {
            halo->size = 18.0f;
            halo->shift = (float)(dir * (6 + 12 * i));
            halo->style = 0;
        } else {
            halo->size = 24.0f;
            halo->shift = (float)(dir * (8 + 16 * i));
            halo->style = 3;
        }
        for (j = 0; j < HALO_LAYERS; j++)
            halo->layer_alpha[j] = 0.0f;
    }
}

void rag_begin_asura(struct RagEffect *self, char style)
{
    rag_follow_master(self);
    if (self->frame == 0) {
        if (self->duration < 350)
            self->duration = 350;
        rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
        rag_casting_circle(self, 45, "effect\\ring_white.tga", 2);
        rag_casting_circle(self, 25, "effect\\ring_white.tga", 2);
    }
    if (self->frame == 1) {
        if (style == 0)
            launch_asura(self, style, "effect\\asura3.tga", "effect\\asura2.tga", "effect\\asura1.tga", -1);
        else
            launch_asura(self, style, "effect\\asura13.tga", "effect\\asura12.tga", "effect\\asura11.tga", -1);
    }
    if (self->frame == 21) {
        if (style == 0)
            launch_asura(self, style, "effect\\asura4.tga", "effect\\asura5.tga", "effect\\asura6.tga", 1);
        else
            launch_asura(self, style, "effect\\asura14.tga", "effect\\asura15.tga", "effect\\asura16.tga", 1);
    }
}

void rag_footprint(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    struct PrimTexRecord *recs;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_prim(self, 0x43, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 1;
    p->duration = self->duration;
    prim_set_texture(p, texture);
    p->scale = style == 1 ? 100.0f : 0.0f;
    recs = p->records.tex_records;
    recs[0].active = 1;
    recs[1].active = 0;
    recs[2].active = 0;
    recs[3].active = 0;
    recs[0].age = 0;
    recs[0].size = style == 1 ? 6.0f : 5.0f;
    recs[0].bob = 0;
    recs[0].angle = (short)(int)self->offset.x;
    recs[0].alpha = 150;
    recs[0].y = -0.2f;
}

void rag_blind(struct RagEffect *self, char level)
{
    struct EffectPrim *p;
    struct PrimFramedPanel *panel;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    p = rag_launch_prim(self, 0x47, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags |= 0x200;
    p->duration = self->duration;
    p->motion_count = 2;
    prim_new_textures(p);
    p->textures[0] = rag_texture("effect\\fullb.tga");
    p->textures[1] = rag_texture("effect\\white02.bmp");
    self->base.pos.y -= 9.0f;
    panel = p->records.framed_panels;
    panel[1].active = 0;
    panel[2].active = 0;
    panel[3].active = 0;
    panel[0].active = 1;
    panel[0].age = 0;
    if (level == 0) {
        panel[0].size = 65.0f;
        panel[0].bright = 0;
    } else {
        panel[0].size = 140.0f - (float)(level * 10);
        panel[0].bright = 1;
    }
    panel[0].alpha = 0;
}

void rag_poison(struct RagEffect *self, char style)
{
    struct EffectPrim *p;
    struct PrimScrollGrid *grid;

    rag_follow_master(self);
    if (!rag_restart_forever(self))
        return;
    p = rag_launch_prim(self, 0x48, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags |= 0x200;
    p->duration = self->duration;
    p->motion_count = 1;
    prim_set_texture(p, style ? "effect\\white02.bmp" : "effect\\poison_f.bmp");
    self->base.pos.y -= 9.0f;
    grid = p->records.scroll_grids;
    grid[1].active = 0;
    grid[2].active = 0;
    grid[3].active = 0;
    grid[0].active = 1;
    grid[0].age = 0;
    grid[0].cell = 30.0f;
    grid[0].scroll = 0.0f;
    grid[0].alpha = 0;
    grid[0].style = (unsigned char)style;
}

void rag_guard(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    struct PrimSlash *slash;

    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\kyrie_guard.wav");
    p = rag_launch_prim(self, 0x3d, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 2;
    prim_new_textures(p);
    p->textures[0] = rag_texture(texture);
    p->textures[1] = rag_texture("effect\\guardK2.tga");
    p->scale = 11.0f;
    slash = p->records.slashes;
    slash[3].active = 0;
    slash[2].active = 0;
    slash[1].active = 0;
    slash[0].active = 1;
    slash[0].age = 0;
    slash[0].radius = 7.8f;
    slash[0].alpha = 0;
    slash[0].width = 1.0f;
    slash[0].flare = 0.0f;
    slash[0].color = (style >= 2 && style <= 5) ? (unsigned char)style : 0;
}

void rag_hit_line(struct RagEffect *self, char style)
{
    struct EffectPrim *p;
    int i, j;

    rag_follow_master(self);
    if ((unsigned)self->frame > 3)
        return;
    p = rag_launch_prim(self, 0x46, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    p->motion_count = 1;
    prim_set_texture(p, "effect\\white02.bmp");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWhip *whip = &p->records.whips[i];

        whip->active = 1;
        whip->age = -rand_below(26);
        whip->thickness = 0.1f;
        whip->alpha = (short)(rand_below(51) + 125);
        whip->heading = (short)rand_below(360);
        whip->link_length = (float)rand_below(8) + 6.0f;
        whip->links[0] = 0;
        whip->bend[0] = 0.0f;
        whip->root = self->base.pos;
        for (j = WHIP_JOINTS - 1; j >= 1; j--) {
            whip->bend[j] = 0.0f;
            whip->links[j] = 0;
        }
        whip->mode = (short)style;
    }
}

void rag_time_bubbles(struct RagEffect *self, const char *texture, char style)
{
    struct EffectPrim *p;
    int i;

    rag_follow_master(self);
    if (self->frame == 0) {
        if (style == 0)
            rag_play_wave(self, "_heal_effect.wav", -40.0f);
        else if (style == 1)
            rag_play_wave(self, "effect\\흡기.wav", -80.0f);
    }
    if (self->frame > 20 || self->frame % 4 != 0)
        return;
    p = rag_launch_prim(self, 0x3b, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 2;
    prim_new_textures(p);
    if (style == 2) {
        p->textures[0] = rag_texture(texture);
        p->textures[1] = rag_texture(texture);
    } else {
        p->textures[0] = rag_texture("effect\\pok1.tga");
        p->textures[1] = rag_texture("effect\\pok3.tga");
    }
    p->scale = 0.0f;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &p->records.billboards[i];

        bb->active = 1;
        bb->age = 0;
        if (style == 2) {
            if (i >= 2)
                bb->active = 0;
            else
                bb->size = (float)rand_below(11) * 0.1f + 2.0f;
            bb->pos.x = (float)rand_below(19) + self->base.pos.x - 9.0f;
            bb->pos.z = (float)rand_below(19) + self->base.pos.z - 9.0f;
        } else {
            bb->size = (float)rand_below(10) * 0.1f + 1.5f;
            bb->pos.x = (float)rand_below(7) + self->base.pos.x - 3.0f;
            bb->pos.z = (float)rand_below(7) + self->base.pos.z - 3.0f;
        }
        bb->phase = 0.0f;
        bb->alpha = 0;
        bb->pos.y = self->base.pos.y - 6.0f;
        bb->heading = 0;
        bb->speed = 0.0f;
        bb->climb = (float)rand_below(20) * 0.01f + 0.2f;
        bb->spin = 0;
        bb->texture = 0;
        bb->state = 1;
        if (style == 0 || style == 2)
            bb->color_style = 11;
        else
            bb->color_style = style == 3 ? 18 : 1;
        bb->style = 0.0f;
    }
}

void rag_triple_attack(struct RagEffect *self)
{
    rag_follow_master(self);
    if (self->frame <= 1) {
        struct EffectPrim *p = rag_launch_textured(self, 0x45, "effect\\cloud11.tga");
        if (!p)
            return;
        float dx, dz;
        int i;

        p->scale = 0.0f;
        p->delta = self->offset;
        dx = self->offset.x - self->base.pos.x;
        dz = self->offset.z - self->base.pos.z;
        for (i = 0; i < PRIM_RECORD_COUNT; i++) {
            struct PrimPipe *pipe = &p->records.pipes[i];
            float back;

            pipe->active = 1;
            pipe->age = -rand_below(20);
            prim_heading_degrees(dx, dz, &pipe->heading);
            pipe->alpha = 0;
            pipe->start.x = (float)rand_below(100) * 0.06f - 3.0f + self->base.pos.x;
            pipe->start.z = (float)rand_below(100) * 0.06f - 3.0f + self->base.pos.z;
            pipe->start.y = self->base.pos.y;
            back = (float)rand_below(100) * 0.15f;
            pipe->start.x -= g_cosTable[pipe->heading] * back;
            pipe->start.z -= g_sinTable[pipe->heading] * back;
            pipe->end.x = pipe->start.x;
            pipe->end.z = pipe->start.z;
            pipe->end.y = self->base.pos.y;
            pipe->radius = 0.25f;
            pipe->drop = (float)rand_below(100) * 0.04f + 8.0f;
            pipe->length = (float)rand_below(100) * 0.2f + 20.0f;
            pipe->style = 0.0f;
        }
    }
    if (self->frame == 10)
        rag_play_wave_here(self, "effect\\EF_hit2.wav");
    if (self->frame == 20)
        rag_play_wave_here(self, "effect\\EF_hit4.wav");
    if (self->frame == 30)
        rag_play_wave_here(self, "effect\\EF_hit2.wav");
}

void rag_maple(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimSpriteMover *mv;

    rag_follow_master(self);
    rag_restart_forever(self);
    if ((self->frame & 1) != 0 || self->frame >= 200)
        return;
    p = rag_launch_prim(self, 0x28, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = self->duration;
    mv = &p->records.sprite_mover;
    mv->active = 1;
    mv->speed = (float)(rand_below(4) + 2) * 0.03f;
    p->scale = (float)(rand_below(6) + 35) / 100.0f;
    mv->pos.x = (float)rand_below(300) + self->base.pos.x - 150.0f;
    mv->sway[0][0] = (float)rand_below(360);
    mv->sway[0][1] = (float)rand_below(360);
    mv->sway[1][0] = (float)rand_below(360);
    mv->sway[1][1] = (float)rand_below(360);
    mv->pos.y = self->base.pos.y - 200.0f;
    mv->pos.z = (float)rand_below(300) + self->base.pos.z - 150.0f;
    mv->style = 1;
    p->alpha = 200.0f;
    render_object_load_act_res(&p->base, RAG_SPRITE_DIR "단풍.act");
    render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "단풍.spr");
    render_object_set_action_base(&p->base, 0, 0);
    prim_sync_particles(p);
}

void rag_angel(struct RagEffect *self, char style)
{
    rag_follow_master(self);
    if (self->frame == 0
        || (self->frame <= 18 && (style == 1 || style == 11) && self->frame % 3 == 0)) {
        struct EffectPrim *p = rag_launch_prim(self, 0x55, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        struct PrimSpriteMover *mv;

        self->last_prim = p;
        p->duration = self->duration;
        mv = &p->records.sprite_mover;
        mv->age = 0;
        mv->active = 1;
        p->scale = 1.0f;
        mv->centre.x = self->base.pos.x;
        mv->centre.y = self->base.pos.y - 40.0f;
        mv->centre.z = self->base.pos.z;
        mv->style = (signed char)style;
        mv->style2 = (signed char)self->frame;
        p->alpha = 0.0f;
        switch (style) {
        case 0:
            render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "천사.spr");
            render_object_load_act_res(&p->base, RAG_SPRITE_DIR "천사.act");
            p->list_flags = 5;
            break;
        case 1:
            render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "천사날개.spr");
            render_object_load_act_res(&p->base, RAG_SPRITE_DIR "천사날개.act");
            p->list_flags = 5;
            break;
        case 2:
            render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "깃털" "1.spr");
            render_object_load_act_res(&p->base, RAG_SPRITE_DIR "깃털" "1.act");
            p->list_flags = 5;
            break;
        case 3:
            render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "깃털" "2.spr");
            render_object_load_act_res(&p->base, RAG_SPRITE_DIR "깃털" "2.act");
            p->list_flags = 5;
            break;
        case 4:
            render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "깃털" "3.spr");
            render_object_load_act_res(&p->base, RAG_SPRITE_DIR "깃털" "3.act");
            p->list_flags = 5;
            break;
        case 10:
            render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "한복천사.spr");
            render_object_load_act_res(&p->base, RAG_SPRITE_DIR "한복천사.act");
            p->list_flags = 1;
            mv->centre.y = self->base.pos.y - 30.0f;
            p->list_flags |= 0x200;
            break;
        case 11:
            render_object_load_spr_res(&p->base, RAG_SPRITE_DIR "한복천사(날개).spr");
            render_object_load_act_res(&p->base, RAG_SPRITE_DIR "한복천사(날개).act");
            p->list_flags = 5;
            mv->centre.y = self->base.pos.y - 30.0f;
            break;
        }
        render_object_set_action_base(&p->base, 0, 1);
        prim_sync_particles(p);
        p->loop_motion = 1;
        p->base.action = 0;
    }
    if (style != 0 && style != 10)
        return;
    if (self->frame == 65)
        launch_blade_fans(self, "effect\\ring_blue.tga", 4.0f, 0);
    if (self->frame == 66)
        launch_blade_fans(self, "effect\\ring_blue.tga", 4.0f, 45);
}
