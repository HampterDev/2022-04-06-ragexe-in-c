#ifndef ACTOR_WEATHER_H
#define ACTOR_WEATHER_H

#include "base.h"
#include "render/vec3.h"

struct GameActor;
struct RagEffect;

struct WeatherScript {
    struct String map;
    struct String name;
};

struct Weather {
    unsigned char dim;
    unsigned char ground_lighting;
    unsigned char flashing;
    struct Vec scripts;
    struct GameActor *owner;
    float light[3];
    float light_max[3];
    float tint[3];
    float color[3];
    struct RagEffect *effect;
    int flash_frames;
};

extern struct Weather g_weather;

struct Weather *weather_construct(struct Weather *self);
void weather_destruct(struct Weather *self);
/* g_clientInfoWeatherScript: weather_add_script on g_weather for an etcinfo.txt line. */
void weather_add_etc_script(struct String map, struct String effect);
/* g_worldResLight: a loaded map's ambient light becomes g_weather's tint, its diffuse light
   g_weather's light and maximum light. */
void weather_take_map_light(const float ambient[3], const float diffuse[3]);
void weather_add_script(struct Weather *self, struct String map,
                                   struct String name);
Vec3 *weather_sun_position(struct Weather *self, Vec3 *out);
void weather_launch(struct Weather *self, unsigned char kind);
void weather_process(struct Weather *self);
void weather_script_process(struct Weather *self, struct String map);

void weather_set_dim(struct Weather *self);
void weather_clear_dim(struct Weather *self);
void weather_snow(struct Weather *self);
void weather_forget_effect(struct Weather *self);
void weather_stop_effect(struct Weather *self);

#endif
