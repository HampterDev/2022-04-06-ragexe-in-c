#ifndef EFFECTS_RAG_RAG_EFFECT_TYPES_H
#define EFFECTS_RAG_RAG_EFFECT_TYPES_H

#include "base.h"
#include "actor/types.h"
#include "render/vec3.h"
#include "render/primitive_types.h"
#include "resources/str_data.h"

struct EffectPrim;
struct EzEffectRes;
struct StrAnim;

#define PRIM_TRIANGLES 0x200

#define RAG_STR_LAYERS 128

struct RagEffect {
    struct RenderObject base;
    struct RenderObject *master;
    float msg_params[4];
    float master_roty;
    float pitch;
    Vec3 screen_pos;
    int has_str_offset;
    int aux_flag;
    int effect_id;
    int repeat_count;
    int id_state;
    int hit_count;
    int frame;
    int duration;
    Vec3 offset;
    Vec3 offset2;
    Vec3 head_offset;
    struct List prims;
    struct EzEffectRes *str_res;
    struct StrAnim *str_anim;
    struct StrKeyState str_state[RAG_STR_LAYERS];
    struct PrimTriangle triangles[PRIM_TRIANGLES];
    int str_layers_done;
    int str_layers_to_play;
    unsigned char str_drawn[RAG_STR_LAYERS];
    int str_key_index[RAG_STR_LAYERS];
    int str_loops;
    int str_max_loops;
    struct EffectPrim *last_prim;
    int color_r;
    int color_g;
    int color_b;
    int dest_blend;
    unsigned char always_draw;
    int show_id;
};

#endif
