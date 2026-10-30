#ifndef RESOURCES_STR_DATA_H
#define RESOURCES_STR_DATA_H

#include "base.h"

#define STR_LAYER_TEXTURES 110
#define STR_LAYERS 60
struct Texture;
struct EzEffectRes;

struct StrKeyState {
    float x, y;
    float u, v, us, vs;
    float second_uv[4];
    float corner_x[4];
    float corner_y[4];
    float aniframe;
    int anitype;
    float delay;
    float angle;
    float color[4];
    int src_blend;
    int dest_blend;
    int mtpreset;
};
SIZE_ASSERT(StrKeyState, 0x74);

enum StrAniType {
    STR_ANI_NONE = 0,
    STR_ANI_STEP = 1,
    STR_ANI_ONCE = 2,
    STR_ANI_LOOP = 3,
    STR_ANI_REVERSE = 4,
    STR_ANI_PINGPONG = 5,
};

struct StrKey {
    int frame;
    int type;
    struct StrKeyState s;
};
SIZE_ASSERT(StrKey, 0x7c);

struct StrLayer {
    int texture_count;
    struct Texture *textures[STR_LAYER_TEXTURES];
    const char *texture_names[STR_LAYER_TEXTURES];
    int key_count;
    const struct StrKey *keys;
};

struct StrAnim {
    int frame_count;
    int layer_count;
    struct StrLayer layers[STR_LAYERS];
};

void *str_layer_get_texture(struct StrLayer *layer, int index);
void *str_layer_get_set_texture(struct StrLayer *layer, int index, const char *folder);

#endif
