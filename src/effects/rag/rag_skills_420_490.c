#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "skills/skill_info.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/actor_common.h"

#define EFFECT_DIR "이팩트\\"
#define ITEM_DIR "유저인터페이스\\item\\"

static struct Texture *texture(const char *name)
{
    return texture_cache_get(&g_textureCache, name, 0, 0);
}

static struct EffectPrim *launch_prim_in(struct RagEffect *self, int kind, int list_flags, int motions)
{
    struct EffectPrim *p = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    self->last_prim = p;
    p->list_flags = list_flags;
    p->duration = self->duration;
    if (motions != 0)
        p->motion_count = motions;
    p->textures = rag_prim_new_textures(p);
    return p;
}

static struct EffectPrim *launch_prim(struct RagEffect *self, int kind, int motions)
{
    return launch_prim_in(self, kind, 5, motions);
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

static void white_streak_tail(struct RagEffect *self, struct PrimArcTube *tube, float height, float points, char look)
{
    int k;

    tube->height = height;
    tube->centre.x = self->base.pos.x;
    tube->centre.y = self->base.pos.y;
    tube->centre.z = self->base.pos.z;
    tube->points = points;
    for (k = 20; k >= 1; k--)
        tube->arc[k] = 0.0f;
    tube->style = look;
}

void rag_white_streaks_burst(struct RagEffect *self, int look)
{
    struct EffectPrim *p;
    struct PrimArcTube *tube;
    int first, end, i, step, k, n;

    follow_master(self);
    if ((char)look == 1) {
        first = 0;
        end = 2;
    } else if ((char)look == 3) {
        first = 0;
        end = 5;
    } else {
        first = look;
        end = look;
    }
    if (self->frame < first || self->frame >= end)
        return;
    p = launch_prim(self, 100, 1);
    if (!p)
        return;
    p->textures[0] = texture("effect\\white02.bmp");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        tube = &p->records.arc_tubes[i];
        tube->active = 1;
        if ((char)look == 1)
            tube->frame = -rand_below(11);
        else if ((char)look == 3)
            tube->frame = -30 - rand_below(41);
        else
            tube->frame = -rand_below(26);
        if ((char)look == 3) {
            tube->radius = 0.07f;
            tube->alpha = (short)(rand_below(121) + 125);
        } else {
            tube->radius = 0.1f;
            tube->alpha = (short)(rand_below((char)look >= 3 ? 121 : 51) + 125);
        }
        tube->yaw = (short)rand_below(360);
        if ((char)look == 1)
            tube->reach = (float)rand_below(5) + 5.0f;
        else
            tube->reach = (float)rand_below(6) + 6.0f;
        tube->arc[0] = 0.0f;
        white_streak_tail(self, tube, (float)rand_below(7) * 0.1f + 0.6f,
                          (char)look == 1 ? 3.0f : (char)look == 3 ? 4.0f : 5.0f, (char)look);
        if ((char)look != 1)
            continue;
        for (step = 10; step != 0; step--) {
            tube->arc[0] += 6.0f;
            if (tube->arc[0] > 180.0f)
                tube->arc[0] = 180.0f;
            n = (int)tube->points;
            for (k = n; k >= 1; k--)
                tube->arc[k] = tube->arc[k - 1];
        }
    }
}

void rag_white_streaks(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    struct PrimArcTube *tube;
    int i;

    follow_master(self);
    if ((unsigned)(self->frame - 6) > 3)
        return;
    p = launch_prim(self, 100, 1);
    if (!p)
        return;
    p->textures[0] = texture("effect\\white02.bmp");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        tube = &p->records.arc_tubes[i];
        tube->active = 1;
        tube->frame = -rand_below(31);
        tube->radius = look == 3 ? 0.05f : 0.1f;
        tube->alpha = (short)(rand_below(121) + 125);
        tube->yaw = (short)rand_below(360);
        tube->reach = (float)rand_below(6) + 6.0f;
        tube->arc[0] = 0.0f;
        white_streak_tail(self, tube, (float)rand_below(7) * 0.1f + 0.8f, 4.0f, look);
    }
}

void rag_white_streaks_back(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    struct PrimArcTube *tube;
    int i;

    follow_master(self);
    if ((unsigned)self->frame > 2)
        return;
    p = launch_prim(self, 100, 1);
    if (!p)
        return;
    p->textures[0] = texture("effect\\white02.bmp");
    p->scale = 0.0f;
    p->delta = self->offset;
    p->base.pos = self->base.pos;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        tube = &p->records.arc_tubes[i];
        tube->active = 1;
        if (look == 3) {
            tube->frame = -25 - rand_below(21);
            tube->radius = 0.06f;
        } else {
            tube->frame = -15 - rand_below(26);
            tube->radius = 0.07f;
        }
        tube->alpha = (short)(rand_below(121) + 125);
        prim_heading_degrees(self->base.pos.x - self->offset.x, self->base.pos.z - self->offset.z, &tube->yaw);
        tube->yaw += (short)(rand_below(121) - 60);
        if (tube->yaw >= 360)
            tube->yaw -= 360;
        if (tube->yaw < 0)
            tube->yaw += 360;
        tube->reach = (float)rand_below(7) + 6.0f;
        tube->arc[0] = 0.0f;
        white_streak_tail(self, tube, (float)rand_below(7) * 0.1f + 0.8f, 3.0f, look);
    }
}

void rag_electric_rings(struct RagEffect *self, char look, int angle)
{
    struct EffectPrim *p;
    struct PrimArcTube *tube;
    int i;

    follow_master(self);
    if ((unsigned)(self->frame - 20) > 3)
        return;
    p = launch_prim(self, 101, 1);
    if (!p)
        return;
    p->textures[0] = texture("effect\\elec1.tga");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        tube = &p->records.arc_tubes[i];
        tube->active = 1;
        tube->frame = 0;
        tube->radius = 0.1f;
        tube->alpha = 0;
        tube->yaw = (short)(i * 45 + angle);
        tube->reach = 0.0f;
        tube->arc[0] = 0.0f;
        white_streak_tail(self, tube, (float)rand_below(11) * 0.01f + 0.0f, 20.0f, look);
        tube->style_jagged = 0;
    }
}

void rag_electric_lines(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    struct PrimSwingTube *tube;
    int i, k;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 102, 1);
    if (!p)
        return;
    p->textures[0] = texture("effect\\elec1.tga");
    p->scale = 0.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        tube = &p->records.swing_tubes[i];
        tube->active = 1;
        tube->frame = 0;
        tube->radius = 0.1f;
        tube->alpha = 0;
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &tube->yaw);
        tube->reach = 0.0f;
        tube->arc[0] = (float)(i >= 2 ? i * 180 - 360 : i * 180);
        tube->height = (float)rand_below(6) * 0.01f + 0.2f;
        tube->centre.x = self->base.pos.x;
        tube->centre.y = self->base.pos.y;
        tube->centre.z = self->base.pos.z;
        tube->points = 20.0f;
        for (k = 20; k >= 1; k--)
            tube->arc[k] = 0.0f;
    }
}

void rag_storm_ring(struct RagEffect *self, const char *tex, char look)
{
    struct EffectPrim *p;
    struct PrimSquareStack *st;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 103, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    st = &p->records.square_stacks[0];
    st->active = 1;
    st->age = 0;
    st->radius = 40.0f;
    st->alpha = 0;
    st->turn = (short)rand_below(360);
    st->shrink = 2.0f;
    st->top = 50.0f;
    st->y_step = 5.0f;
    st->layers = 10;
    st->style = look;
    st->fade = 0;
    if (look == 10) {
        st->radius = 14.0f;
        st->top = 70.0f;
        st->layers = 14;
        st->shrink = 0.0f;
        st->fade = 1;
    } else if (look == 2) {
        st->fade = 10;
    }
}

void rag_single_ring(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimEnergyArc *ea;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim_in(self, 104, 0x201, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 0.0f;
    p->records.energy_arcs[3].active = 0;
    p->records.energy_arcs[2].active = 0;
    p->records.energy_arcs[1].active = 0;
    ea = &p->records.energy_arcs[0];
    ea->active = 1;
    ea->age = 0;
    ea->drop = 8.0f;
    ea->alpha = 0;
    ea->radius = 8.0f;
    ea->arc = 180;
}

void rag_wind_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimEnergyArc *ea;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim_in(self, 105, 0x201, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 100.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ea = &p->records.energy_arcs[i];
        ea->active = 1;
        ea->age = 0;
        ea->drop = 8.0f;
        ea->alpha = 0;
        ea->radius = 8.0f;
        ea->arc = 360;
        ea->width = 20.0f;
        ea->start = (short)(i * 45);
    }
}

void rag_cloud_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;
    struct PrimEnergyArc *ea;
    int i;

    follow_master(self);
    if (self->frame != 0 && self->frame != 4)
        return;
    p = launch_prim(self, 106, 1);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = 4.0f;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ea = &p->records.energy_arcs[i];
        ea->active = 1;
        ea->age = 0;
        ea->drop = 8.0f;
        ea->alpha = 0;
        ea->radius = 8.0f;
        ea->arc = 360;
        ea->width = 10.0f;
        ea->start = (short)(i * 45);
        ea->tilt = (float)(i * 22);
    }
}

void rag_line_bursts(struct RagEffect *self, short style)
{
    struct EffectPrim *p;
    struct PrimSlideStripe *st;
    int side, i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim_in(self, 56, 0x201, 0);
    if (!p)
        return;
    p->textures[0] = texture(style < 10 ? "effect\\line3.tga" : "effect\\line3y.tga");
    side = style % 10;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        st = &p->records.slide_stripes[i];
        st->active = 1;
        st->age = 0;
        st->back = 100.0f;
        if (side == 0)
            st->side_shift = -15.0f - (float)rand_below(11);
        else if (side == 1)
            st->side_shift = (float)rand_below(11) + 15.0f;
        st->start = (float)rand_below(200);
        if (style < 10) {
            st->half_width = (float)rand_below(4) * 0.1f + 0.3f;
            st->length = (float)rand_below(31) + 20.0f;
            st->color = (unsigned char)rand_below(3);
            rand_below(21);
        } else if (style < 20) {
            st->half_width = (float)rand_below(6) * 0.1f + 0.5f;
            st->length = (float)rand_below(31) + 40.0f;
            st->color = (unsigned char)(rand_below(3) + 10);
            rand_below(11);
        } else {
            st->half_width = (float)rand_below(4) * 0.1f + 0.4f;
            st->length = (float)rand_below(31) + 30.0f;
            st->color = (unsigned char)(rand_below(3) + 20);
            rand_below(21);
        }
        prim_heading_degrees(self->offset.x - self->base.pos.x, self->offset.z - self->base.pos.z, &st->heading);
        st->alpha = 0;
    }
}

void rag_blue_casting(struct RagEffect *self)
{
    follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave(self, "effect\\EF_BeginSpell.wav", -100.0f);
    rag_casting_circle(self, 45, "effect\\ring_blue.tga", 5);
    rag_casting_circle(self, 25, "effect\\ring_blue.tga", 5);
}

unsigned char rag_blue_magic_rings(struct RagEffect *self, int angle)
{
    struct EffectPrim *p;
    struct PrimBladeFan *fan;
    int i;

    follow_master(self);
    if (self->frame == 0) {
        p = launch_prim(self, 75, 0);
        if (!p)
            return 0;
        p->textures[0] = texture("effect\\magic_blue.tga");
        p->scale = 4.0f;
        for (i = 0; i < PRIM_RECORD_COUNT; i++) {
            fan = &p->records.blade_fans[i];
            fan->active = 1;
            fan->age = 0;
            fan->length = 2.0f;
            fan->inner = 1.0f;
            fan->height = 0.0f;
            fan->turn = (short)(i * 90 + angle);
            fan->alpha = 0;
            fan->style = 1;
        }
    }
    return 1;
}

void rag_stop_sprite(struct RagEffect *self, char look)
{
    struct EffectPrim *p;
    struct PrimSpriteMover *mv;

    follow_master(self);
    if (self->frame != 0)
        return;
    follow_master(self);
    p = rag_launch_prim(self, 51, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->list_flags |= 0x200;
    p->duration = self->duration;
    mv = &p->records.sprite_mover;
    mv->active = 1;
    mv->age = 0;
    mv->speed = 0.0f;
    p->scale = 1.0f;
    mv->pos.x = self->base.pos.x;
    mv->pos.y = self->base.pos.y;
    mv->pos.z = self->base.pos.z;
    if (look != 2)
        mv->pos.y -= 7.0f;
    mv->follow_master = 1;
    if (look == 2) {
        p->alpha = 100.0f;
        render_object_load_spr_res(&p->base, EFFECT_DIR "cconfine.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "cconfine.act");
        p->motion_period = 12;
        p->loop_motion = 0;
        p->pattern = PRIM_PATTERN_HOLD_MOTION;
    } else {
        p->alpha = 200.0f;
        render_object_load_spr_res(&p->base, EFFECT_DIR "스톱.spr");
        render_object_load_act_res(&p->base, EFFECT_DIR "스톱.act");
        p->motion_period = 4;
        render_object_set_action_base(&p->base, 0, 0);
    }
    prim_sync_particles(p);
}

void rag_black_casting(struct RagEffect *self, char silent)
{
    follow_master(self);
    if (self->frame != 0)
        return;
    if (silent == 0)
        rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
    rag_casting_circle(self, 45, "effect\\ring_black.tga", 6);
    rag_casting_circle(self, 25, "effect\\ring_black.tga", 6);
}

void rag_rising_light(struct RagEffect *self, char look)
{
    float view[12];
    struct EffectPrim *p;
    struct TexDrawVertex screen;
    const float *m;
    float d;

    if (!self->master || !game_copy_view_matrix(view))
        return;
    prim_project_vertex(&screen, view, self->master->pos);
    self->screen_pos.x = screen.x;
    self->screen_pos.y = screen.y;
    follow_master(self);
    if (self->frame == 0)
        rag_play_wave_here(self, look == 3 ? "effect\\소울 체인지.wav"
                                           : "effect\\EF_IncAgility.wav");
    if (self->frame & 1)
        return;
    p = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;
    self->last_prim = p;
    p->duration = 50;
    mat_set_rotate_y(p->matrix, deg_to_rad(rand_below(360)));
    d = (float)(rand_below(7) + 2);
    m = p->matrix;
    p->delta2.x = m[3] * 0.0f + m[0] * 0.0f + m[6] * d + m[9];
    p->delta2.y = m[4] * 0.0f + m[1] * 0.0f + m[7] * d + m[10];
    p->delta2.z = m[5] * 0.0f + m[2] * 0.0f + m[8] * d + m[11];
    mat_set_rotate_x(p->matrix, deg_to_rad(90));
    if (look >= 2) {
        p->speed = (float)(rand_below(50) + 40) / 100.0f;
        p->size[1].value = (float)(rand_below(100) + 40) / 10.0f;
        p->size[2].value = (float)rand_below(6) * 0.05f + 0.1f;
    } else {
        p->speed = (float)(rand_below(50) + 20) / 100.0f;
        p->size[1].value = (float)(rand_below(100) + 20) / 10.0f;
        p->size[2].value = (float)rand_below(4) * 0.1f + 0.1f;
    }
    p->alpha = 0.0f;
    p->alpha_max = 60.0f;
    p->alpha_speed = p->alpha_max / 20.0f;
    p->fade_frame = p->duration - 20;
    p->textures = rag_prim_new_textures(p);
    p->textures[0] = texture("effect\\ac_center2.tga");
}

void rag_picture_or_skill_icon(struct RagEffect *self, const char *tex, short start,
                                          short look)
{
    struct EffectPrim *p;
    struct PrimHalo *halo;
    char name[64];
    int k;

    follow_master(self);
    if (self->frame != start)
        return;
    p = launch_prim(self, 68, 1);
    if (!p)
        return;
    if (look == 2) {
        sprintf(name, ITEM_DIR "%s.bmp", skill_get_id_name(2285));
        p->textures[0] = texture(name);
    } else if (start > 0) {
        sprintf(name, ITEM_DIR "%s.bmp", skill_get_id_name(rand() % 2 == 0 ? rand() % 12 + 9 : rand() % 13 + 80));
        p->textures[0] = texture(name);
    } else {
        p->textures[0] = texture(tex);
    }
    p->scale = 0.0f;
    halo = p->records.halos;
    halo->active = 1;
    halo->age = 0;
    if (start > 0) {
        halo->size = 5.0f;
        halo->shift = 1.0f;
        halo->style = 4;
    } else {
        halo->size = look == 3 ? 7.0f : 18.0f;
        halo->shift = 0.0f;
        halo->style = 1;
    }
    halo->texture = 0;
    for (k = 0; k < HALO_LAYERS; k++)
        halo->layer_alpha[k] = 0.0f;
}

static void dark_ring(struct EffectPrim *p, int i, short turn, float radius, short tilt)
{
    struct PrimRing *ring = &p->records.rings[i];

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

void rag_dark_rings(struct RagEffect *self, const char *tex)
{
    struct EffectPrim *p;

    follow_master(self);
    if (self->frame == 1) {
        p = launch_prim(self, 107, 0);
        if (!p)
            return;
        p->textures[0] = texture(tex);
        p->scale = 12.0f;
        dark_ring(p, 0, 270, 2.5f, 53);
        dark_ring(p, 1, 0, 5.0f, 60);
        dark_ring(p, 2, 90, 7.5f, 55);
        dark_ring(p, 3, 180, 10.0f, 50);
    }
    if (self->frame == 2) {
        p = launch_prim(self, 107, 0);
        if (!p)
            return;
        p->textures[0] = texture(tex);
        p->scale = 12.0f;
        dark_ring(p, 0, 271, 2.7f, 52);
        dark_ring(p, 1, 1, 5.2f, 59);
        dark_ring(p, 2, 91, 7.7f, 54);
        dark_ring(p, 3, 181, 10.2f, 49);
    }
}

void rag_flower_rings(struct RagEffect *self, const char *tex, char scale, char look)
{
    static const struct { float radius; short tilt; float height; short turn; } sizes[2][4] = {
        { { 4.5f, 85, 27.0f, 0 }, { 5.0f, 80, 28.0f, 90 }, { 5.5f, 70, 29.0f, 180 }, { 5.0f, 75, 30.0f, 270 } },
        { { 5.5f, 50, 30.0f, 90 }, { 6.0f, 55, 29.0f, 180 }, { 6.5f, 65, 28.0f, 270 }, { 6.0f, 60, 27.0f, 0 } },
    };
    struct EffectPrim *p;
    struct PrimRing *ring;
    int i;

    follow_master(self);
    if (self->frame != 0)
        return;
    p = launch_prim(self, 108, 0);
    if (!p)
        return;
    p->textures[0] = texture(tex);
    p->scale = (float)scale;
    for (i = 0; i < PRIM_RECORD_COUNT; i++) {
        ring = &p->records.rings[i];
        ring->active = 1;
        ring->age = 0;
        ring->span = 315;
        ring->alpha = 0;
        ring->offset_flag = 0.0f;
        if (look == 0 || look == 1) {
            ring->radius = sizes[look][i].radius;
            ring->tilt = sizes[look][i].tilt;
            ring->height = sizes[look][i].height;
            ring->turn = sizes[look][i].turn;
        }
        ring->ripple_start = (float)ring->tilt;
        ring->throb_base = ring->height;
    }
}

void rag_flying_kick(struct RagEffect *self)
{
    if (self->frame == 5) {
        rag_follow_master(self);
        rag_play_wave_here(self, "effect\\T_날라차기.wav"  );
        self->master->state = 0x1b;
        render_object_play_motion_range(self->master, 0x60, 5, 5, 0x60, 5, 1, 25, 0);
    }
    if (self->frame == 35 && self->master->state == 0x1b)
        self->master->state = 2;
}

void rag_flower_rings_ramp(struct RagEffect *self)
{
    struct RenderObject *fx;

    if (self->frame == 5) {
        rag_follow_master(self);
        rag_play_wave_here(self, "effect\\T_안락한마법.wav"  );
    }
    fx = self->master;
    if (!(fx->overlay_flags & 0x800)) {
        fx->overlay_flags += 0x800;
        if (self->frame < 200)
            fx->outline_alpha = (unsigned char)self->frame;
        else if (self->frame < 400)
            fx->outline_alpha = (unsigned char)(400 - self->frame);
    }
    rag_flower_rings(self, "effect\\ring_blue.tga", 4, 0);
    rag_flower_rings(self, "effect\\ring_blue.tga", 4, 1);
}
