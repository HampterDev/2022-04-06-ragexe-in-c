#include "resources/animation.h"
#ifndef EFFECTS_RENDER_OBJECT_H
#define EFFECTS_RENDER_OBJECT_H

#include "actor/render_object.h"

#include "effects/effect.h"
#include "actor/render_ops.h"
#include <mmsystem.h>

static __inline float render_object_frames_since(unsigned start_tick)
{
    return (float)(double)(unsigned)(timeGetTime() - start_tick) / 24.0f;
}

#endif
