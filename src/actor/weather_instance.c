#include "actor/weather.h"

/* The client's weather. Apart from actor/weather.c, whose scripts reach the effects and the
   game mode, so that the ground and model drawing that only read it do not. */
struct Weather g_weather;
