#ifndef ACTOR_OBJECT_OPS_H
#define ACTOR_OBJECT_OPS_H

#include "actor/types.h"

typedef int64_t RenderMsgArg;
#define MSG_PTR(arg) ((void *)(intptr_t)(arg))
#define MSG_ARG_PTR(p) ((RenderMsgArg)(intptr_t)(p))

struct GameObjectOps {
    void (*destroy)(struct GameObject *);
    unsigned char (*on_process)(struct GameObject *);
    void (*send_msg)(struct GameObject *, struct GameObject *sender,
        RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
    void (*render)(struct GameObject *, const float *view);
    int (*get_direction)(struct GameObject *, float roty);
};

extern const struct GameObjectOps game_object_ops;

static __inline const struct GameObjectOps *game_object_ops_of(const struct GameObject *object)
{
    return object->ops;
}

static __inline void game_object_destroy(struct GameObject *object)
{
    game_object_ops_of(object)->destroy(object);
}

static __inline unsigned char game_object_process(struct GameObject *object)
{
    return game_object_ops_of(object)->on_process(object);
}

static __inline void game_object_render(struct GameObject *object, const float *view)
{
    game_object_ops_of(object)->render(object, view);
}

static __inline int game_object_direction(struct GameObject *object, float roty)
{
    return game_object_ops_of(object)->get_direction(object, roty);
}

static __inline void game_object_send(struct GameObject *object, struct GameObject *sender,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    game_object_ops_of(object)->send_msg(object, sender, msg, par1, par2, par3, par4);
}

int game_object_dir_of(struct GameObject *object, float roty);
int game_actor_dir_of(struct GameObject *object, float roty);
unsigned char game_object_abstract_process(struct GameObject *object);

void c3d_granny_game_actor_delete(struct GameObject *object);
void able_to_make_effect_delete(struct GameObject *object);
void arrow_effect_delete(struct GameObject *object);
void blur_pc_delete(struct GameObject *object);
void character_npc_delete(struct GameObject *object);
void corpse_delete(struct GameObject *object);
void dummy_effect_launcher_deleting_destruct(struct GameObject *object);
void effect_maker_delete(struct GameObject *object);
void effect_prim_delete(struct GameObject *object);
void game_actor_delete(struct GameObject *object);
void game_object_delete(struct GameObject *object);
void graffi_skill_delete(struct GameObject *object);
void granny_corpse3d_delete(struct GameObject *object);
void ground_scope_delete(struct GameObject *object);
void hallu_npc_delete(struct GameObject *object);
void hallu_pc2_delete(struct GameObject *object);
void hallu_pc_delete(struct GameObject *object);
void hallu_plant_delete(struct GameObject *object);
void item_deleting_destroy(struct GameObject *object);
void mandra_attack_effect_delete(struct GameObject *object);
void mouse_pointer_delete(struct GameObject *object);
void msg_effect_delete(struct GameObject *object);
void npc_delete(struct GameObject *object);
void pc_deleting_destruct(struct GameObject *object);
void player_deleting_destruct(struct GameObject *object);
void rag_effect_delete(struct GameObject *object);
void render_object_delete(struct GameObject *object);
void skill_delete(struct GameObject *object);
void sound_maker_delete(struct GameObject *object);
unsigned char c3d_granny_game_actor_on_process(struct GameObject *object);
unsigned char arrow_effect_on_process(struct GameObject *object);
unsigned char blur_pc_on_process(struct GameObject *object);
unsigned char corpse_on_process(struct GameObject *object);
unsigned char dummy_effect_launcher_on_process(struct GameObject *object);
unsigned char effect_maker_on_process(struct GameObject *object);
unsigned char effect_prim_on_process(struct GameObject *object);
unsigned char game_object_on_process(struct GameObject *object);
unsigned char graffi_skill_on_process(struct GameObject *object);
unsigned char granny_corpse3d_on_process(struct GameObject *object);
unsigned char ground_scope_on_process(struct GameObject *object);
unsigned char hallu_npc_on_process(struct GameObject *object);
unsigned char hallu_pc2_on_process(struct GameObject *object);
unsigned char hallu_pc_on_process(struct GameObject *object);
unsigned char hallu_plant_on_process(struct GameObject *object);
unsigned char item_on_process(struct GameObject *object);
unsigned char mandra_attack_effect_on_process(struct GameObject *object);
unsigned char mouse_pointer_on_process(struct GameObject *object);
unsigned char msg_effect_on_process(struct GameObject *object);
unsigned char npc_on_process(struct GameObject *object);
unsigned char pc_on_process(struct GameObject *object);
unsigned char player_on_process(struct GameObject *object);
unsigned char rag_effect_on_process(struct GameObject *object);
unsigned char render_object_on_process(struct GameObject *object);
unsigned char skill_on_process(struct GameObject *object);
unsigned char sound_maker_on_process(struct GameObject *object);
void dummy_effect_launcher_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void able_to_make_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void arrow_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void blur_pc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void character_npc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void corpse_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void effect_maker_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void game_actor_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void graffi_skill_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void ground_scope_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void hallu_npc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void hallu_pc2_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void hallu_pc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void hallu_plant_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void item_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void mandra_attack_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void mouse_pointer_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void msg_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void npc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void pc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void player_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void render_object_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void skill_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void sound_maker_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void game_object_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4);
void c3d_granny_game_actor_render(struct GameObject *object, const float *view);
void blur_pc_render(struct GameObject *object, const float *view);
void character_npc_render(struct GameObject *object, const float *view);
void corpse_render(struct GameObject *object, const float *view);
void effect_maker_render(struct GameObject *object, const float *view);
void effect_prim_render(struct GameObject *object, const float *view);
void graffi_skill_render(struct GameObject *object, const float *view);
void granny_corpse3d_render(struct GameObject *object, const float *view);
void ground_scope_render(struct GameObject *object, const float *view);
void hallu_npc_render(struct GameObject *object, const float *view);
void hallu_pc2_render(struct GameObject *object, const float *view);
void hallu_pc_render(struct GameObject *object, const float *view);
void hallu_plant_render(struct GameObject *object, const float *view);
void item_render(struct GameObject *object, const float *view);
void msg_effect_render(struct GameObject *object, const float *view);
void npc_render(struct GameObject *object, const float *view);
void pc_render(struct GameObject *object, const float *view);
void rag_effect_render(struct GameObject *object, const float *view);
void render_object_render(struct GameObject *object, const float *view);
void skill_render(struct GameObject *object, const float *view);
void sound_maker_render(struct GameObject *object, const float *view);
void game_object_render_base(struct GameObject *object, const float *view);

#endif
