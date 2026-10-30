#include "render/rp_pool.h"
#include "../src/effects/skills/gravity_control_effect.c"
#include "effects/render_object.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void mem_free(void *p)
{ abort(); }
void *mem_try_alloc(size_t size)
{ abort(); }
void string_assign(struct String *s, const char *text, size_t len) { abort(); }
void particle_quad_unit_corners(struct TexDrawVertex *quad) { abort(); }
void ez2_str_particle_load(struct Ez2StrParticle *p, const char *name) { abort(); }
void ez2_str_particle_render_current(struct Ez2StrParticle *p) { abort(); }
struct Ez2StrParticle *ez2str_particle_new(Vec3 pos, int list_flags) { return NULL; }

static struct GravityControlEffect effect;
static struct Ez2StrParticle particle;
static struct RenderObject actor;
static int actor_present, ground_present, released, stepped, sounds;
static int ground_token;

void *effect_find_actor(int aid)
{
    assert(aid == 42);
    return actor_present ? &actor : NULL;
}

struct C3dAttr *game_current_map_attr(void)
{
    return ground_present ? (struct C3dAttr *)&ground_token : NULL;
}

float c3d_attr_get_height(struct C3dAttr *ground, float x, float z)
{
    assert(ground == (struct C3dAttr *)&ground_token && x == 3.0f && z == 7.0f);
    return 12.0f;
}

void effect_play_wave_at(const char *wave, const Vec3 *position, float height)
{
    assert(wave && position == &effect.base.pos && height == 0.0f);
    ++sounds;
}

void particle_free(struct Particle *p)
{
    assert(p == &particle.base && !released);
    ++released;
}

unsigned char ez2_str_particle_process(struct Ez2StrParticle *p, Vec3 offset)
{
    assert(p == &particle && offset.x == 0.0f && offset.y == 0.0f && offset.z == 0.0f);
    ++stepped;
    return 1;
}

static void prepare(void)
{
    memset(&effect, 0, sizeof effect);
    memset(&particle, 0, sizeof particle);
    memset(&actor, 0, sizeof actor);
    effect.base.owner_aid = 42;
    effect.particle = &particle;
    effect.lift_limit = -20.0f;
    effect.base.pos.x = 8.0f;
    actor.pos.x = 3.0f;
    actor.pos.y = 99.0f;
    actor.pos.z = 7.0f;
    released = stepped = sounds = 0;
    actor_present = ground_present = 1;
}

int main(void)
{
    int i;
    prepare();
    gravity_control_effect_process(&effect.base);
    assert(actor.pos.y == 11.5f && stepped == 1 && particle.base.age == 1);
    assert(particle.base.pos.x == effect.base.pos.x);
    for (i = 0; i < 50; ++i)
        gravity_control_effect_process(&effect.base);
    assert(actor.pos.y == -8.0f);
    gravity_control_effect_release(&effect.base);
    assert(actor.pos.y == 12.0f && released == 1 && sounds == 1 && !effect.particle);

    prepare();
    actor_present = ground_present = 0;
    gravity_control_effect_process(&effect.base);
    assert(stepped == 0);
    gravity_control_effect_release(&effect.base);
    gravity_control_effect_release(&effect.base);
    assert(released == 1 && sounds == 0 && !effect.particle);

    prepare();
    ground_present = 0;
    gravity_control_effect_release(&effect.base);
    assert(actor.pos.y == 99.0f && released == 1 && sounds == 0);

    prepare();
    effect.particle = NULL;
    gravity_control_effect_process(&effect.base);
    assert(stepped == 0);
    actor_present = 0;
    gravity_control_effect_release(&effect.base);
    assert(released == 0);

    prepare();
    effect.particle = NULL;
    gravity_control_effect_init(&effect.base);
    assert(!effect.particle);
    gravity_control_effect_process(&effect.base);
    assert(stepped == 0);
    puts("gravity control: lift limit, owner loss, missing map and particle release passed");
    return 0;
}
