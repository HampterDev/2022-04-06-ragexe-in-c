#include "std/swing_pace.h"

float swing_pace_hit_ms(float frame_delay, float hit_frame)
{
    return hit_frame * frame_delay * (float)SWING_PACE_UNIT_MS;
}

float swing_pace_frame_delay(float frame_delay, float hit_frame, int lands_ms)
{
    float slowest, fastest, paced;

    /* written so that a value that is not a number keeps the delay too */
    if (!(frame_delay > 0.0f) || !(hit_frame > 0.0f))
        return frame_delay;
    slowest = frame_delay * (float)SWING_PACE_SLOWEST;
    fastest = frame_delay / (float)SWING_PACE_FASTEST;
    if (lands_ms <= 0)
        return fastest;
    paced = (float)lands_ms / (hit_frame * (float)SWING_PACE_UNIT_MS);
    if (paced > slowest)
        return slowest;
    return paced < fastest ? fastest : paced;
}
