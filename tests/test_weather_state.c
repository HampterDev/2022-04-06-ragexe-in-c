#include "effects/rag/rag_effect.h"
#include "actor/weather.h"
#include "actor/actor_common.h"
#include "game/context.h"
#include "session/state.h"
#include "world_state_memory.h"
#include <math.h>
#include <stdio.h>

struct Session g_session1;
static int player_present, effect_present, launched[256], launch_count, end_frame;
static int actor;
static struct RagEffect effect;

int rag_effect_get_duration(struct RagEffect *e)
{ assert(e == &effect); return end_frame; }
void rag_effect_set_duration(struct RagEffect *e, int n)
{ assert(e == &effect); end_frame = n; }
struct Player *game_current_player(void)
{ return player_present ? (struct Player *)&actor : NULL; }
struct RagEffect *game_actor_launch_effect(struct GameActor *a, int id, Vec3 offset, float rotation)
{
    assert((void *)a == &actor && !offset.x && !offset.y && !offset.z && rotation == 0);
    assert(launch_count < 256); launched[launch_count++] = id;
    return &effect;
}
unsigned char able_to_make_effect_has_effect(struct GameActor *a, struct RagEffect *e)
{ assert((void *)a == &actor && e == &effect); return (unsigned char)effect_present; }
float get_sin_deg(int n) { return sinf((float)n * 0.017453292519943295f); }
float get_cos_deg(int n) { return cosf((float)n * 0.017453292519943295f); }

static struct String text(const char *s)
{
    struct String out;
    string_init(&out);
    string_assign_cstr(&out, s);
    return out;
}

int main(void)
{
    struct Weather *w = &g_weather;
    struct WeatherScript *row;
    Vec3 sun;
    int i;
    effect.frame = 100;
    end_frame = 1000; effect_present = 1;
    weather_construct(w);
    assert(w->ground_lighting == 1 && w->effect == NULL && w->scripts.first == NULL);
    weather_add_script(w, text("long_map_name_for_owned_weather.rsw"), text("snow"));
    weather_add_script(w, text("long_map_name_for_owned_weather.rsw"), text("sakura"));
    row = w->scripts.first;
    assert(!strcmp(string_cstr(&row[0].map), "long_map_name_for_owned_weather.rsw"));
    g_session1.effectOn = 1;
    weather_script_process(w, text("missing.rsw"));
    assert(launch_count == 0);
    weather_script_process(w, text("long_map_name_for_owned_weather.rsw"));
    assert(launch_count == 0);
    player_present = 1;
    weather_script_process(w, text("long_map_name_for_owned_weather.rsw"));
    assert(launch_count == 2 && launched[0] == 0xa2 && launched[1] == 0xa3);
    weather_stop_effect(w);
    assert(end_frame == 400);
    weather_launch(w, 9);
    assert(launch_count == 2);
    weather_forget_effect(w);
    weather_launch(w, 9);
    assert(launch_count == 3 && launched[2] == 0x3b1);
    effect_present = 0;
    weather_process(w);
    assert(w->effect == NULL);
    w->tint[0] = 0.25f;
    weather_set_dim(w);
    for (i = 0; i < 120; ++i) weather_process(w);
    assert(w->light[0] > 0.49f && w->light[0] <= 0.5f);
    assert(fabsf(w->color[0] - (1.0f - 0.75f * (1.0f - w->light[0]))) < 0.00001f);
    weather_clear_dim(w);
    weather_sun_position(w, &sun);
    assert(fabsf(sun.x) < 0.001f && fabsf(sun.y + 1000.0f) < 0.001f && sun.z == 0);
    weather_destruct(w);
    assert(live_blocks == 0);
    puts("weather state: script ownership, player guards, fading, lighting and typed sun position passed");
    return 0;
}
