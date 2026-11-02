#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include <stdint.h>

#define MONSTER_DIR "몬스터\\"

#define RAG_MSG_F(self, i) ((self)->msg_params[i])

void rag_melody_column_156(struct RagEffect *self, const char *tex,
                                      unsigned char style)
{
    struct EffectPrim *p;
    struct PrimFanQuad *q;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 156, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    q = &p->records.fan_quads[0];
    q->active = 1;
    q->frame = 0;
    q->size = 6.0f;
    q->spin = 0.0f;
    q->alpha = 120;
    q->pos.x = self->base.pos.x;
    q->pos.z = self->base.pos.z;
    q->base_y = self->base.pos.y - 6.0f;
    q->bob = (short)rag_rand(360);
    q->angle = 45;
    q->texture = 0;
    q->jitter_style = style;
}

void rag_line_to_offset_154(struct RagEffect *self, unsigned char style,
                                       const char *tex)
{
    struct EffectPrim *p;
    struct PrimWedgePair *pair;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 154, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    pair = &p->records.wedge_pairs[0];
    pair->active = 1;
    pair->frame = 0;
    pair->reach = 50.0f;
    pair->side = 5.0f;
    prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &pair->angle);
    pair->alpha = 0;
    pair->style = style;
}

static void cloud_burst(struct RagEffect *self, const char *tex, signed char colour)
{
    struct EffectPrim *p;
    int i;

    p = self->last_prim = rag_launch_prim(self, 132, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 1.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimStandingSquare *sq = &self->last_prim->records.standing_squares[i];

        sq->active = 1;
        sq->frame = -20 * i;
        sq->phase = (short)rag_rand(360);
        sq->grow = 10.0f;
        sq->yaw = (short)rag_rand(360);
        sq->alpha = 100;
        sq->drop = 10.0f;
        sq->shade = (float)i;
        sq->style = (float)colour;
    }
}

void rag_cloud_bursts(struct RagEffect *self, const char *tex, signed char colour)
{
    self->base.pos = self->master->pos;
    if (self->frame == 0)
        cloud_burst(self, tex, colour);
    if (self->frame == 10)
        cloud_burst(self, tex, colour);
}

void rag_tinted_sparks(struct RagEffect *self, signed char style)
{
    struct EffectPrim *p;

    rag_project_master(self);
    if (self->frame & 1)
        return;
    p = rag_spark_launch(self, 5.0f, -0.015f);
    if (!p)
        return;
    if (style == 2) {
        p->r = rand() % 256;
        p->g = p->r;
        p->b = 255;
    } else {
        p->r = 255;
        p->g = rand() % 256;
        p->b = p->g;
    }
    rag_spark_finish(p);
}

void rag_falling_shards_157(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame >= 1000 || self->frame % 10 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 157, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimDropSquare *sq = &self->last_prim->records.drop_squares[i];

        sq->active = 1;
        sq->fall = (float)rag_rand(51) * 0.01f + 2.5f;
        sq->size = (float)rag_rand(31) * 0.1f + 7.0f;
        sq->frame = 0;
        sq->angle = (short)(rand() % 360);
        sq->radius = (float)(rand() % 33);
        sq->pos.y = (float)(rand() % 21 - 80);
        sq->alpha = 0;
        sq->style = (unsigned char)(rand() % 3);
    }
}

static float wrap_degrees(float a)
{
    if (0.0f > a)
        a += 360.0f;
    if (a >= 360.0f)
        a -= 360.0f;
    return a;
}

void rag_melody_notes_155(struct RagEffect *self, const char *tex, unsigned char style)
{
    struct EffectPrim *p;
    struct PrimTwinWisps *w;
    Vec3 from;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame >= 30 || self->frame % 10 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 155, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    w = &p->records.twin_wisps[0];
    from.x = self->base.pos.x;
    from.z = self->base.pos.z;
    from.y = self->base.pos.y - 5.0f;
    w->active = 1;
    w->speed = (float)rag_rand(21) * 0.01f + 0.8f;
    w->size = (float)rag_rand(21) * 0.1f + 3.0f;
    w->frame = 0;
    prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &w->heading);
    w->pos0.z = from.z - self->base.pos.z;
    w->pos0.x = from.x - self->base.pos.x;
    w->pos0.y = from.y - self->base.pos.y;
    w->course[0].base_yaw = (float)w->heading;
    w->course[0].yaw_target = (float)(w->heading - 45 + rand() % 91);
    w->course[0].yaw = (float)(w->heading - 45 + rand() % 91);
    w->course[0].pitch_target = 180.0f;
    w->course[0].pitch = 180.0f;
    w->alpha[0] = 0.0f;
    w->pos1.y = from.y - self->base.pos.y;
    w->pos1.x = from.x - self->base.pos.x;
    w->pos1.z = from.z - self->base.pos.z;
    w->course[1].base_yaw = (float)w->heading;
    w->course[1].yaw_target = (float)(w->heading - 45 + rand() % 91);
    w->course[1].yaw = (float)(w->heading - 45 + rand() % 91);
    w->course[1].pitch_target = 180.0f;
    w->course[1].pitch = 180.0f;
    for (i = 0; i < 2; i++) {
        struct PrimWispCourse *c = &w->course[i];

        c->base_yaw = wrap_degrees(c->base_yaw);
        c->yaw_target = wrap_degrees(c->yaw_target);
        c->yaw = wrap_degrees(c->yaw);
        c->pitch_target = wrap_degrees(c->pitch_target);
        c->pitch = wrap_degrees(c->pitch);
    }
    w->alpha[1] = 0.0f;
    w->style = style;
}

void rag_whitelight_151(struct RagEffect *self, const char *tex, unsigned char style)
{
    struct EffectPrim *p;
    struct PrimOrbitLauncher *rec;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 151, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    rec = &p->records.orbit_launchers[0];
    rec->active = 1;
    rec->frame = 0;
    rec->size = 2.5f;
    rec->owner = self->offset.y;
    rec->alpha = 0.0f;
    rag_rand(360);
    rag_rand(360);
    rag_rand(360);
    rag_rand(360);
    rec->radius = 5.0f;
    rec->pos.x = g_sinTable[90] * -30.0f;
    rec->pos.y = -(g_sinTable[0] * 20.0f) - 40.0f;
    rec->pos.z = 0.0f;
    rec->style = style;
}

void rag_mavka_152(struct RagEffect *self, unsigned char style)
{
    struct EffectPrim *p;
    int i, k;

    self->base.pos = self->master->pos;
    if (self->frame >= 10)
        return;
    p = self->last_prim = rag_launch_prim(self, 152, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = self->duration;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSpriteGhosts *g = &self->last_prim->records.sprite_ghosts[i];

        g->frame = 0;
        g->active = 1;
        self->last_prim->scale = 0.3f;
        g->offsets[2].x = 0.0f;
        g->offsets[2].y = 0.0f;
        g->offsets[0].x = 0.0f;
        g->offsets[0].y = 0.0f;
        g->offsets[3].x = 0.0f;
        g->offsets[3].y = 0.0f;
        g->spin_a = 0;
        g->spin_b = 0;
        g->speed = 2.0f;
        for (k = 0; k < 6; k++)
            g->drift[k] = (float)(rand() % 360);
        for (k = 0; k < 6; k++)
            g->target[k] = (float)(rand() % 360);
        g->style = style;
    }
    p = self->last_prim;
    p->alpha = 0.0f;
    render_object_load_spr_res(&p->base, MONSTER_DIR "mavka.spr");
    render_object_load_act_res(&p->base, MONSTER_DIR "mavka.act");
    p->motion_period = 8;
    p->list_flags |= 0x200;
    render_object_set_action_base(&p->base, 0, 1);
    prim_sync_particles(p);
}

static void monster_sprite(struct EffectPrim *p, const char *spr, const char *act)
{
    render_object_load_spr_res(&p->base, spr);
    render_object_load_act_res(&p->base, act);
}

static void monster_action(struct EffectPrim *p, int action)
{
    render_object_set_action_base(&p->base, action, 1);
    prim_sync_particles(p);
}

void rag_summoned_monsters(struct RagEffect *self, signed char style)
{
    void *attr = game_current_map_attr();
    struct EffectPrim *p;
    struct PrimSpriteHalo *halo;
    int last, kind_of;
    short deg;
    float dist;

    self->base.pos = self->master->pos;
    if (style == 13 || style == 14)
        last = 30;
    else if (style >= 9)
        last = 20;
    else
        last = style >= 2 ? 10 : 50;
    if (!attr || self->frame >= last)
        return;
    p = self->last_prim = rag_launch_prim(self, 153, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = self->duration - self->frame;
    halo = p->records.sprite_halos;
    halo[0].frame = 0;
    halo[0].active = 1;
    if (style == 2) {
        p->scale = 0.6f;
        dist = 40.0f;
        deg = (short)(self->frame * 36);
    } else if (style == 13) {
        deg = (short)(self->frame * 12);
        if (self->frame % 3 == 0) {
            dist = 8.0f;
            p->scale = 0.9f;
        } else if (self->frame % 3 == 1) {
            dist = 15.0f;
            p->scale = 1.0f;
        } else {
            dist = 22.0f;
            p->scale = 1.1f;
        }
    } else if (style == 14) {
        deg = (short)(self->frame * 12);
        if (self->frame % 3 == 0) {
            dist = 8.0f;
            p->scale = 0.5f;
        } else if (self->frame % 3 == 1) {
            dist = 14.0f;
            p->scale = 0.65f;
        } else {
            dist = 20.0f;
            p->scale = 0.8f;
        }
    } else if (style == 15) {
        deg = (short)(self->frame * 18);
        if (!(self->frame & 1)) {
            dist = 4.0f;
            p->scale = (float)(rand() % 11) * 0.05f + 1.5f;
        } else {
            dist = 10.0f;
            p->scale = (float)(rand() % 11) * 0.05f + 2.0f;
        }
    } else if (style == 9) {
        deg = (short)(self->frame * 18);
        if (!(self->frame & 1)) {
            dist = 12.0f;
            p->scale = 0.85f;
        } else {
            dist = 24.0f;
            p->scale = 0.95f;
        }
    } else if (style >= 10) {
        deg = (short)(self->frame * 18);
        if (!(self->frame & 1)) {
            dist = 6.0f;
            p->scale = (float)(rand() % 11) * 0.03f + 0.4f;
        } else {
            dist = 12.0f;
            p->scale = (float)(rand() % 11) * 0.03f + 0.5f;
        }
    } else if (style >= 3) {
        p->scale = 1.0f;
        dist = 40.0f;
        deg = (short)(self->frame * 36);
    } else {
        p->scale = 0.7f;
        deg = (short)(rand() % 360);
        dist = (float)(rand() % 45 + 5);
    }
    p = self->last_prim;
    p->base.pos = p->master->base.pos;
    p->base.pos.x += g_sinTable[deg] * dist;
    p->base.pos.z += g_cosTable[deg] * dist;
    p->base.pos.y = c3d_attr_get_height(attr, p->base.pos.x, p->base.pos.z);
    halo = p->records.sprite_halos;
    halo[0].additive = style == 15;
    p->alpha = 0.0f;
    rand();
    switch (style) {
    case 2:
        monster_sprite(p, MONSTER_DIR "워그.spr", MONSTER_DIR "워그.act");
        p->motion_period = rand() % 3 + 6;
        monster_action(p, rand() % 8);
        break;
    case 3:
        monster_sprite(p, MONSTER_DIR "violy.spr", MONSTER_DIR "violy.act");
        p->motion_period = rand() % 3 + 6;
        monster_action(p, rand() % 8);
        break;
    case 4:
        p->motion_period = rand() % 3 + 6;
        kind_of = (unsigned short)(rand() % 3);
        monster_sprite(p, MONSTER_DIR "succubus.spr", MONSTER_DIR "succubus.act");
        if (kind_of == 0)
            monster_action(p, rand() % 8 + 56);
        else if (kind_of == 1)
            monster_action(p, rand() % 8 + 48);
        else
            monster_action(p, rand() % 8 + 40);
        break;
    case 5:
        monster_sprite(p, MONSTER_DIR "mineral.spr", MONSTER_DIR "mineral.act");
        p->motion_period = rand() % 3 + 6;
        monster_action(p, rand() % 8);
        break;
    case 6:
        monster_sprite(p, MONSTER_DIR "ktullanux.spr", MONSTER_DIR "ktullanux.act");
        p->motion_period = rand() % 3 + 6;
        monster_action(p, rand() % 8);
        break;
    case 7:
        monster_sprite(p, MONSTER_DIR "garm.spr", MONSTER_DIR "garm.act");
        p->motion_period = rand() % 3 + 6;
        monster_action(p, rand() % 8);
        break;
    case 8:
        monster_sprite(p, MONSTER_DIR "miyabi_ningyo.spr", MONSTER_DIR "miyabi_ningyo.act");
        p->motion_period = rand() % 3 + 6;
        monster_action(p, rand() % 8 + 48);
        break;
    case 9:
        monster_sprite(p, MONSTER_DIR "garden_keeper.spr", MONSTER_DIR "garden_keeper.act");
        p->motion_period = 4;
        monster_action(p, rand() % 8 + 16);
        break;
    case 13:
        monster_sprite(p, MONSTER_DIR "ginseng_bullet.spr", MONSTER_DIR "ginseng_bullet.act");
        p->motion_period = 3;
        monster_action(p, rand() % 8);
        break;
    case 14:
        monster_sprite(p, MONSTER_DIR "nepenthes2_bullet.spr", MONSTER_DIR "nepenthes2_bullet.act");
        p->motion_period = 4;
        monster_action(p, 0);
        break;
    case 15:
        monster_sprite(p, MONSTER_DIR "imp.spr", MONSTER_DIR "imp.act");
        p->motion_period = 4;
        monster_action(p, rand() % 8);
        break;
    default:
        if (style >= 10) {
            monster_sprite(p, MONSTER_DIR "ifrit.spr", MONSTER_DIR "ifrit.act");
            p->motion_period = 4;
            monster_action(p, rand() % 8 + 32);
        } else {
            monster_sprite(p, MONSTER_DIR "geographer.spr", MONSTER_DIR "geographer.act");
            p->motion_period = 4;
            monster_action(p, 0);
        }
        break;
    }
}

static void scale_and_turn(Vec3 *v, float k, short h)
{
    float c, s, x, z;

    v->x *= k;
    v->z *= k;
    v->y *= k;
    c = g_cosTable[h];
    s = g_sinTable[h];
    x = v->x;
    z = v->z;
    v->x = c * x - s * z;
    v->z = s * x + c * z;
}

static void scatter_point(Vec3 *v)
{
    v->x = (float)(rag_rand(45) - 22);
    v->y = (float)rag_rand(30);
    v->z = (float)(rag_rand(11) - 5);
}

void rag_ice_shards_150(struct RagEffect *self)
{
    struct EffectPrim *p;
    unsigned char water;
    int i;

    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = self->last_prim = rag_launch_prim(self, 150, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    if (rag_rand(10) > 0) {
        p->textures[0] = RAG_TEXTURE("effect\\freeze_a_small.bmp");
        water = 0;
    } else {
        p->textures[0] = RAG_TEXTURE("effect\\waterp1.tga");
        water = 1;
    }
    p->scale = 4.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRisingMotes *rec = &self->last_prim->records.rising_motes[i];
        unsigned n, m;
        float k;
        short h;

        rec->yaw = (short)(int)RAG_MSG_F(self, 1);
        rec->yaw = (short)(rec->yaw % 360);
        h = rec->yaw;
        if (10.0f > RAG_MSG_F(self, 0))
            RAG_MSG_F(self, 0) = 10.0f;
        rec->size10 = (short)(int)RAG_MSG_F(self, 0);
        k = (float)rec->size10 / 10.0f;
        n = (unsigned)(int64_t)RAG_MSG_F(self, 2);
        m = n % 1000000;
        rec->tint[0] = (unsigned char)(m / 10000);
        rec->tint[1] = (unsigned char)((m - rec->tint[2] * 10000) / 100);
        rec->tint[2] = (unsigned char)(m % 100);
        rec->active = 1;
        rec->rise = 0.05f;
        rec->size = k * (water == 1 ? 7.0f : 4.5f);
        rec->motes[2].y = (float)rag_rand(30);
        rec->drift[0][0] = (float)rag_rand(360);
        rec->drift[0][1] = (float)rag_rand(360);
        rec->drift[1][0] = (float)rag_rand(360);
        rec->drift[1][1] = (float)rag_rand(360);
        rec->motes[2].x = (float)(rag_rand(45) - 22);
        rec->motes[2].z = (float)(rag_rand(11) - 5);
        scale_and_turn(&rec->motes[2], k, h);
        scatter_point(&rec->motes[0]);
        rec->drift[2][0] = (float)rag_rand(360);
        rec->drift[2][1] = (float)rag_rand(360);
        rec->drift[3][0] = (float)rag_rand(360);
        rec->drift[3][1] = (float)rag_rand(360);
        scale_and_turn(&rec->motes[0], k, h);
        scatter_point(&rec->motes[3]);
        rec->drift[4][0] = (float)rag_rand(360);
        rec->drift[4][1] = (float)rag_rand(360);
        rec->drift[5][0] = (float)rag_rand(360);
        rec->drift[5][1] = (float)rag_rand(360);
        scale_and_turn(&rec->motes[3], k, h);
        scatter_point(&rec->motes[1]);
        rec->drift[6][0] = (float)rag_rand(360);
        rec->drift[6][1] = (float)rag_rand(360);
        rec->drift[7][0] = (float)rag_rand(360);
        rec->drift[7][1] = (float)rag_rand(360);
        scale_and_turn(&rec->motes[1], k, h);
    }
}
