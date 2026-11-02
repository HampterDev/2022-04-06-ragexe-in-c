#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "effects/effect_math.h"
#include "game/context.h"

float effect_random_tenths(float lo, float hi)
{
    int lo_tenths, random, span;

    if (lo == hi)
        return lo;
    lo_tenths = (int)(lo * 10.0f);
    random = rand();
    span = abs(-lo_tenths - (int)(hi * -10.0f)) + 1;
    return (float)(random % span + lo_tenths) * 0.1f;
}

float effect_heading_deg(float x, float z)
{
    float len = float_sqrt(x * x + z * z);
    float deg = (float)acos((double)(1.0f / len * x)) / g_pi * g_halfTurnDegrees;

    if (0.0f > z)
        return 360.0f - deg;
    return deg;
}
