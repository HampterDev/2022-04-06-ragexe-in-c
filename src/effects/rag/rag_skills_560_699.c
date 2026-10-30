#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/actor_ops.h"
#include "core/object_type.h"

#define EFFECT_DIR "이팩트\\"
#define ITEM_DIR "유저인터페이스\\item\\"

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

static struct EffectPrim *launch_prim_list1(struct RagEffect *self, int kind)
{
    struct EffectPrim *p = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    self->last_prim = p;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
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

static void ring_fill_columns(struct PrimRing *ring, float column, unsigned char flag)
{
    int k;

    for (k = 0; k < PRIM_RING_COLUMNS; k++) {
        ring->column[k] = column;
        ring->column_flag[k] = flag;
    }
}

static void textures_from(struct EffectPrim *p, const char *const *names, int n)
{
    int i;

    for (i = 0; i < n; i++)
        p->textures[i] = texture(names[i]);
}

static void fire_ring(struct EffectPrim *p, int i, float radius, short fade_from)
{
    struct PrimRing *ring = &p->records.rings[i];

    ring->active = 1;
    ring->age = 0;
    ring->span = 360;
    ring->height = 80.0f;
    ring->turn = (short)rand_below(360);
    ring->alpha = 0;
    ring->radius = radius;
    ring->tilt = 90;
    ring->mode = fade_from;
    ring->offset_flag = 0.0f;
    ring_fill_columns(ring, 0.0f, 3);
}

static void fire_cloud(struct EffectPrim *p, int i, float height, short turn, float radius)
{
    struct PrimRing *ring = &p->records.rings[i];

    ring->active = 1;
    ring->age = 0;
    ring->span = 30;
    ring->height = height;
    ring->turn = turn;
    ring->alpha = 0;
    ring->radius = radius;
    ring->tilt = 90;
    ring->mode = 3;
    ring_fill_columns(ring, 1.5f, 0);
}

void rag_fire_change_rings(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    short fade_from = look == 1 ? -1 : 1400;

    if (self->frame == 0) {
        p = launch_prim(self, 60, 0);
        if (!p)
            return;
        if (look == 3) {
            p->textures[0] = texture("effect\\ring_red.tga");
            p->scale = 10.0f;
        } else {
            p->textures[0] = texture("effect\\magic_violet.tga");
            p->scale = 4.0f;
        }
        fire_ring(p, 0, 7.0f, fade_from);
        fire_ring(p, 1, 5.5f, fade_from);
        fire_ring(p, 2, 4.0f, fade_from);
        p->records.rings[3].active = 0;
    }
    if (self->frame == 2) {
        p = launch_prim(self, 62, 0);
        if (!p)
            return;
        p->textures[0] = texture("effect\\cloud11.tga");
        p->scale = look == 3 ? 10.0f : 4.0f;
        fire_cloud(p, 0, 2.5f, 0, 13.0f);
        fire_cloud(p, 1, 5.0f, 90, 11.0f);
        fire_cloud(p, 2, 7.5f, 180, 9.0f);
        fire_cloud(p, 3, 10.0f, 270, 8.0f);
    }
}

void rag_falling_items(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    struct PrimBillboard *bb;
    void *attr;
    int i, drop;

    const struct RenderObject *player = (const struct RenderObject *)game_current_player();
    if (!player || !game_current_map_attr())
        return;
    self->base.pos = player->pos;
    if (self->frame != 0)
        return;
    p = launch_prim(self, 123, 1);
    if (!p)
        return;
    if (look == 0) {
        p->textures[0] = texture(ITEM_DIR "하얀허브.bmp");
    } else if (look == 1) {
        p->textures[0] = texture(ITEM_DIR "화이트슬림포션.bmp");
    } else if (look == 2) {
        unsigned char pick = (unsigned char)rand_below(3);
        if (pick == 0)
            p->textures[0] = texture(ITEM_DIR "화염병.bmp");
        else if (pick == 1)
            p->textures[0] = texture(ITEM_DIR "염산병.bmp");
        else
            p->textures[0] = texture(ITEM_DIR "알코올.bmp");
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        bb = &p->records.billboards[i];
        bb->active = 1;
        bb->age = 0;
        bb->style = 3.0f;
        bb->spin = (short)rand_below(360);
        bb->alpha = 0;
        bb->texture = 0;
        bb->heading = (short)(self->duration - 50);
        bb->pos.x = (float)(rand_below(160) - 80);
        bb->pos.z = (float)(rand_below(160) - 80);
        attr = game_current_map_attr();
        if (!attr) {
            bb->active = 0;
            continue;
        }
        drop = rand_below(66);
        bb->pos.y = (float)((double)c3d_attr_get_height(attr, bb->pos.x + self->base.pos.x,
                                               bb->pos.z + self->base.pos.z) - 5.0 - (double)(float)drop);
        bb->pos.x += self->base.pos.x;
        bb->pos.z += self->base.pos.z;
        bb->wander[0] = (float)rand_below(360);
        bb->wander[1] = (float)rand_below(360);
        bb->wander[2] = (float)rand_below(360);
        bb->wander[3] = (float)rand_below(360);
        bb->size = (float)rand_below(11) * 0.1f + 3.0f;
        bb->phase = (float)rand_below(360);
        bb->color_style = 7;
    }
}

unsigned char rag_blue_magic_rings_now(struct RagEffect *self, int angle)
{
    struct EffectPrim *p;
    struct PrimBladeFan *fan;
    int i;

    follow_master(self);
    p = launch_prim(self, 75, 0);
    if (!p)
        return 0;
    p->textures[0] = texture("effect\\magic_blue.tga");
    p->scale = 4.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        fan = &p->records.blade_fans[i];
        fan->active = 1;
        fan->age = 0;
        fan->height = 1.5f;
        fan->turn = (short)(i * 90 + angle);
        fan->alpha = 0;
        fan->length = 2.0f;
        fan->inner = 1.0f;
        fan->style = 0;
    }
    return 1;
}

void rag_master_copies(struct RagEffect *self)
{
    struct World *world = game_current_world();
    struct RenderObject *copy;
    void *mem;

    if (!world || !self->master)
        return;
    self->base.pos.x = 155.0f;
    self->base.pos.z = 255.0f;
    self->base.pos.y = 155.0f;
    render_object_set_argb(self->master, -1, 155, 155, 255);
    if (self->frame == 9998)
        self->frame = 0;
    if (self->frame % 20 != 0)
        return;
    if (!object_as_type(self->master, OBJECT_PC))
        return;
    mem = mem_alloc(sizeof(struct BlurPC));
    copy = mem ? (struct RenderObject *)blur_pc_construct(mem, (struct Pc *)self->master) : NULL;
    if (!copy)
        return;
    render_object_send(copy, &self->base, 0, (RenderMsgArg)(int)self->base.pos.x,
                        (RenderMsgArg)(int)self->base.pos.y, (RenderMsgArg)(int)self->base.pos.z, 0);
    effect_add_world_object(world, copy);
}

void rag_sakura_petals(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimSpriteMover *mv;
    char wave[52];
    short a;

    follow_master(self);
    if (self->frame >= 12 && self->frame <= 15) {
        sprintf(wave, "effect\\EF_FireArrow%d.wav", rand_below(3) + 1);
        rag_play_wave_here(self, wave);
    }
    if (self->frame >= 20)
        return;
    p = rag_launch_prim(self, 124, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = self->duration;
    mv = &p->records.sprite_mover;
    mv->active = 1;
    mv->speed = (float)(rand_below(3) + 2) * 0.15f;
    p->scale = (float)(rand_below(6) + 22) / 70.0f;
    mv->pos.y = self->base.pos.y;
    a = (short)rand_below(360);
    mv->pos.x = g_cosTable[a] * 8.0f + self->base.pos.x;
    mv->pos.z = g_sinTable[a] * 8.0f + self->base.pos.z;
    mv->sway[0][0] = (float)rand_below(360);
    mv->sway[0][1] = (float)rand_below(360);
    mv->sway[1][0] = (float)rand_below(360);
    mv->sway[1][1] = (float)rand_below(360);
    p->alpha = 250.0f;
    render_object_load_spr_res(&p->base, EFFECT_DIR "sakura01.spr");
    render_object_load_act_res(&p->base, EFFECT_DIR "sakura01.act");
    render_object_set_action_base(&p->base, rand_below(3), 0);
    prim_sync_particles(p);
}

void rag_columns_wide(struct RagEffect *self, const char *tex, char look)
{
    struct EffectPrim *p;
    struct PrimFlatSquare *sq;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 125, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    p->delta = self->offset;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.flat_squares[i];
        sq->active = 1;
        sq->frame = 0;
        sq->size = (float)rand_below(11) * 0.5f + 10.0f;
        sq->alpha = 0;
        sq->pos.x = self->base.pos.x;
        sq->pos.z = self->base.pos.z;
        sq->pos.y = self->base.pos.y - 10.0f;
        sq->heading = 0;
        sq->yaw = (short)i;
        sq->speed = (float)rand_below(21) * 0.1f + 1.0f;
        sq->state = 0;
        if (look == 0)
            sq->turn_up = 0;
        else if (look == 1)
            sq->turn_up = 1;
        sq->style = 1;
    }
}

void rag_columns_shifted(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimFlatSquare *sq;
    int f, i;

    follow_master(self);
    if (self->frame >= 5)
        return;
    p = launch_prim(self, 126, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    p->delta = self->offset;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        sq = &p->records.flat_squares[i];
        sq->active = 1;
        sq->frame = i - 3;
        sq->size = 20.0f;
        sq->alpha = 0;
        f = self->frame;
        if (f == 0 || f == 1 || f == 2) {
            sq->pos.x = f == 1 ? self->base.pos.x + 5.0f : f == 2 ? self->base.pos.x - 5.0f : self->base.pos.x;
            sq->pos.z = self->base.pos.z;
        } else {
            sq->pos.x = self->base.pos.x;
            sq->pos.z = f == 3 ? self->base.pos.z + 5.0f : self->base.pos.z - 5.0f;
        }
        sq->pos.y = (float)rand_below(5) + (self->base.pos.y - 10.0f);
        sq->heading = (short)rand_below(360);
        sq->yaw = (short)i;
        sq->speed = 1.5f;
        sq->style = 1;
    }
}

static void aura_ring(struct EffectPrim *p, int i, float height, short turn, float radius, short tilt)
{
    struct PrimRing *ring = &p->records.rings[i];

    ring->active = 1;
    ring->age = 0;
    ring->span = 315;
    ring->height = height;
    ring->turn = turn;
    ring->alpha = 50;
    ring->mode = 1;
    ring->radius = radius;
    ring->tilt = tilt;
    ring->offset_flag = 0.0f;
}

void rag_aura_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim_list1(self, 23);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    aura_ring(p, 0, 15.0f, 0, 12.9f, 55);
    aura_ring(p, 1, 13.0f, 90, 13.1f, 50);
    aura_ring(p, 2, 11.0f, 180, 13.3f, 45);
    p->records.rings[3].active = 0;
}

void rag_aura_pair(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimTexRecord *recs;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim_list1(self, 28);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 4.0f;
    recs = p->records.tex_records;
    recs[0].active = 1;
    recs[1].active = 1;
    recs[2].active = 0;
    recs[3].active = 0;
    recs[0].size = 46.0f;
    recs[1].size = 46.0f;
    recs[0].bob = 0;
    recs[1].bob = 180;
    recs[0].alpha = 25;
    recs[1].alpha = 25;
    recs[0].y = -1.0f;
    recs[1].y = -1.0f;
}

void rag_aura_disc(struct RagEffect *self, const char *tex, char look)
{
    struct EffectPrim *p;
    struct PrimTexRecord *rec;
    float size;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 26, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    if (look == 4 || look == 14)
        p->scale = 111.0f;
    else if (look == 5)
        p->scale = 112.0f;
    else
        p->scale = 110.0f;
    rec = &p->records.tex_records[0];
    rec->active = 1;
    p->records.tex_records[1].active = 0;
    p->records.tex_records[2].active = 0;
    p->records.tex_records[3].active = 0;
    if ((unsigned char)(look - 2) <= 1) {
        rec->size = 40.0f;
    } else if (look == 4 || look == 14) {
        size = RAG_SIZE_PARAM(self);
        rec->size = 0.0f >= size ? 40.0f : size;
    } else {
        rec->size = 30.0f;
    }
    if (look == 4 || look == 5 || look == 14) {
        rec->angle = 0;
        rec->alpha = 250;
        rec->y = -0.5f;
    } else if (look == 0 || look == 2) {
        rec->angle = 0;
        rec->alpha = 200;
        rec->y = -0.5f;
    } else {
        rec->angle = 358;
        rec->alpha = 50;
        rec->y = -0.4f;
    }
    rec->ground_style = (float)look;
}

void rag_aura_particles(struct RagEffect *self, const char *tex, char height, char look)
{
    struct EffectPrim *p;
    struct PrimLightGroup *g;
    float y;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 27, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    y = -(float)(height * 2);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        g = &p->records.light_groups[i];
        g->active = 1;
        g->size = 1.0f;
        g->sway[0].angle = (float)rand_below(360);
        g->sway[0].target = (float)rand_below(360);
        g->sway[1].angle = (float)rand_below(360);
        g->sway[1].target = (float)rand_below(360);
        g->point[2].x = 0.0f;
        g->point[2].y = y;
        g->point[2].z = 0.0f;
        g->angle[0] = (float)rand_below(360);
        g->point[0].x = 0.0f;
        g->point[0].y = y;
        g->point[0].z = 0.0f;
        g->angle[1] = (float)rand_below(360);
        g->point[3].x = 0.0f;
        g->point[3].y = y;
        g->point[3].z = 0.0f;
        g->angle[2] = (float)rand_below(360);
        g->point[1].x = 0.0f;
        g->point[1].y = y;
        g->point[1].z = 0.0f;
        g->angle[3] = (float)rand_below(360);
        g->style = look == 2 ? 2 : 0;
    }
}

void rag_thunderstorm_particle(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimBillboard *bb;
    float size;

    follow_master(self);
    if (self->frame != 0)
        return;
    size = RAG_SIZE_PARAM(self);
    if (1.0f >= size)
        size = 50.0f;
    p = launch_prim(self, 127, 0);
    if (!p)
        return;
    p->textures[0] = texture("effect\\썬더스톰파티클.tga");
    p->scale = 0.0f;
    bb = &p->records.billboards[0];
    bb->active = 1;
    bb->age = 0;
    bb->size = 1.0f;
    bb->alpha = 0;
    bb->local.x = 0.0f;
    bb->local.z = 0.0f;
    bb->local.y = -size;
    bb->heading = 0;
    bb->speed = 1.0f;
    bb->spin = (short)rand_below(360);
    bb->texture = 0;
    bb->color_style = 2;
    bb->rise_frames = (unsigned char)(int)size;
    p->records.billboards[1].active = 1;
    p->records.billboards[1].alpha = 0;
    p->records.billboards[2].active = 1;
    p->records.billboards[2].alpha = 0;
    p->records.billboards[3].active = 1;
    p->records.billboards[3].alpha = 0;
}

void rag_colored_quad(struct RagEffect *self, char style, unsigned char r,
                                 unsigned char g, unsigned char b)
{
    struct EffectPrim *p;
    struct PrimScrollGrid *grid;

    follow_master(self);
    if (self->frame == 99999998) {
        self->frame = 0;
        self->duration = 99999999;
    } else if (self->frame != 0) {
        return;
    }
    p = rag_launch_prim(self, 172, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags |= 0x200;
    p->duration = self->duration;
    p->motion_count = 1;
    p->textures = rag_prim_new_textures(p);
    p->textures[0] = texture("effect\\white02.bmp");
    self->base.pos.y -= 9.0f;
    grid = p->records.scroll_grids;
    grid->active = 1;
    grid->age = 0;
    grid->scroll = 0.0f;
    grid->alpha = 0;
    p->argb = ((unsigned)b << 16 | (unsigned)g << 8) | r;
    grid->style = (unsigned char)style;
}

void rag_texture_animation(struct RagEffect *self, char look)
{
    static const char *const dust[9] = {
        "effect\\dust01.bmp", "effect\\dust02.bmp", "effect\\dust03.bmp", "effect\\dust04.bmp", "effect\\dust05.bmp",
        "effect\\dust06.bmp", "effect\\dust07.bmp", "effect\\dust08.bmp", "effect\\dust09.bmp",
    };
    static const char *const red[13] = {
        "effect\\torch_red01.bmp", "effect\\torch_red02.bmp", "effect\\torch_red03.bmp", "effect\\torch_red04.bmp",
        "effect\\torch_red05.bmp", "effect\\torch_red06.bmp", "effect\\torch_red07.bmp", "effect\\torch_red08.bmp",
        "effect\\torch_red09.bmp", "effect\\torch_red10.bmp", "effect\\torch_red11.bmp", "effect\\torch_red12.bmp",
        "effect\\torch_red13.bmp",
    };
    static const char *const green[13] = {
        "effect\\torch_green01.bmp", "effect\\torch_green02.bmp", "effect\\torch_green03.bmp",
        "effect\\torch_green04.bmp", "effect\\torch_green05.bmp", "effect\\torch_green06.bmp",
        "effect\\torch_green07.bmp", "effect\\torch_green08.bmp", "effect\\torch_green09.bmp",
        "effect\\torch_green10.bmp", "effect\\torch_green11.bmp", "effect\\torch_green12.bmp",
        "effect\\torch_green13.bmp",
    };
    static const char *const violet[13] = {
        "effect\\torch_violet01.bmp", "effect\\torch_violet02.bmp", "effect\\torch_violet03.bmp",
        "effect\\torch_violet04.bmp", "effect\\torch_violet05.bmp", "effect\\torch_violet06.bmp",
        "effect\\torch_violet07.bmp", "effect\\torch_violet08.bmp", "effect\\torch_violet09.bmp",
        "effect\\torch_violet10.bmp", "effect\\torch_violet11.bmp", "effect\\torch_violet12.bmp",
        "effect\\torch_violet13.bmp",
    };
    static const char *const lord[11] = {
        "effect\\lord_stick_a.bmp", "effect\\lord_stick_b.bmp", "effect\\lord_stick_c.bmp",
        "effect\\lord_stick_e.bmp", "effect\\lord_stick_f.bmp", "effect\\lord_stick_g.bmp",
        "effect\\lord_stick_h.bmp", "effect\\lord_stick_plazma_a.bmp", "effect\\lord_stick_plazma_b.bmp",
        "effect\\lord_stick_plazma_c.bmp", "effect\\lord_blast_stick_a.bmp",
    };
    const char *const *names = NULL;
    struct EffectPrim *p;
    struct PrimTexRecord *rec;
    float size;
    int n = 0;

    follow_master(self);
    if (self->frame != 0)
        return;
    follow_master(self);
    p = rag_launch_prim(self, 128, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags = 5;
    p->duration = self->duration;
    switch (look) {
    case 0: names = dust; n = 9; break;
    case 2: names = green; n = 13; break;
    case 1: names = red; n = 13; break;
    case 3: names = violet; n = 13; break;
    case 4: names = lord; n = 11; break;
    }
    if (names != NULL) {
        p->motion_count = n;
        p->textures = rag_prim_new_textures(p);
        textures_from(p, names, n);
    }
    p->scale = 0.0f;
    rec = &p->records.tex_records[0];
    rec->age = 0;
    rec->active = 1;
    size = RAG_SIZE_PARAM(self);
    rec->size = 0.0f >= size ? 20.0f : size;
    rec->point[2].y = self->base.pos.y - 10.0f;
    rec->look = 4;
    if (look == 4) {
        rec->spin = 120;
        rec->shape = 1.0f;
    } else {
        rec->spin = 135;
        rec->shape = 0.0f;
    }
    rec->pulse = 0.0f;
    rec->alpha = look != 0 ? 130 : 100;
    rec->point[0].x = self->base.pos.x;
    rec->point[0].z = self->base.pos.z;
    rec->angle = (short)rand_below(360);
    rec->texture = 0;
    rec->style = (signed char)look;
}

void rag_sight_trail(struct RagEffect *self)
{
    float view[12];
    struct TexDrawVertex screen;
    struct EffectPrim *p;
    float s, c;
    int deg;

    rag_follow_master(self);
    if (!self->master || !game_copy_view_matrix(view))
        return;
    prim_project_vertex(&screen, view, self->master->pos);
    self->screen_pos.x = screen.x;
    self->screen_pos.y = screen.y;
    if (self->frame & 1)
        return;
    deg = (int)((float)self->frame * -5.0f);
    s = get_sin_deg(deg);
    c = get_cos_deg(deg);
    deg_to_rad(deg);
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    p->delta2.x = s * 15.0f;
    p->delta2.y = -20.0f;
    p->delta2.z = c * -15.0f;
    p->velocity_accel.y = 0.1f;
    p->motion_period = 3;
    p->scale = 2.5f;
    p->scale_speed = -0.1f;
    p->alpha = 50.0f;
    p->alpha_speed = -3.0f;
    render_object_load_spr_res(&p->base, EFFECT_DIR "Sight.spr");
    render_object_load_act_res(&p->base, EFFECT_DIR "Sight.act");
    prim_sync_particles(p);
}
