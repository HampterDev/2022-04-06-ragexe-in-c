#include "actor/render_ops.h"
#include "actor/object_ops.h"
#include <stdint.h>
#ifndef UI_MISC_ROLLING_CUTTER_H
#define UI_MISC_ROLLING_CUTTER_H

struct RollingCutterRagEffect {
    struct RagEffect base;
    int spin_frames;
    int frames;
};

struct RollingCutterRagEffect *rolling_cutter_rag_effect_construct(struct RollingCutterRagEffect *self);

int game_actor_get_counter(struct GameActor *self, int key);
void game_actor_set_counter(struct GameActor *self, int key, int64_t value);

#endif
