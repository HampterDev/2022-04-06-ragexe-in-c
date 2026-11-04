#include "effects/str_animation.h"
#include <math.h>
#include <string.h>

static float wrap_frame(float frame, float period)
{
    float wrapped;

    if (!(period > 0.0f) || !isfinite(frame))
        return 0.0f;
    wrapped = fmodf(frame, period);
    if (wrapped < 0.0f)
        wrapped += period;
    return wrapped < period ? wrapped : 0.0f;
}

void str_key_apply_morph(struct StrKeyState *state, const struct StrKeyState *delta,
                         int texture_count, int64_t elapsed)
{
    float frame, last;
    int i;

    state->x += delta->x;
    state->y += delta->y;
    state->u += delta->u;
    state->v += delta->v;
    state->us += delta->us;
    state->vs += delta->vs;
    state->angle += delta->angle;
    for (i = 0; i < 4; ++i) {
        state->second_uv[i] += delta->second_uv[i];
        state->corner_x[i] += delta->corner_x[i];
        state->corner_y[i] += delta->corner_y[i];
        state->color[i] += delta->color[i];
    }

    switch (delta->anitype) {
    case STR_ANI_STEP:
        state->aniframe += delta->aniframe;
        break;
    case STR_ANI_ONCE:
        frame = state->aniframe + delta->delay;
        last = texture_count > 0 ? (float)(texture_count - 1) : 0.0f;
        if (isnan(frame) || frame < 0.0f || texture_count <= 0)
            frame = 0.0f;
        else if (frame >= (float)texture_count)
            frame = last;
        state->aniframe = frame;
        break;
    case STR_ANI_LOOP:
        state->aniframe = wrap_frame(state->aniframe + delta->delay, (float)texture_count);
        break;
    case STR_ANI_REVERSE:
        state->aniframe = wrap_frame(state->aniframe - delta->delay, (float)texture_count);
        break;
    case STR_ANI_PINGPONG:
        if (texture_count <= 1) {
            state->aniframe = 0.0f;
            break;
        }
        last = (float)(texture_count - 1);
        frame = truncf((float)elapsed * delta->delay + state->aniframe);
        frame = wrap_frame(frame, 2.0f * last);
        state->aniframe = frame > last ? 2.0f * last - frame : frame;
        break;
    }
}

int str_animation_rewind(const struct StrAnim *anim, int key_index[STR_LAYERS],
                         struct StrKeyState state[STR_LAYERS], unsigned char drawn[STR_LAYERS])
{
    int i;
    memset(state, 0, STR_LAYERS * sizeof *state);
    memset(drawn, 0, STR_LAYERS * sizeof *drawn);
    for (i = 0; i < STR_LAYERS; ++i)
        key_index[i] = -1;
    if (!anim || anim->layer_count <= 0 || anim->layer_count > STR_LAYERS)
        return 0;
    for (i = 0; i < anim->layer_count; ++i)
        key_index[i] = 0;
    return anim->layer_count - (anim->layers[0].key_count <= 0);
}

int str_animation_step(const struct StrAnim *anim, int frame, int key_index[STR_LAYERS],
                       struct StrKeyState state[STR_LAYERS], unsigned char drawn[STR_LAYERS])
{
    int i, finished = 0;
    memset(drawn, 0, STR_LAYERS * sizeof *drawn);
    if (!anim || anim->layer_count < 0 || anim->layer_count > STR_LAYERS)
        return 0;
    for (i = 0; i < anim->layer_count; ++i) {
        const struct StrLayer *layer = &anim->layers[i];
        const struct StrKey *key;
        int index = key_index[i];
        int64_t elapsed;
        if (index < 0 || index >= layer->key_count || !layer->keys)
            continue;
        key = &layer->keys[index];
        elapsed = (int64_t)frame - key->frame;
        if (elapsed < 0)
            continue;
        if (elapsed && index < layer->key_count - 1 && key[1].frame == frame) {
            ++key;
            key_index[i] = ++index;
            if (index == layer->key_count - 1) {
                key_index[i] = -1;
                ++finished;
            }
        }
        if (key->type == 0) {
            state[i] = key->s;
            if (key_index[i] >= 0 && index < layer->key_count - 1 && key[1].frame == key->frame)
                key_index[i] = index + 1;
        } else {
            str_key_apply_morph(&state[i], &key->s, layer->texture_count, elapsed);
        }
        drawn[i] = 1;
    }
    return finished;
}
