#include "effects/effect_allocator.h"
#include "platform/win_types.h"
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"
#include "platform/clock.h"

#define EFFECT_NIPELHEIM_CAST 0x81d
#define NIPELHEIM_CAST_TGA "effect\\nipelheim\\nipelheim_cast\\nip_cast.tga"
#define SEGMENTS 21
#define SEGMENT_TURN 0.29919925332069397

struct CylinderEffect {
    struct Effect base;
    unsigned start_ms;
    struct Vec segments;
};

#define SEGMENT_INDEX(p) ((p)->index)

struct CylinderParticle {
    int segment_tilt;
    float segment_height;
    float segment_spin;
};
PARTICLE_EFFECT_DATA_FITS(CylinderParticle);
static __inline struct CylinderParticle *cylinder_particle(const struct Particle *p)
{
    return (struct CylinderParticle *)p->effect_data;
}

struct CylinderShape {
    double radius;
    double tilt;
    double height;
};

struct CylinderEdge {
    Vec3 bottom;
    Vec3 top;
};

struct String;
void cylinder_effect_init(struct Effect *self);
void cylinder_effect_process(struct Effect *self);
void cylinder_effect_render(struct Effect *self);
void cylinder_effect_release(struct Effect *self);
unsigned char cylinder_effect_is_alive(struct Effect *self);
static void cylinder_effect_destroy(struct Effect *effect);

static const struct EffectOps cylinder_effect_ops = {
    .set_submit = effect_set_submit,
    .init = cylinder_effect_init,
    .update_range = effect_update_range,
    .process = cylinder_effect_process,
    .render = cylinder_effect_render,
    .release = cylinder_effect_release,
    .is_alive = cylinder_effect_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = cylinder_effect_destroy,
    .fade_out = effect_fade_out,
};

struct CylinderEffect *cylinder_effect_construct(struct CylinderEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &cylinder_effect_ops;
    self->segments.first = self->segments.last = self->segments.end = 0;
    return self;
}

struct Effect *cylinder_effect_new(void)
{
    struct CylinderEffect *self = mem_try_alloc(sizeof *self);

    return self ? &cylinder_effect_construct(self)->base : NULL;
}

void cylinder_effect_destruct(struct CylinderEffect *self)
{
    self->base.ops = &cylinder_effect_ops;
    vec_free(&self->segments);
    effect_destruct(&self->base);
}

static void cylinder_effect_destroy(struct Effect *effect)
{
    struct CylinderEffect *self = (struct CylinderEffect *)effect;
    cylinder_effect_destruct(self);
    mem_free(self);
}

void cylinder_effect_release(struct Effect *effect)
{
    struct CylinderEffect *self = (struct CylinderEffect *)effect;
    struct Particle **it;
    for (it = self->segments.first; it != (struct Particle **)self->segments.last; it++)
        particle_free(*it);
    self->segments.last = self->segments.first;
}

struct CylinderEdge *cylinder_effect_segment_edge(const struct CylinderShape *shape,
                                                             struct CylinderEdge *out, double angle)
{
    float bottom_x, bottom_z, lean_x, top_y, top_z;
    double lean, lean_2;

    bottom_x = (float)((double)float_cos((float)angle) * shape->radius);
    bottom_z = (float)((double)float_sin((float)angle) * shape->radius);
    lean = shape->height * (double)float_cos((float)shape->tilt);
    lean_x = (float)((double)float_cos((float)angle) * lean);
    top_y = (float)(shape->height * (double)-float_sin((float)shape->tilt)) + 0.0f;
    lean_2 = shape->height * (double)float_cos((float)shape->tilt);
    top_z = (float)((double)float_sin((float)angle) * lean_2) + bottom_z;

    out->bottom.x = bottom_x;
    out->bottom.y = 0.0f;
    out->bottom.z = bottom_z;
    out->top.x = bottom_x + lean_x;
    out->top.y = top_y;
    out->top.z = top_z;
    return out;
}

void cylinder_effect_shape_segment(struct CylinderEffect *self, struct Particle *p)
{
    struct CylinderShape shape;
    struct CylinderEdge left, right;
    double turn, angle;

    if (!p)
        return;
    shape.radius = p->scale;
    shape.tilt = (float)cylinder_particle(p)->segment_tilt / g_halfTurnDegrees * g_pi;
    shape.height = cylinder_particle(p)->segment_height;
    turn = p->roll;
    angle = (double)SEGMENT_INDEX(p) * SEGMENT_TURN + turn;
    cylinder_effect_segment_edge(&shape, &left, angle);
    cylinder_effect_segment_edge(&shape, &right, angle + SEGMENT_TURN);
    p->corners[0].x = left.top.x;
    p->corners[0].y = left.top.y;
    p->corners[0].z = left.top.z;
    p->corners[2].x = left.bottom.x;
    p->corners[2].y = left.bottom.y;
    p->corners[2].z = left.bottom.z;
    p->corners[1].x = right.top.x;
    p->corners[1].y = right.top.y;
    p->corners[1].z = right.top.z;
    p->corners[3].x = right.bottom.x;
    p->corners[3].y = right.bottom.y;
    p->corners[3].z = right.bottom.z;
    p->roll = (float)((double)cylinder_particle(p)->segment_spin + turn);
}

void cylinder_effect_add_segments(struct CylinderEffect *self)
{
    int i;

    for (i = 0; i < SEGMENTS; i++) {
        struct Particle *p = particle_new(self->base.pos, 5);
        struct Particle *head = p;

        if (!p)
            continue;
        head->effect_type = self->base.type;
        head->index = i;
        head->group = 0;
        head->owner_tag = 1;
        particle_quad_unit_corners(p->corners);
        p->roll = 0.0f;
        if (self->base.type == EFFECT_NIPELHEIM_CAST) {
            struct String name;
            string_init(&name);
            string_assign(&name, NIPELHEIM_CAST_TGA, sizeof(NIPELHEIM_CAST_TGA) - 1);
            particle_set_texture(p, name);
            p->scale = 4.5f;
            cylinder_particle(p)->segment_tilt = 70;
            cylinder_particle(p)->segment_height = 25.0f;
            cylinder_particle(p)->segment_spin = 0.052359871566295624f;
            p->src_blend = 5;
            p->dest_blend = 6;
        }
        vec_push(&self->segments, &p, sizeof p);
    }
}

void cylinder_effect_init(struct Effect *effect)
{
    struct CylinderEffect *self = (struct CylinderEffect *)effect;
    self->start_ms = platform_ticks_ms();
    cylinder_effect_add_segments(self);
}

unsigned char cylinder_effect_is_alive(struct Effect *effect)
{
    struct CylinderEffect *self = (struct CylinderEffect *)effect;
    int duration = self->base.duration;
    return self->start_ms + (unsigned)duration >= platform_ticks_ms();
}

void cylinder_effect_process(struct Effect *effect)
{
    struct CylinderEffect *self = (struct CylinderEffect *)effect;
    struct Particle **it;

    for (it = self->segments.first; it != (struct Particle **)self->segments.last; it++) {
        struct Particle *p = *it;
        Vec3 here = self->base.pos;
        p->pos.x = here.x;
        p->pos.y = here.y;
        p->pos.z = here.z;
        cylinder_effect_shape_segment(self, p);
    }
}

void cylinder_effect_render_segment(struct CylinderEffect *self, struct Particle *p)
{
    struct ParticleRenderArgs args;
    int i;

    if (!p)
        return;
    i = SEGMENT_INDEX(p);
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)((double)i / 21.0);
    args.u_right = (float)((double)(i + 1) / 21.0);
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void cylinder_effect_render(struct Effect *effect)
{
    struct CylinderEffect *self = (struct CylinderEffect *)effect;
    struct Particle **it;
    for (it = self->segments.first; it != (struct Particle **)self->segments.last; it++)
        cylinder_effect_render_segment(self, *it);
}
