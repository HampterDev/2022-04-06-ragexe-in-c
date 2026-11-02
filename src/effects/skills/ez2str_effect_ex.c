#include "effects/effect_allocator.h"
#include "actor/actor_ops.h"
#include "render/projection.h"
#include "render/math3d.h"
#include <math.h>
#include <string.h>
#include "effects/skills/ez2str_effect_ex.h"
#include "effects/skills/skill_view.h"
#include "effects/ez2str_effect_mgr.h"
#include "effects/render_object.h"
#include "session/session_skills.h"

struct Effect;

static void ez2_str_effect_ex_destroy(struct Effect *effect);

static const struct EffectOps ez2_str_effect_ex_ops = {
    .set_submit = effect_set_submit,
    .init = ez2_str_effect_ex_init,
    .update_range = effect_update_range,
    .process = ez2_str_effect_ex_process,
    .render = ez2_str_effect_ex_render,
    .release = ez2_str_effect_ex_release,
    .is_alive = ez2_str_effect_ex_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = ez2_str_effect_ex_set_stop_frame,
    .set_start_delay = ez2_str_effect_ex_set_start_delay,
    .set_spin = ez2_str_effect_ex_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = ez2_str_effect_ex_set_zoom,
    .set_alpha_scale = ez2_str_effect_ex_set_alpha_scale,
    .destroy = ez2_str_effect_ex_destroy,
    .fade_out = ez2_str_effect_ex_fade_out,
};

#define EZ2STR_EX_LIST_DEFAULT 5
#define EZ2STR_EX_LIST_FLOOR   0xd
#define EZ2STR_EX_LIST_SCREEN  0x205
#define FADE_STEP              0.05f

#define ACTOR_VISIBLE(a)    ((const struct RenderObject *)(a))->visible
#define ACTOR_REVEALED(a)    ((const struct RenderObject *)(a))->revealed
#define PARTICLE_LIFETIME(p) ((p)->base.life_seconds)
#define SCREEN_OFFSET(p)     ((p)->base.offset)
#define SOUND_LOOPS(p)       ((p)->sound_loops)

struct Ez2StrEffectEx *ez2_str_effect_ex_construct(struct Ez2StrEffectEx *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &ez2_str_effect_ex_ops;
    self->particle = 0;
    self->fading = 0;
    self->start_frame = 0;
    return self;
}

struct Effect *ez2_str_effect_ex_new(void)
{
    struct Ez2StrEffectEx *self = mem_try_alloc(sizeof *self);

    return self ? &ez2_str_effect_ex_construct(self)->base : NULL;
}

void ez2_str_effect_ex_destruct(struct Ez2StrEffectEx *self)
{
    self->base.ops = &ez2_str_effect_ex_ops;
    effect_destruct(&self->base);
}

static void ez2_str_effect_ex_destroy(struct Effect *effect)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    ez2_str_effect_ex_destruct(self);
    mem_free(self);
}

void ez2_str_effect_ex_init(struct Effect *effect)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    struct Ez2StrEffectMgr *mgr = ez2str_effect_mgr_instance();
    const struct EZ2STREntry *row = ez2_str_effect_mgr_find_by_effect_id(mgr, self->base.type);
    struct Ez2StrParticle *p;

    if (!row)
        return;
    p = ez2str_particle_new(self->base.pos, EZ2STR_EX_LIST_DEFAULT);
    if (!p)
        return;
    particle_quad_unit_corners(p->base.corners);
    p->actor_aid = self->base.owner_aid;
    ez2_str_particle_open(p, string_cstr(&row->file_path));
    SCREEN_OFFSET(p).x = row->pos_x;
    SCREEN_OFFSET(p).y = row->pos_y;
    SCREEN_OFFSET(p).z = 0.0f;
    if (row->flags & EZ2STR_EX_INFINITE) {
        self->base.has_instance_key = 1;
        p->loop_forever = 1;
    }
    p->start_delay = row->start_delay;
    self->start_frame = row->start_delay / 16;
    if (row->sound_path.size != 0)
        effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, string_cstr(&row->sound_path),
                             self->start_frame + 1, 0.0f);
    if (row->quake_delay >= 0)
        effect_add_sound_cue(&self->base, SOUND_CUE_GAME_MESSAGE, 0, row->quake_delay, 0.0f);
    if (row->flags & EZ2STR_EX_FLOOR)
        p->base.list_flags = EZ2STR_EX_LIST_FLOOR;
    if (row->flags & EZ2STR_EX_SCREEN_FILL)
        p->base.list_flags = EZ2STR_EX_LIST_SCREEN;
    SOUND_LOOPS(p) = (unsigned char)((row->flags >> 3) & 1);
    self->particle = p;
}

static void place_and_step(struct Ez2StrEffectEx *self, struct Ez2StrParticle *p)
{
    float view[12], from_view[12];
    float x, y, z, a, b, c;
    Vec3 no_offset;

    if (game_copy_view_matrix(view)) {
        mat_invert(from_view, view);

        x = self->base.pos.x;
        y = self->base.pos.y;
        z = self->base.pos.z;
        a = (view[3] * y + view[0] * x) + view[6] * z + view[9] + SCREEN_OFFSET(p).x;
        b = (view[4] * y + view[1] * x) + view[7] * z + view[10] - SCREEN_OFFSET(p).y;
        c = (view[5] * y + view[2] * x) + view[8] * z + view[11];
        p->base.pos.x = (b * from_view[3] + a * from_view[0]) + c * from_view[6] + from_view[9];
        p->base.pos.y = (b * from_view[4] + a * from_view[1]) + from_view[7] * c + from_view[10];
        p->base.pos.z = (from_view[2] * a + from_view[5] * b) + from_view[8] * c + from_view[11];
    }

    no_offset.x = 0.0f;
    no_offset.y = 0.0f;
    no_offset.z = 0.0f;
    ez2_str_particle_process(p, no_offset);
}

void ez2_str_effect_ex_process(struct Effect *effect)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    struct Ez2StrParticle *p = self->particle;

    if (!p)
        return;
    if (self->fading == 1) {
        p->alpha_scale -= FADE_STEP;
        p = self->particle;
    }
    if (p) {
        place_and_step(self, p);
        p = self->particle;
    }
    if (SOUND_LOOPS(p) == 1 && p->frame == 1)
        self->base.frame = self->start_frame;
    p->base.age++;
}

void ez2_str_effect_ex_render(struct Effect *effect)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    unsigned char *actor;

    if (!self->particle)
        return;
    actor = effect_find_actor(self->base.owner_aid);
    if (actor) {
        if (is_effect_state_cloaking(actor_get_effect_state(actor)) == 1)
            return;
        if (!ACTOR_REVEALED(actor))
            return;
        if (!ACTOR_VISIBLE(actor))
            return;
    }
    ez2_str_particle_render_current(self->particle);
}

void ez2_str_effect_ex_release(struct Effect *effect)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    if (self->particle) {
        particle_free(&self->particle->base);
        self->particle = 0;
    }
}

unsigned char ez2_str_effect_ex_is_alive(struct Effect *effect)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    struct Ez2StrParticle *p = self->particle;
    float limit;

    if (!p)
        return 0;
    if (self->fading == 1)
        return p->alpha_scale > 0.0f;
    if (p->loop_forever == 1)
        return effect_is_owner_alive(&self->base);
    limit = PARTICLE_LIFETIME(p);
    if (limit == 0.0f)
        return 1;
    return limit * 60.0f > (float)p->base.age;
}

void ez2_str_effect_ex_set_stop_frame(struct Effect *effect, float frame)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    if (self->particle)
        self->particle->stop_frame = (int)frame;
}

void ez2_str_effect_ex_set_start_delay(struct Effect *effect, int ms)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    if (self->particle)
        self->particle->start_delay = ms;
    self->start_frame = ms / 16;
    effect_set_sound_cue_frame(&self->base, SOUND_CUE_WAVE, self->start_frame + 1);
}

void ez2_str_effect_ex_set_spin(struct Effect *effect, float degrees)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    if (self->particle)
        self->particle->spin_degrees = degrees;
}

void ez2_str_effect_ex_set_zoom(struct Effect *effect, float zoom)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    if (self->particle)
        self->particle->zoom = zoom;
}

void ez2_str_effect_ex_set_alpha_scale(struct Effect *effect, float scale)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    if (self->particle)
        self->particle->alpha_scale = scale;
}

void ez2_str_effect_ex_fade_out(struct Effect *effect)
{
    struct Ez2StrEffectEx *self = (struct Ez2StrEffectEx *)effect;
    self->fading = 1;
}

static float line_angle(const Vec3 *from, const Vec3 *to, const float *view, int *found)
{
    float x1, y1, rhw1 = 0.0f, x2, y2, rhw2 = 0.0f;
    float dx, dy, dist2, degrees;
    double cosine;

    renderer_project_point(g_renderer, from, view, &x1, &y1, &rhw1);
    renderer_project_point(g_renderer, to, view, &x2, &y2, &rhw2);
    dx = x2 - x1;
    dy = y2 - y1;
    dist2 = dy * dy + dx * dx;
    if (float_sqrt(dist2) == 0.0f) {
        *found = 0;
        return 0.0f;
    }
    *found = 1;
    cosine = (double)dx / (double)float_sqrt(dist2);
    degrees = (float)acos(cosine) / g_pi * g_halfTurnDegrees;
    if (y1 > y2)
        return 360.0f - degrees;
    return degrees;
}

float effect_screen_angle(Vec3 from, Vec3 to, const float *view)
{
    int found;
    float angle = line_angle(&from, &to, view, &found);

    return found ? angle : 0.0f;
}

float effect_screen_angle_quadrant(Vec3 from, Vec3 to)
{
    float unturned_view[12];
    float angle = 0.0f, snapped;
    int found;

    if (!game_copy_view_matrix_at(unturned_view, 0.0f))
        return 0.0f;
    angle = line_angle(&from, &to, unturned_view, &found);
    if (!found)
        angle = 0.0f;

    snapped = (float)(((int)(angle + 45.0f) / 90) * 90);
    snapped -= game_camera_heading();
    if (360.0f > snapped)
        snapped += 360.0f;
    return snapped;
}
