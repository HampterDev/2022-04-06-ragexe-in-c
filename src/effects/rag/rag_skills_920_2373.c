#include "contents/sound.h"
#include "session/state.h"
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "effects/effect_mgr.h"
#include "render/render_api.h"
#include "actor/actor_common.h"
#include "actor/actor_ops.h"
#include "core/object_type.h"

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

void rag_hallu_doubles_2(struct RagEffect *self)
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
        if (self->frame % 5 != 0)
            return;
        if (!object_as_type(self->master, OBJECT_PC))
            return;
        mem = mem_alloc(sizeof(struct HalluPC2));
        dbl = mem ? (struct RenderObject *)hallu_pc2_construct(mem, (struct Pc *)self->master) : NULL;
        if (!dbl)
            return;
        m = self->master;
        render_object_set_argb(dbl, -1, HALF_TINT(self, m, 0, self->base.pos.x),
                               HALF_TINT(self, m, 1, self->base.pos.y), HALF_TINT(self, m, 2, self->base.pos.z));
        effect_add_world_object(world, dbl);
        return;
    }
    if (self->frame == 9998)
        self->frame = 0;
    if (self->duration == 30) {
        if (!object_as_type(self->master, OBJECT_PC))
            return;
        mem = mem_alloc(sizeof(struct HalluPC2));
        dbl = mem ? (struct RenderObject *)hallu_pc2_construct(mem, (struct Pc *)self->master) : NULL;
        if (!dbl)
            return;
        effect_add_world_object(world, dbl);
        return;
    }
    if (self->frame % 5 != 0)
        return;
    if (!object_as_type(self->master, OBJECT_PC))
        return;
    mem = mem_alloc(sizeof(struct HalluPC2));
    dbl = mem ? (struct RenderObject *)hallu_pc2_construct(mem, (struct Pc *)self->master) : NULL;
    if (!dbl)
        return;
    effect_add_world_object(world, dbl);
}

void rag_vec3_mat_apply(Vec3 *out, const Vec3 *v, const float *m)
{
    out->x = m[3] * v->y + v->x * m[0] + m[6] * v->z + m[9];
    out->y = m[1] * v->x + m[4] * v->y + m[7] * v->z + m[10];
    out->z = m[2] * v->x + m[5] * v->y + m[8] * v->z + m[11];
}

static struct EffectPrim *water_disc(struct RagEffect *self, int kind, int duration, float size)
{
    struct EffectPrim *p;

    p = self->last_prim = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;
    p->duration = duration;
    if (kind == 12) {
        p->list_flags |= 0x200;
        p->angle[1] = 90.0f;
    }
    p->size[0].speed = size;
    p->size[0].accel = p->size[0].speed / (float)p->duration * -0.5f;
    if (kind == 12)
        p->radius[0] = 12.0f;
    else
        p->angle_speed[0] = 3.0f;
    p->alpha = 0.0f;
    p->alpha_speed = p->alpha_max / 15.0f;
    p->fade_frame = 10;
    return p;
}

void rag_water_ball_rings(struct RagEffect *self)
{
    struct EffectPrim *p;

    if (self->frame != 0)
        return;
    sound_play_wave("effect\\wizard_waterball_chulung.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
    p = water_disc(self, 12, 30, 1.75f);
    if (!p)
        return;
    prim_sync_particles(p);
    RAG_NEW_TEXTURES(self->last_prim);
    self->last_prim->textures[0] = RAG_TEXTURE("effect\\ring_blue.tga");
    p = water_disc(self, 14, 30, 1.15f);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    self->last_prim->textures[0] = RAG_TEXTURE("effect\\ice.tga");
    p = water_disc(self, 12, 20, 1.75f);
    if (!p)
        return;
    prim_sync_particles(p);
    RAG_NEW_TEXTURES(self->last_prim);
    self->last_prim->textures[0] = RAG_TEXTURE("effect\\ring_blue.tga");
}

void rag_blue_rings_169(struct RagEffect *self, const char *tex, signed char scale)
{
    static const float radius[PRIM_RECORD_COUNT] = { 15.0f, 16.0f, 17.0f, 18.0f };
    static const float base[PRIM_RECORD_COUNT] = { 0.5f, 1.0f, 1.5f, 2.0f };
    struct EffectPrim *p;
    struct PrimRing *s;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame > 3)
        return;
    p = launch(self, 169);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p = self->last_prim;
    p->scale = (float)scale;
    s = p->records.rings;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        s[i].active = 0;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        s[i].active = 1;
        s[i].age = 0;
        s[i].span = 360;
        s[i].height = radius[i];
        s[i].turn = (short)rag_rand(360);
        s[i].alpha = (short)rag_rand(100);
        s[i].radius = (float)(self->frame + self->frame) + base[i];
        s[i].ripple_start = s[i].radius;
        if (i % 2 == 0)
            s[i].radius = s[i].radius - (float)(rand() % 21) * 0.1f;
        else
            s[i].radius = s[i].radius + (float)(rand() % 21) * 0.1f;
        s[i].tilt = 80;
    }
}

void rag_snow_134(struct RagEffect *self, unsigned char style)
{
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = launch(self, 134);
    if (!p)
        return;
    p->motion_count = 4;
    RAG_NEW_TEXTURES(p);
    self->last_prim->textures[0] = RAG_TEXTURE("effect\\E9_snow.tga");
    self->last_prim->textures[1] = RAG_TEXTURE("effect\\E9_snow01.tga");
    self->last_prim->textures[2] = RAG_TEXTURE("effect\\E9_snow02.tga");
    self->last_prim->textures[3] = RAG_TEXTURE("effect\\E9_snow03.tga");
    self->last_prim->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimShapeRow *row = &self->last_prim->records.shape_rows[i];

        row->active = 0;
        row->frame = 0;
        row->active = 1;
        row->size = (float)(rand() % 21) * 0.08f + 2.5f;
        row->start_y = self->base.pos.y - 12.0f;
        row->look = (signed char)(rand() % 4);
        row->shape = 0.0f;
        row->angle = (short)(rand() % 360);
        row->wobble = 0.0f;
        row->alpha = 0;
        self->last_prim->delta = self->offset;
        prim_heading_degrees(self->base.pos.x - self->offset.x, self->base.pos.z - self->offset.z, &row->heading);
        row->heading += (short)(rand() % 41 + 160);
        if (row->heading >= 360)
            row->heading -= 360;
        row->target.x = self->base.pos.x;
        row->target.z = self->base.pos.z;
        row->target.y = row->start_y;
        rag_rand(360);
        row->texture = 3;
        row->pace = (signed char)(60 - rand() % 21);
    }
}

void rag_blue_rings_121(struct RagEffect *self)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    p = launch(self, 121);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE("effect\\ring_blue.tga");
    p = self->last_prim;
    p->scale = 4.0f;
    p->delta = self->offset;
    self->last_prim->delta2 = self->offset2;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimArcWall *wall = &self->last_prim->records.arc_walls[i];

        if (i == 3) {
            wall->active = 0;
            continue;
        }
        wall->active = 1;
        wall->age = 0;
        wall->arc = 315;
        wall->full_height = 13.0f - ((float)i + (float)i);
        wall->start = (short)(i * 90);
        wall->alpha = 0;
        wall->radius = (float)i * 0.2f + (self->offset2.y == 0.0f ? 3.4f : 2.4f);
        wall->lean = (short)((11 - i) * 5);
        wall->look = 99;
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

void rag_melody_notes_170(struct RagEffect *self, const char *tex, unsigned char style)
{
    struct EffectPrim *p;
    float x0, y0, z0;
    int i, k;

    self->base.pos = self->master->pos;
    if (self->frame >= 30 || self->frame % 10 != 0)
        return;
    p = rag_launch_prim(self, 170, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    if (p == NULL)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    if (p->motion_count > 0) {
        RAG_NEW_TEXTURES(p);
        if (p->textures != NULL)
            p->textures[0] = RAG_TEXTURE(tex);
    }
    p->scale = 0.0f;
    x0 = self->base.pos.x;
    z0 = self->base.pos.z;
    y0 = self->base.pos.y - 5.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimTwinWisps *w = &p->records.twin_wisps[i];

        w->active = 1;
        w->speed = (float)rag_rand(21) * 0.001f + 0.4f;
        if (style == 3)
            w->size = (float)rag_rand(41) * 0.1f + 1.0f;
        else
            w->size = (float)rag_rand(21) * 0.1f + 3.0f;
        w->frame = 0;
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &w->heading);
        w->pos0.z = z0 - self->base.pos.z;
        w->pos0.x = x0 - self->base.pos.x;
        w->pos0.y = y0 - self->base.pos.y;
        w->course[0].base_yaw = (float)w->heading;
        w->course[0].yaw_target = (float)(w->heading + rand() % 360);
        w->course[0].yaw = (float)(w->heading + rand() % 360);
        w->course[0].pitch_target = 180.0f;
        w->course[0].pitch = 180.0f;
        w->alpha[0] = 0.0f;
        w->pos1.x = x0 - self->base.pos.x;
        w->pos1.y = y0 - self->base.pos.y;
        w->pos1.z = z0 - self->base.pos.z;
        w->course[1].base_yaw = (float)w->heading;
        w->course[1].yaw_target = (float)(w->heading + rand() % 360);
        w->course[1].yaw = (float)(w->heading + rand() % 360);
        w->course[1].pitch_target = 180.0f;
        w->course[1].pitch = 180.0f;
        for (k = 0; k < 2; k++) {
            struct PrimWispCourse *c = &w->course[k];

            c->base_yaw = wrap_degrees(c->base_yaw);
            c->yaw_target = wrap_degrees(c->yaw_target);
            c->yaw = wrap_degrees(c->yaw);
            c->pitch_target = wrap_degrees(c->pitch_target);
            c->pitch = wrap_degrees(c->pitch);
        }
        w->alpha[1] = 0.0f;
        w->style = style;
    }
}

void rag_music_rings_173(struct RagEffect *self,
                                    signed char style, float rim)
{
    static const short angle2[PRIM_RECORD_COUNT] = { 60, 55, 65, 70 };
    struct EffectPrim *p;
    struct PrimSwellRing *s;
    char name[128];
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    rag_play_wave(self, "effect\\EF_BeginSpell.wav", -100.0f);
    if (self->duration < 120)
        self->duration = 120;
    p = launch(self, 173);
    if (!p)
        return;
    p->motion_count = 9;
    RAG_NEW_TEXTURES(p);
    for (i = 0; i < 9; i++) {
        sprintf(name, "effect\\new_severerainstorm\\new_severerainstorm_cylinder\\music_%02d.tga", i + 1);
        self->last_prim->textures[i] = RAG_TEXTURE(name);
    }
    self->last_prim->scale = 0.0f;
    s = self->last_prim->records.swell_rings;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        s[i].active = 1;
        s[i].frame = 0;
        s[i].arc = 120;
        s[i].radius = 20.0f;
        s[i].spin = (short)(i * 90);
        s[i].alpha = 0;
        s[i].rim = rim;
        s[i].tilt = angle2[i];
    }
}

void rag_yellow_lines_56(struct RagEffect *self, short sel)
{
    struct EffectPrim *p;
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
    p->textures[0] = RAG_TEXTURE("effect\\line3y.tga");
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
        st->half_width = (float)rag_rand(4) * 0.1f + 0.3f;
        st->length = (float)rag_rand(31) + 20.0f;
        st->color = (unsigned char)rag_rand(3);
        rag_rand(21);
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &st->heading);
        st->alpha = 0;
    }
}

void rag_green_rocket_punches(struct RagEffect *self)
{
    struct EffectPrim *p;
    float dist;
    short heading, a;

    if (self->frame != 0 && self->frame != 2 && self->frame != 4 && self->frame != 6)
        return;
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
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
    p->r = 105;
    p->g = 255;
    p->b = 105;
    p->alpha = (float)((5 - self->frame) * 40);
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

void rag_white_rings_63(struct RagEffect *self, signed char style)
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
    p = self->last_prim;
    p->scale = (float)style;
    s = p->records.rings;
    prim_sub_ring(&s[0], 0, 6.001f, 0, 0.0f, 2, 100, 1);
    prim_sub_ring(&s[1], -10, 6.001f, 25, 0.0f, 3, 100, 1);
    prim_sub_ring(&s[2], -20, 6.001f, 50, 0.0f, 4, 100, 1);
    s[3].active = 0;
    s[3].age = 0;
    s[3].mode = style == 29 ? 5 : 1;
}

void rag_sonic_waves_146(struct RagEffect *self, const char *tex, signed char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0 && self->frame != 20 && self->frame != 40)
        return;
    self->base.pos = self->master->pos;
    p = launch(self, 146);
    if (!p)
        return;
    p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWaveRing *s = &self->last_prim->records.wave_rings[i];

        s->active = 1;
        s->frame = -8 * i;
        s->base_height = style == 0 ? 30.0f : 20.0f;
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &s->arc_start);
        s->arc = 315;
        s->arc_start += 195;
        if (s->arc_start >= 360)
            s->arc_start -= 360;
        s->alpha = 0;
        s->radius = (style == 0 ? 6.0f : 9.0f) - (float)i * 0.1f;
        s->tilt = (short)((14 - i) * 5);
        s->walked.x = 0.0f;
        s->walked.z = 0.0f;
        s->strip_style = 0.0f;
    }
}

void rag_digital_space_168(struct RagEffect *self, int effect_id)
{
    struct EffectPrim *p;

    if (self->master != NULL)
        self->base.pos = self->master->pos;
    if (self->frame == 0) {
        p = self->last_prim = rag_launch_prim(self, 168, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        if (effect_id == 1240) {
            render_object_load_spr_res(&self->last_prim->base, EFFECT_DIR "digital_space\\digital_space.spr");
            render_object_load_act_res(&self->last_prim->base, EFFECT_DIR "digital_space\\digital_space.act");
            self->last_prim->base.action = 0;
            self->last_prim->loop_motion = 1;
            self->last_prim->motion_period = 8;
            self->last_prim->list_flags = 0x209;
            self->last_prim->pattern |= 0x200;
        }
    }
    if (self->master != NULL && self->last_prim != NULL) {
        self->last_prim->master_sprite_lift = self->master->hover_height;
    }
    if (self->master != NULL && self->last_prim != NULL)
        self->last_prim->alpha = (float)(((struct AbleToMakeEffect *)self->master)->half_alpha ? 70 : 255);
    if (self->frame == self->duration - 1) {
        self->frame = 0;
        if (self->last_prim)
            self->last_prim->frame = 0;
    }
}

void rag_cloud_dash_2(struct RagEffect *self)
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

            pipe->active = 1;
            pipe->age = -rag_rand(20);
            prim_heading_degrees(dx, dz, &pipe->heading);
            pipe->alpha = 0;
            pipe->start.x = (float)rag_rand(100) * 0.3f - 15.0f + self->base.pos.x;
            pipe->start.z = (float)rag_rand(100) * 0.3f - 15.0f + self->base.pos.z;
            pipe->start.y = self->base.pos.y;
            pipe->start.x = pipe->start.x - g_cosTable[pipe->heading] * 100.0f;
            pipe->start.z = pipe->start.z - g_sinTable[pipe->heading] * 100.0f;
            pipe->end.x = pipe->start.x;
            pipe->end.z = pipe->start.z;
            pipe->end.y = self->base.pos.y;
            pipe->radius = 0.25f;
            pipe->drop = (float)rag_rand(100) * 0.04f + 8.0f;
            pipe->length = (float)rag_rand(100) * 0.5f + 100.0f;
            pipe->style = 5.0f;
        }
    }
    if (self->frame == 10)
        rag_play_wave_here(self, "effect\\EF_hit2.wav");
    if (self->frame == 20)
        rag_play_wave_here(self, "effect\\EF_hit4.wav");
}

void rag_hallu_walk(struct RagEffect *self)
{
    struct World *world = game_current_world();
    struct RenderObject *dbl;
    void *mem;
    float dx, dy, dz, len2, half, inv;

    if (!world || !self->master)
        return;
    dx = self->master->pos.x - self->offset.x;
    dy = self->master->pos.y - self->offset.y;
    dz = self->master->pos.z - self->offset.z;
    len2 = dx * dx + dy * dy + dz * dz;
    half = float_sqrt(len2) * 0.5f;
    inv = 1.0f / float_sqrt(len2);
    dx *= inv;
    dy *= inv;
    dz *= inv;
    render_object_set_argb(self->master, -1, 200, 200, 0);
    if (self->frame % 5 != 0)
        return;
    if (!object_as_type(self->master, OBJECT_PC))
        return;
    mem = mem_alloc(sizeof(struct HalluPC));
    dbl = mem ? hallu_pc_construct(mem, self->master) : NULL;
    if (!dbl)
        return;
    dbl->pos = self->offset;
    render_object_send(dbl, &self->base, 0, 200, 200, 0, 0);
    effect_add_world_object(world, dbl);
    self->offset.x = dx * half + self->offset.x;
    self->offset.y = dy * half + self->offset.y;
    self->offset.z = dz * half + self->offset.z;
}

void rag_fighter_rings_31(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    __int64 i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = launch(self, 31);
    if (!p)
        return;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    self->last_prim->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimRing *s = &self->last_prim->records.rings[(int)i];

        s->active = 1;
        s->age = 0;
        s->span = 360;
        s->height = 25.0f;
        s->tilt = 80;
        s->turn = (short)(i * 90);
        s->alpha = 0;
        s->mode = 5;
        s->radius = (float)(double)i + 1.0f;
        s->offset_flag = 0.0f;
        self->last_prim->accel = 99.0f;
    }
}

void rag_after_image(struct RagEffect *self)
{
    struct World *world = game_current_world();
    struct RenderObject *dbl;
    void *mem;
    float d;

    if (!world || !self->master || !object_as_type(self->master, OBJECT_PC))
        return;
    mem = mem_alloc(sizeof(struct HalluPC2));
    dbl = mem ? (struct RenderObject *)hallu_pc2_construct(mem, (struct Pc *)self->master) : NULL;
    if (!dbl)
        return;
    dbl->pos = self->master->pos;
    d = get_sin_deg((int)self->master->roty) * 5.0f;
    dbl->pos.x -= d;
    d = get_cos_deg((int)self->master->roty) * 5.0f;
    dbl->pos.z += d;
    render_object_set_argb(dbl, 100, 192, 192, 192);
    effect_add_world_object(world, dbl);
}

void rag_jack_frost(struct RagEffect *self)
{
    struct EffectPrim *p;
    float m[12];
    Vec3 at, v;
    int i, r;

    self->base.pos = self->master->pos;
    if (self->frame % 40 == 0) {
        v.y = 0.0f;
        v.z = 0.0f;
        for (i = 1; i < 4; i++) {
            at = self->base.pos;
            r = rand() % 17;
            if (r < 5)
                r = 5;
            v.x = (float)r;
            mat_set_rotate_y(m, (float)i * 5.0f * 20.0f / 180.0f * 3.141592f);
            rag_vec3_mat_apply(&at, &v, m);
            at.x += self->base.pos.x;
            at.y = self->base.pos.y + at.y;
            at.z = self->base.pos.z + at.z;
            effect_mgr_spawn(g_effectMgr, 0, g_session1.map_rule_flags[0] == 1 ? 0x608 : 0x5a7, at, 0, -1, 0, 0);
        }
    }
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 0xf, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = self->duration;
    p->pattern |= 0x800;
    p->angle[0] = 0.0f;
    p->angle_speed[0] = 2.5f;
    p->angle[2] = 90.0f;
    p->next_size1_accel = 0.0f;
    p->next_size2_accel = 0.0f;
    p->size[1].value = 40.0f;
    p->size[2].value = 40.0f;
    p->size[1].speed = -1.0f;
    p->size[2].speed = -1.0f;
    p->change_frame = 20;
    p->next_size1_speed = 0.0f;
    p->next_size2_speed = 0.0f;
    RAG_NEW_TEXTURES(p);
    p->r = 254;
    p->b = 254;
    p->g = 254;
    p->textures[0] = RAG_TEXTURE("effect\\new_jackfrost\\new_jackfrost_magicrune\\jack_magic_rune.tga");
}
