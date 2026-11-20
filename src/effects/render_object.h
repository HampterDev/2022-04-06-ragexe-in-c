#include "resources/animation.h"
#ifndef EFFECTS_RENDER_OBJECT_H
#define EFFECTS_RENDER_OBJECT_H

#include "actors/render_object.h"

#include "effects/effect.h"
#include "actors/render_ops.h"
#include "platform/clock.h"

static __inline float render_object_frames_since(unsigned start_tick)
{
    return (float)(double)(unsigned)(platform_ticks_ms() - start_tick) / 24.0f;
}

#endif
