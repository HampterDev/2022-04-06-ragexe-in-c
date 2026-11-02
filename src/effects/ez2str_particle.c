#include "resources/ezeffect.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "render/render_api.h"
#include "effects/effect.h"
#include "game/context.h"
#include <stdint.h>

static const double k_ez2strPi = 3.141592653589;
static const double k_ez2strRandMax = 32767.0;

void ez2_str_particle_reset(struct Ez2StrParticle *self)
{
    if (self->res)
        res_unlock(&self->res->res);
    particle_reset(&self->base);
    self->res = 0;
    self->anim = 0;
    self->layers_done = 0;
    self->layers_to_play = 0;
    memset(self->drawn, 0, sizeof self->drawn);
    self->frame = 0;
    self->loop_forever = 0;
    memset(self->key_index, 0, sizeof(self->key_index));
    memset(self->state, 0, sizeof(self->state));
    self->min_frames = 0;
    self->stopping = 0;
    self->stop_frame = 0;
    self->loops = 0;
    self->start_delay = 0;
    self->spin_degrees = 0;
    self->zoom = 1.0f;
    self->alpha_scale = 1.0f;
    self->window_id = -1;
    self->anchor_to_window = 0;
    self->visible = 1;
    self->ui_offset.x = self->ui_offset.y = self->ui_offset.z = 0;
    self->screen_dx = 0;
    self->screen_dy = 0;
    self->lift = 0;
    self->scale_by_actor = 0;
    self->flip_v = 0;
    self->flip_h = 0;
}

struct Ez2StrParticle *ez2_str_particle_construct(struct Ez2StrParticle *self,
                                                                    Vec3 pos, int list_flags)
{
    self->res = NULL;
    particle_construct(&self->base, pos, list_flags);
    self->base.pool_kind = PARTICLE_KIND_EZ2STR;
    string_init(&self->path);
    string_init(&self->texture_set);
    self->ui_offset.x = self->ui_offset.y = self->ui_offset.z = 0;
    self->sound_loops = 0;
    ez2_str_particle_reset(self);
    return self;
}

void ez2_str_particle_destruct(struct Ez2StrParticle *self)
{
    if (self->res)
        res_unlock(&self->res->res);
    self->res = NULL;
    self->anim = NULL;
    string_free(&self->texture_set);
    string_free(&self->path);
}

void ez2_str_particle_skip(struct Ez2StrParticle *self, float fraction)
{
    int frames;

    if (!self->res)
        return;
    frames = (int)((float)self->anim->frame_count * fraction);
    if (frames <= 0)
        return;
    do {
        Vec3 offset = { 0.0f, -80.0f, 0.0f };
        ez2_str_particle_process(self, offset);
    } while (--frames);
}

float *effect_random_point_in_disc(float *out, float cx, float cy, int64_t radius)
{
    double turn = rand() / k_ez2strRandMax;
    float dist = float_sqrt((float)(rand() / k_ez2strRandMax));
    float radius_f = (float)(double)radius;
    double distance, theta;

    out[0] = 0;
    out[1] = 0;
    distance = (double)dist * radius_f;
    theta = (turn + turn) * k_ez2strPi;
    out[0] = (float)((double)cx + (double)float_cos((float)theta) * distance);
    out[1] = (float)((double)cy + (double)float_sin((float)theta) * distance);
    return out;
}
