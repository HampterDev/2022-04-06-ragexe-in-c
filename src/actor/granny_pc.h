#ifndef ACTOR_GRANNY_PC_H
#define ACTOR_GRANNY_PC_H

#include "resources/granny_sdk.h"
#include "resources/granny_playback.h"
#include "actor/actor.h"
#include "actor/granny_actor.h"
#include "actor/game_actor.h"
#include <stdint.h>

struct GrannyPc {
    CGAMEACTOR_MEMBERS
    struct String imf_name;
    int honor;
    int virtue;
    int head_dir;
    int head;
    int head_palette;
    int64_t weapon;
    int accessory;
    int accessory2;
    int accessory3;
    int shield;
    struct String head_pal_name;
    struct UIWindow *hp_gage_wnd;
    int pvp_rank;
    int pvp_total;
    struct GrannyModelState *model;
    struct GrannyModelState *parts[10];
    unsigned char clock_now[16];
    unsigned char clock_last[16];
    float anim_time;
    int model_action;
    int model_base_action;
    float facing;
    unsigned char alpha;
    unsigned color;
    int anim;
    char model_name[0x104];
    unsigned char first_frame;
    int draw_phase;
    granny_world_pose *body_pose;
    struct Texture *shadow_tex;
    struct Vec shadow_points;
    struct GrannyHairPivot hair_pivot;
};

granny_world_pose *granny_pc_pose(struct GrannyPc *self, struct GrannyModelState *model);

void granny_pc_free_res(struct GrannyPc *self);
int granny_pc_replace_models(struct GrannyPc *self);
void granny_pc_play_state_anim(struct GrannyPc *self, int loop);

void granny_pc_render_part(struct GrannyPc *self, const float *matrix,
                           const struct GrannyLight *light, struct GrannyModelState *model, int part, struct GrannyModelState *pose_model);
void granny_pc_render_part_kept(struct GrannyPc *self, const float *matrix, int part, struct GrannyModelState *model);
void granny_pc_render_shadow(struct GrannyPc *self, const float *matrix, int reuse);

#endif
