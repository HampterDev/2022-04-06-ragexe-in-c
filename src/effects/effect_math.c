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

double effect_screen_heading(Vec3 from, Vec3 to)
{
    float view[12];
    float from_x, from_y, from_rhw = 0.0f, to_x, to_y, to_rhw = 0.0f;
    float dx, dy, deg;
    double heading;

    if (!game_copy_view_matrix(view))
        return 0.0;
    renderer_project_point(g_renderer, &from, view, &from_x, &from_y, &from_rhw);
    renderer_project_point(g_renderer, &to, view, &to_x, &to_y, &to_rhw);

    dx = to_x - from_x;
    dy = to_y - from_y;
    if (dx == 0.0f && dy == 0.0f)
        return 0.0;
    deg = (float)acos(dx / (double)float_sqrt(dy * dy + dx * dx)) / g_pi * g_halfTurnDegrees;
    heading = deg;
    if (from_y > to_y)
        heading = 360.0 - heading;
    return heading;
}
