#include "effects/rag/rag_effect.h"
#include "actor/actor_common.h"
#include "session/state.h"
#include "game/context.h"
#include "actor/weather.h"
#include "ui/ui_std.h"

#include <string.h>

#define EFFECT_FADING_FRAMES 300

struct Weather *weather_construct(struct Weather *self)
{
    self->scripts.first = NULL;
    self->scripts.last = NULL;
    self->scripts.end = NULL;
    self->owner = NULL;
    self->light_max[0] = 0;
    self->light_max[1] = 0;
    self->light_max[2] = 0;
    self->dim = 0;
    self->ground_lighting = 1;
    self->flashing = 0;
    self->effect = NULL;
    self->flash_frames = 0;
    self->light[0] = 1.0f;
    self->light[1] = 1.0f;
    self->light[2] = 1.0f;
    self->tint[0] = 0;
    self->tint[1] = 0;
    self->tint[2] = 0;
    self->color[0] = 1.0f;
    self->color[1] = 1.0f;
    self->color[2] = 1.0f;
    return self;
}

void weather_destruct(struct Weather *self)
{
    struct WeatherScript *s;

    for (s = (struct WeatherScript *)self->scripts.first;
         s != (struct WeatherScript *)self->scripts.last; s++) {
        string_free(&s->name);
        string_free(&s->map);
    }
    vec_free(&self->scripts);
}

void weather_add_script(struct Weather *self, struct String map,
                                   struct String name)
{
    struct WeatherScript line, copy;

    string_init(&line.map);
    string_copy(&line.name, &name);
    string_assign(&line.map, string_cstr(&map), map.size);
    string_copy(&copy.map, &line.map);
    string_copy(&copy.name, &line.name);
    vec_push(&self->scripts, &copy, sizeof copy);
    string_free(&line.name);
    string_free(&line.map);
    string_free(&map);
    string_free(&name);
}

void weather_add_etc_script(struct String map, struct String effect)
{
    weather_add_script(&g_weather, map, effect);
}

Vec3 *weather_sun_position(struct Weather *self, Vec3 *out)
{
    float s, c;

    out->x = 0;
    out->y = 0;
    out->z = 0;
    s = get_sin_deg((self->flash_frames >> 3) + 90);
    c = get_cos_deg((self->flash_frames >> 3) + 90);
    out->z = 0;
    out->x = c * 1000.0f + s * 0.0f;
    out->y = c * 0.0f - s * 1000.0f;
    return out;
}

static int effect_is_fading(struct RagEffect *e)
{
    return e && rag_effect_get_duration(e) - e->frame <= EFFECT_FADING_FRAMES;
}

static struct RagEffect *launch_on_player(struct Weather *self, int effect_id)
{
    Vec3 zero = { 0, 0, 0 };

    self->owner = (struct GameActor *)game_current_player();
    if (!self->owner)
        return NULL;
    return game_actor_launch_effect(self->owner, effect_id, zero, 0);
}

static void launch_pokjuk(struct Weather *self)
{
    Vec3 zero = { 0, 0, 0 };
    int i;

    launch_on_player(self, 0x12d);
    if (!self->owner)
        return;
    for (i = 0; i < 160; i++)
        game_actor_launch_effect(self->owner, 0x129, zero, 0);
}

void weather_launch(struct Weather *self, unsigned char kind)
{
    Vec3 zero = { 0, 0, 0 };
    int effect_id;

    if (effect_is_fading(self->effect))
        return;
    self->owner = (struct GameActor *)game_current_player();
    if (!self->owner)
        return;
    switch (kind) {
    case 0:  effect_id = 0xe5; break;
    case 1:  effect_id = 0xe6; break;
    case 2:  effect_id = 0xe9; break;
    case 3:  effect_id = 0x203; break;
    case 4:  effect_id = 0x204; break;
    case 5:  effect_id = 0x250; break;
    case 7:  effect_id = 0x2b9; break;
    case 8:  effect_id = 0x2ba; break;
    case 9:  effect_id = 0x3b1; break;
    case 10: effect_id = 0x415; break;
    case 11: effect_id = 0x45b; break;
    case 15: effect_id = 0x479; break;
    default: return;
    }
    self->effect = game_actor_launch_effect(self->owner, effect_id, zero, 0);
}

void weather_set_dim(struct Weather *self)
{
    self->dim = 1;
}

void weather_snow(struct Weather *self)
{
    if (effect_is_fading(self->effect))
        return;
    self->effect = launch_on_player(self, 0xa2);
}

void weather_process(struct Weather *self)
{
    int i;

    if (self->dim == 1) {
        if (self->light[0] > 0.5f)
            self->light[0] -= 0.005f;
        if (self->light[1] > 0.5f)
            self->light[1] -= 0.005f;
    } else if (self->flashing == 1) {
        self->flash_frames++;
        if (self->flash_frames > 1000)
            self->flashing = 0;
        if ((self->flash_frames / 3) & 1) {
            self->light[0] = 1.0f;
            self->light[1] = 1.0f;
        } else {
            self->light[0] = 0.2f;
            self->light[1] = 0.2f;
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (self->light[i] < 1.0f && self->light[i] < self->light_max[i])
                self->light[i] += 0.005f;
        }
    }
    if (self->effect && !able_to_make_effect_has_effect(self->owner, self->effect))
        self->effect = NULL;
    for (i = 0; i < 3; i++)
        self->color[i] = 1.0f - (1.0f - self->tint[i]) * (1.0f - self->light[i]);
}

void weather_forget_effect(struct Weather *self)
{
    self->effect = NULL;
}

void weather_script_process(struct Weather *self, struct String map)
{
    struct WeatherScript *line;

    if (g_session1.effectOn != 0) {
        for (line = (struct WeatherScript *)self->scripts.first;
             line != (struct WeatherScript *)self->scripts.last; line++) {
            if (!string_equal(&line->map, &map))
                continue;
            if (string_equal_cstr(&line->name, "snow")) {
                if (!effect_is_fading(self->effect))
                    self->effect = launch_on_player(self, 0xa2);
            } else if (string_equal_cstr(&line->name, "sakura")) {
                if (!effect_is_fading(self->effect))
                    self->effect = launch_on_player(self, 0xa3);
            } else if (string_equal_cstr(&line->name, "pokjuk")) {
                launch_pokjuk(self);
            }
        }
    }
    string_free(&map);
}

void weather_stop_effect(struct Weather *self)
{
    if (self->effect)
        rag_effect_set_duration(self->effect, self->effect->frame + EFFECT_FADING_FRAMES);
}

void weather_clear_dim(struct Weather *self)
{
    self->dim = 0;
}
