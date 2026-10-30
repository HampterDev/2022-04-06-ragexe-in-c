#ifndef EFFECTS_STR_ANIMATION_H
#define EFFECTS_STR_ANIMATION_H

#include "resources/str_data.h"
#include <stdint.h>

#define STR_SCREEN_CX 320.0f
#define STR_SCREEN_CY 240.0f

int str_animation_rewind(const struct StrAnim *anim, int key_index[STR_LAYERS],
                         struct StrKeyState state[STR_LAYERS], unsigned char drawn[STR_LAYERS]);
int str_animation_step(const struct StrAnim *anim, int frame, int key_index[STR_LAYERS],
                       struct StrKeyState state[STR_LAYERS], unsigned char drawn[STR_LAYERS]);
void str_key_apply_morph(struct StrKeyState *state, const struct StrKeyState *delta,
                         int texture_count, int64_t elapsed);

#endif
