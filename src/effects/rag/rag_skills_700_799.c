#include "contents/sound.h"
#include "session/state.h"
#include "game/context.h"
#include <string.h>
#include "core/object_type.h"
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/actor_common.h"
#include "actor/actor_ops.h"
#include "session/session_jobs.h"

#define EFFECT_DIR "이팩트\\"

void rag_stone_chips(struct RagEffect *self)
{
    int i;

    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\wizard_icewall.wav");
    self->duration = 9999;
    for (i = -2; i < 4; i += 2) {
        struct EffectPrim *p;
        float s, c;
        int deg;

        p = self->last_prim = rag_launch_prim(self, 9, 0.0f, 0.0f, 0.0f);
        if (!p)
            return;
        p->duration = self->duration;
        p->pattern |= 1;
        deg = rag_rand(360);
        s = get_sin_deg(deg);
        c = get_cos_deg(deg);
        p->delta2.y = 30.0f;
        p->delta2.x = s * 0.0f;
        p->delta2.z = -((float)i * c);
        p->scale = (float)(rag_rand(40) / 100) + 2.5f;
        p->size[2].value = (float)(rag_rand(100) + 300) / 10.0f;
        p->angle[0] = (float)rag_rand(360);
        p->angle[1] = (float)(rag_rand(6) + 87);
        p->pattern_arg = 20;
        p->alpha = 200.0f;
        mat_set_rotate_x(p->matrix, deg_to_rad((int)p->angle[1]));
        mat_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
        memcpy(p->matrix2, p->matrix, sizeof(p->matrix));
        p->speed = 3.0f;
        p->accel = p->speed / 40.0f * -2.0f;
        p->fade_frame = p->duration - 10;
        RAG_NEW_TEXTURES(p);
        p->textures[0] = RAG_TEXTURE("effect\\stone.bmp");
    }
}

static void fireball(struct RagEffect *self, int turned, float turn, int r, int g, int b, int last_frame)
{
    struct EffectPrim *p;
    float dist;
    p = self->last_prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->duration = 20;
    p->pattern |= 0x120;
    p->delta2.x = 0.0f;
    p->delta2.y = -15.0f;
    p->delta2.z = 0.0f;
    render_object_turn_toward(&p->base, self->offset.x, self->offset.z);
    if (turned) {
        p->base.roty += turn;
        if (p->base.roty < 0.0f)
            p->base.roty += 360.0f;
        if (p->base.roty >= 360.0f)
            p->base.roty -= 360.0f;
    }
    p->angle[0] = -p->base.roty;
    p->base.roty = 180.0f - p->base.roty;
    if (p->base.roty < 0.0f)
        p->base.roty += 360.0f;
    dist = render_object_calc_ground_dist(&p->base, self->offset.x, self->offset.z);
    p->speed = -(dist / (float)p->duration);
    mat_set_rotate_y(p->matrix, deg_to_rad((int)p->angle[0]));
    p->scale = 1.0f;
    p->motion_period = 1;
    p->r = r;
    p->g = g;
    p->b = b;
    p->alpha = (float)((last_frame - self->frame) * 40);
    render_object_load_spr_res(&p->base, EFFECT_DIR "fireball.spr");
    render_object_load_act_res(&p->base, EFFECT_DIR "fireball.act");
    prim_sync_particles(p);
}

void rag_fireball_volleys(struct RagEffect *self)
{
    if (self->frame == 5)
        sound_play_wave("effect\\EF_FireBall.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
    if (self->frame >= 0 && self->frame <= 3)
        fireball(self, 0, 0.0f, 0x9b, 0x9b, 0xff, 5);
    if (self->frame >= 6 && self->frame <= 9)
        fireball(self, 1, -5.0f, 0x9b, 0xff, 0x9b, 11);
    if (self->frame >= 12 && self->frame <= 15)
        fireball(self, 1, 5.0f, 0xff, 0x9b, 0x9b, 17);
}

void rag_hallucination_doubles(struct RagEffect *self)
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
        if (self->frame % 20 != 0)
            return;
        if (!object_as_type(self->master, OBJECT_PC)
            || session_is_pc_job(&g_session1, game_actor_get_job((struct GameActor *)self->master)) == 0) {
            mem = mem_alloc(sizeof(struct HalluNPC));
            dbl = mem ? (struct RenderObject *)hallu_npc_construct(mem, (struct GameActor *)self->master) : NULL;
            if (!dbl)
                return;
        } else {
            mem = mem_alloc(sizeof(struct HalluPC));
            dbl = mem ? hallu_pc_construct(mem, self->master) : NULL;
            if (!dbl)
                return;
        }
        m = self->master;
        render_object_set_argb(dbl, -1, HALF_TINT(self, m, 0, self->base.pos.x),
                               HALF_TINT(self, m, 1, self->base.pos.y), HALF_TINT(self, m, 2, self->base.pos.z));
    } else {
        render_object_set_argb(m, -1, (int)self->base.pos.x, (int)self->base.pos.y, (int)self->base.pos.z);
        if (self->frame == 9998)
            self->frame = 0;
        if (self->duration != 41 && self->frame % 20 != 0)
            return;
        if (!object_as_type(self->master, OBJECT_PC)
            || session_is_pc_job(&g_session1, game_actor_get_job((struct GameActor *)self->master)) == 0) {
            mem = mem_alloc(sizeof(struct HalluNPC));
            dbl = mem ? (struct RenderObject *)hallu_npc_construct(mem, (struct GameActor *)self->master) : NULL;
            if (!dbl)
                return;
        } else {
            mem = mem_alloc(sizeof(struct HalluPC));
            dbl = mem ? hallu_pc_construct(mem, self->master) : NULL;
            if (!dbl)
                return;
        }
        render_object_send(dbl, &self->base, 0, (int)self->base.pos.x, (int)self->base.pos.y,
                           (int)self->base.pos.z, 0);
    }
    effect_add_world_object(world, dbl);
}

unsigned char rag_tinted_puffs(struct RagEffect *self)
{
    if (self->frame == 0) {
        render_object_send(self->master, NULL, 0x7f, 250, 250, 200, 0);
        rag_potion_pillar(self, 1.0f, 50);
    }
    if (self->frame == 18)
        game_send_message(0x7a, 1, 0, 0, 0);
    self->base.pos = self->master->pos;
    if ((unsigned)(self->frame - 13) <= 0x60 && self->frame % 4 == 0) {
        struct EffectPrim *p;
        float dist;

        p = self->last_prim = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
        if (!p)
            return 0;
        p->duration = 40;
        mat_set_rotate_y(p->matrix, deg_to_rad(rag_rand(360)));
        dist = (float)(rag_rand(7) + 2);
        p->delta2 = rag_mat_apply(p->matrix, 0.0f, 0.0f, dist);
        mat_set_rotate_x(p->matrix, deg_to_rad(90));
        p->speed = (float)(rag_rand(50) + 20) / 60.0f;
        p->size[1].value = (float)(rag_rand(60) + 30) / 10.0f;
        p->size[2].value = 0.2f;
        p->r = 240;
        p->g = 240;
        p->b = 120;
        p->alpha = 0.0f;
        p->alpha_max = 180.0f;
        p->alpha_speed = p->alpha_max / 20.0f;
        p->fade_frame = p->duration - 20;
        RAG_NEW_TEXTURES(p);
        p->textures[0] = RAG_TEXTURE("effect\\ac_center2.tga");
        p->variant = 0;
    }
    rag_str_step(self, 0.0f, -80.0f, 0.0f);
    if (self->frame > 150) {
        render_object_send(self->master, NULL, 0x80, 0, 0, 0, 0);
        return 0;
    }
    return 1;
}

void rag_cast_circle_131(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimNestedSquares *sq;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 131, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    sq = p->records.nested_squares;
    sq[0].active = 0;
    sq[1].active = 0;
    sq[2].active = 0;
    sq[3].active = 0;
    sq[0].active = 1;
    sq[0].frame = 0;
    sq[0].size = 24.0f;
    sq[0].shift_x = 0.0f;
    sq[0].texture = 0;
    sq[0].style = 1;
}

void rag_three_rings_129(struct RagEffect *self, const char *tex, unsigned char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = self->last_prim = rag_launch_prim(self, 129, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        p->records.wide_rings[i].active = 0;
    for (i = 0; i < 3; i++) {
        struct PrimWideRing *ring = &self->last_prim->records.wide_rings[i];

        ring->active = 1;
        ring->frame = 0;
        ring->alpha = 30;
        ring->radius = (float)i * 0.5f + 3.0f;
        ring->base_radius = ring->radius;
        ring->angle = (short)(i * 25);
        ring->outer_centre = self->base.pos;
        self->offset = self->base.pos;
        self->offset.y -= 40.0f;
        ring->inner_centre = self->offset;
        ring->style = style;
    }
}

static void cloud(struct RagEffect *self, const char *tex, signed char style)
{
    struct EffectPrim *p;
    int i, start;

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
    for (i = 0, start = 0; start > -80; i++, start -= 20) {
        struct PrimStandingSquare *sq = &self->last_prim->records.standing_squares[i];

        sq->active = 1;
        sq->frame = start;
        sq->phase = (short)rag_rand(360);
        sq->grow = 10.0f;
        sq->yaw = (short)rag_rand(360);
        sq->alpha = 200;
        sq->drop = 10.0f;
        sq->shade = (float)i;
        sq->style = (float)style;
    }
}

void rag_clouds_textured(struct RagEffect *self, const char *tex, signed char style)
{
    self->base.pos = self->master->pos;
    if (self->frame >= 99999998) {
        self->frame = 0;
        self->duration = 99999999;
        cloud(self, tex, style);
    } else if (self->frame == 0) {
        cloud(self, tex, style);
    }
    if (self->frame == 10)
        cloud(self, tex, style);
}

void rag_electric_arcs(struct RagEffect *self, signed char style, int angle)
{
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if ((unsigned)self->frame > 3)
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
        tube->yaw = (short)(i * 45 + angle);
        tube->reach = 0.0f;
        tube->arc[0] = 0.0f;
        tube->height = (float)rag_rand(11) * 0.01f + 0.0f;
        tube->centre = self->base.pos;
        tube->centre.y -= 5.0f;
        tube->points = 20.0f;
        tube->style = style;
        tube->style_jagged = 3;
    }
}

void rag_torch_flame(struct RagEffect *self, unsigned char flag)
{
    static const char *const frames[13] = {
        "effect\\torch_red01.bmp", "effect\\torch_red02.bmp", "effect\\torch_red03.bmp",
        "effect\\torch_red04.bmp", "effect\\torch_red05.bmp", "effect\\torch_red06.bmp",
        "effect\\torch_red07.bmp", "effect\\torch_red08.bmp", "effect\\torch_red09.bmp",
        "effect\\torch_red10.bmp", "effect\\torch_red11.bmp", "effect\\torch_red12.bmp",
        "effect\\torch_red13.bmp",
    };
    struct EffectPrim *p;
    struct PrimShapeRow *row;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 133, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 13;
    RAG_NEW_TEXTURES(p);
    for (i = 0; i < 13; i++)
        self->last_prim->textures[i] = RAG_TEXTURE(frames[i]);
    p->scale = 0.0f;
    row = p->records.shape_rows;
    row[0].active = 0;
    row[1].active = 0;
    row[2].active = 0;
    row[3].active = 0;
    row[0].frame = 0;
    row[0].active = 1;
    row[0].size = 12.0f;
    row[0].start_y = self->base.pos.y - 12.0f;
    row[0].look = 4;
    row[0].shape = 0.0f;
    row[0].angle = 135;
    row[0].wobble = 0.0f;
    row[0].alpha = 0;
    p->delta = self->offset;
    prim_heading_degrees(self->base.pos.x - self->offset.x, self->base.pos.z - self->offset.z, &row[0].heading);
    row[0].heading += 180;
    if (row[0].heading >= 360)
        row[0].heading -= 360;
    row[0].target.x = self->offset.x;
    row[0].target.z = self->offset.z;
    row[0].target.y = row[0].start_y;
    rag_rand(360);
    row[0].texture = 0;
}

static void three_rings_60(struct RagEffect *self, const char *tex, const float *height, const float *radius,
                           short tilt)
{
    struct EffectPrim *p;
    int i;

    p = self->last_prim = rag_launch_prim(self, 60, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 16.0f;
    for (i = 0; i < 3; i++)
        prim_sub_ring(&p->records.rings[i], 0, height[i], (short)rag_rand(360), radius[i], tilt, 0xc08, 0);
    p->records.rings[3].active = 0;
}

unsigned char rag_spreading_rings(struct RagEffect *self, const char *tex,
                                             unsigned char wide)
{
    static const float radius[3] = { 3.6f, 3.3f, 3.0f };
    static const float narrow[3] = { 1.3f, 1.0f, 0.7f };
    static const float short13[3] = { 13.0f, 14.0f, 15.0f };
    static const float tall24[3] = { 24.0f, 27.0f, 30.0f };
    static const float tall34[3] = { 34.0f, 37.0f, 40.0f };
    static const float tall74[3] = { 74.0f, 77.0f, 80.0f };

    self->base.pos = self->master->pos;
    if (self->frame == 0)
        three_rings_60(self, tex, short13, radius, 25);
    if (wide == 1) {
        if (self->frame == 30) {
            rag_play_wave_here(self, "effect\\EF_GlassWall.wav");
            three_rings_60(self, tex, tall24, radius, 90);
        }
    } else {
        if (self->frame == 30) {
            rag_play_wave_here(self, "effect\\EF_GlassWall.wav");
            three_rings_60(self, tex, tall34, radius, 90);
        }
        if (self->frame == 60)
            three_rings_60(self, tex, tall74, narrow, 90);
    }
    return self->frame != self->duration;
}

void rag_blur_trail(struct RagEffect *self)
{
    struct World *world = game_current_world();
    struct RenderObject *blur;
    void *mem;

    if (!world || !self->master)
        return;
    self->base.pos.x = 225.0f;
    self->base.pos.z = 255.0f;
    self->base.pos.y = 225.0f;
    render_object_set_argb(self->master, -1, 0xe1, 0xe1, 0xff);
    if (self->frame == 9998)
        self->frame = 0;
    if (self->frame % 10 != 0)
        return;
    if (!object_as_type(self->master, OBJECT_PC))
        return;
    mem = mem_alloc(sizeof(struct BlurPC));
    blur = mem ? (struct RenderObject *)blur_pc_construct(mem, (struct Pc *)self->master) : NULL;
    if (!blur)
        return;
    render_object_send(blur, &self->base, 0, (int)self->base.pos.x, (int)self->base.pos.y,
                        (int)self->base.pos.z, 0);
    effect_add_world_object(world, blur);
}

void rag_white_rings(struct RagEffect *self)
{
    struct EffectPrim *p;
    struct PrimRing *s;

    if (self->frame % 100 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 63, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE("effect\\ring_white.tga");
    p->scale = 16.0f;
    s = p->records.rings;
    prim_sub_ring(&s[0], 0, 6.001f, 0, 0.0f, 2, 100, 1);
    prim_sub_ring(&s[1], -10, 6.001f, 25, 0.0f, 3, 100, 1);
    prim_sub_ring(&s[2], -20, 6.001f, 50, 0.0f, 4, 100, 1);
    s[3].active = 0;
    s[3].age = 0;
    s[3].mode = 1;
}

static float rand_pm20(void)
{
    return (float)(rag_rand(41) - 20);
}

void rag_soul_particles(struct RagEffect *self, unsigned char style)
{
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame == 99999998) {
        self->frame = 0;
        self->duration = 99999999;
    } else if (self->frame != 0) {
        return;
    }
    p = self->last_prim = rag_launch_prim(self, 130, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 0;
    p->list_flags |= 1;
    p->list_flags |= 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE("effect\\whitelight.tga");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimWanderGlows *glows = &self->last_prim->records.wander_glows[i];

        glows->active = (style == 1 && i >= 1) ? 0 : 1;
        glows->size = 2.4f;
        if (style == 1) {
            glows->glow[2].x = 0.0f;
            glows->glow[2].z = 0.0f;
        } else {
            glows->glow[2].x = rand_pm20();
            glows->glow[2].z = rand_pm20();
        }
        glows->glow[2].y = (float)rag_rand(100);
        glows->wander[0].angle = (float)rag_rand(360);
        glows->wander[0].target = (float)rag_rand(360);
        glows->wander[1].angle = (float)rag_rand(360);
        glows->wander[1].target = (float)rag_rand(360);
        glows->alpha[0] = 0;
        glows->flash[0] = 0;
        glows->glow[0].x = rand_pm20();
        glows->glow[0].z = rand_pm20();
        glows->glow[0].y = (float)rag_rand(100);
        glows->wander[2].angle = (float)rag_rand(360);
        glows->wander[2].target = (float)rag_rand(360);
        glows->wander[3].angle = (float)rag_rand(360);
        glows->wander[3].target = (float)rag_rand(360);
        glows->alpha[1] = 0;
        glows->flash[1] = 0;
        glows->glow[3].x = rand_pm20();
        glows->glow[3].z = rand_pm20();
        glows->glow[3].y = (float)rag_rand(100);
        glows->wander[4].angle = (float)rag_rand(360);
        glows->wander[4].target = (float)rag_rand(360);
        glows->wander[5].angle = (float)rag_rand(360);
        glows->wander[5].target = (float)rag_rand(360);
        glows->alpha[2] = 0;
        glows->flash[2] = 0;
        glows->glow[1].x = rand_pm20();
        glows->glow[1].z = rand_pm20();
        glows->glow[1].y = (float)rag_rand(100);
        glows->wander[6].angle = (float)rag_rand(360);
        glows->wander[6].target = (float)rag_rand(360);
        glows->wander[7].angle = (float)rag_rand(360);
        glows->wander[7].target = (float)rag_rand(360);
        glows->alpha[3] = 0;
        glows->flash[3] = 0;
        glows->glow[2].y = 0.0f;
        glows->glow[0].y = 0.0f;
        glows->glow[3].y = 0.0f;
        glows->glow[1].y = 0.0f;
        glows->mode = style;
        glows->style = style == 2 ? 10 : 8;
    }
}

void rag_ground_ring_147(struct RagEffect *self, const char *tex, short style)
{
    struct EffectPrim *p;
    struct PrimHalo *halo;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 147, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    halo = p->records.halos;
    halo[0].active = 0;
    halo[1].active = 0;
    halo[2].active = 0;
    halo[3].active = 0;
    halo[0].active = 1;
    halo[0].age = 0;
    if (style == 1) {
        halo[0].size = 21.0f;
        halo[0].style = 6;
    } else if (style == 2) {
        halo[0].size = 21.0f;
        halo[0].style = 7;
    } else {
        halo[0].size = 13.0f;
        halo[0].style = 5;
    }
    halo[0].shift = 0.0f;
    halo[0].texture = 0;
}

void rag_scan_arcs(struct RagEffect *self, short style)
{
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 135, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE("effect\\elec1.tga");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        p->records.swing_arcs[i].active = 0;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimSwingArc *arc = &self->last_prim->records.swing_arcs[i];

        arc->active = 1;
        arc->frame = 0;
        arc->width = 0.1f;
        arc->alpha = i == 0 ? 250 : (short)(140 - i * 30);
        arc->heading = (short)(int)(self->master->roty - 90.0f);
        if (arc->heading < 0)
            arc->heading += 360;
        arc->size = 20.0f;
        arc->points[0] = 0.0f;
        arc->height_scale = (float)rag_rand(11) * 0.01f + 0.0f;
        arc->pos = self->base.pos;
        arc->pos.y -= 10.0f;
        arc->count = 20.0f;
        arc->marks[0] = 0;
        arc->clockwise = style;
        arc->style = 0;
    }
}

void rag_ring_burst(struct RagEffect *self, const char *tex, signed char style)
{
    struct EffectPrim *p;
    int i;

    if (self->frame != 0)
        return;
    self->base.pos = self->master->pos;
    p = self->last_prim = rag_launch_prim(self, 137, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    if (style <= 1) {
        p->scale = 4.0f;
    } else if (style == 2) {
        p->scale = 7.0f;
    } else if (style == 3) {
        p->scale = 6.0f;
    } else if (style == 10) {
        self->last_prim->scale = (float)(rand() % 5 + 4);
    } else if (style == 11) {
        p->scale = 5.0f;
    } else if (style == 12) {
        short pick = (short)(rand() % 3);

        if (pick == 0)
            self->last_prim->scale = 4.0f;
        else if (pick == 1)
            self->last_prim->scale = 40.0f;
        else
            self->last_prim->scale = 41.0f;
    }
    for (i = 0; i < PRIM_RECORD_COUNT; i++)
        self->last_prim->records.crown_rings[i].active = 0;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimCrownRing *crown = &self->last_prim->records.crown_rings[i];
        float scatter;
        int r, deg;

        crown->active = 1;
        crown->arc = 360;
        crown->arc_start = (short)(i * 90);
        if (style == 11) {
            crown->alpha = 50;
            crown->base_height = (float)(i * 20) + 60.0f;
        } else if (style == 12) {
            crown->alpha = 130;
            crown->base_height = (float)(i * 30) + 30.0f;
        } else {
            crown->alpha = 150;
            crown->base_height = 120.0f;
        }
        crown->pos.y = 0.0f;
        r = rand();
        if (style == 10) {
            crown->frame = r % 100;
            scatter = (float)(rand() % 41);
        } else if (style == 12) {
            crown->frame = r % 50 + 50;
            scatter = (float)(rand() % 17);
        } else {
            crown->frame = r % 40 + 60;
            if (style == 11)
                scatter = (float)(rand() % 31) * 0.1f + 6.0f;
            else
                scatter = 5.0f;
        }
        deg = (short)rag_rand(360);
        crown->pos.x = g_cosTable[deg] * scatter;
        crown->pos.z = g_sinTable[deg] * scatter;
        crown->radius = (float)(rand() % 11) * 0.015f + 0.15f;
        crown->tilt = 90;
        crown->offset.x = 0.0f;
        crown->offset.y = 0.0f;
        crown->offset.z = 0.0f;
    }
}

void rag_snowfall(struct RagEffect *self, unsigned char style)
{
    struct EffectPrim *p;
    int i;

    self->base.pos = self->master->pos;
    if ((unsigned)self->frame > 100 || self->frame % 8 != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 59, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 2;
    RAG_NEW_TEXTURES(p);
    self->last_prim->textures[0] = RAG_TEXTURE("effect\\E9_snow03.tga");
    self->last_prim->textures[1] = RAG_TEXTURE("effect\\E9_snow02.tga");
    self->last_prim->scale = 0.0f;
    self->last_prim->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        struct PrimBillboard *bb = &p->records.billboards[i];
        float scatter;
        int deg;

        bb->active = 1;
        bb->age = 0;
        bb->size = (float)rag_rand(6) * 0.1f + 1.5f;
        bb->phase = 0.0f;
        bb->alpha = 0;
        scatter = (float)rag_rand(41);
        deg = (short)rag_rand(360);
        bb->pos.x = self->base.pos.x;
        bb->pos.z = self->base.pos.z;
        bb->pos.x = g_cosTable[deg] * scatter + bb->pos.x;
        bb->pos.z = g_sinTable[deg] * scatter + bb->pos.z;
        bb->pos.y = self->base.pos.y - 6.0f - (float)rag_rand(31);
        bb->heading = 0;
        bb->speed = 0.0f;
        bb->climb = (float)rag_rand(20) * 0.01f + 0.1f;
        bb->spin = 0;
        bb->texture = 0;
        bb->state = 1;
        if (style == 0)
            bb->color_style = 11;
        else if (style == 1)
            bb->color_style = 1;
        else if (style == 2)
            bb->color_style = 1;
        else if (style == 3)
            bb->color_style = 13;
        bb->style = 0.0f;
    }
}

void rag_white_flash_136(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimSpiralColumn *col;

    self->base.pos = self->master->pos;
    if (self->frame != 0)
        return;
    p = self->last_prim = rag_launch_prim(self, 136, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    p->list_flags = 5;
    p->duration = self->duration;
    p->motion_count = 1;
    RAG_NEW_TEXTURES(p);
    p->textures[0] = RAG_TEXTURE(tex);
    p->scale = 0.0f;
    p->base.pos = self->base.pos;
    col = p->records.spiral_columns;
    col[0].active = 0;
    col[1].active = 0;
    col[2].active = 0;
    col[3].active = 0;
    col[0].active = 1;
    col[0].frame = 0;
    col[0].size = 1.0f;
    col[0].shimmer = 0.0f;
    col[0].alpha = 0;
    col[0].pos.x = self->base.pos.x;
    col[0].pos.z = self->base.pos.z;
    col[0].pos.y = self->base.pos.y;
    rag_rand(20);
    col[0].angle = 45;
    col[0].texture = 0;
}

#define UI_ITEM "유저인터페이스\\item\\"

void rag_weapon_toss(struct RagEffect *self)
{
    rag_toss_wave(self);
    rag_tumbling_items(self, UI_ITEM "루드라의활.bmp", 5.0f, 0);
    rag_tumbling_items(self, UI_ITEM "아트로스의흉기.bmp", 5.0f, 0);
    rag_tumbling_items(self, UI_ITEM "모닝스타.bmp", 5.0f, 0);
    rag_tumbling_items(self, UI_ITEM "바스타드소드.bmp", 5.0f, 0);
    rag_tumbling_items(self, UI_ITEM "글레이브.bmp", 5.0f, 0);
    rag_tumbling_items(self, UI_ITEM "블레이드휩.bmp", 5.0f, 0);
    rag_tumbling_items(self, UI_ITEM "드래곤슬레이어.bmp", 5.0f, 0);
    rag_tumbling_items(self, UI_ITEM "배틀액스.bmp", 5.0f, 0);
}
