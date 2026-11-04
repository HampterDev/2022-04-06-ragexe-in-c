#include "actor/weather.h"
#include <string.h>

void weather_take_map_light(const float ambient[3], const float diffuse[3])
{
    memcpy(g_weather.tint, ambient, sizeof g_weather.tint);
    memcpy(g_weather.light, diffuse, sizeof g_weather.light);
    memcpy(g_weather.light_max, diffuse, sizeof g_weather.light_max);
}
