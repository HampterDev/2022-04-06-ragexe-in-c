#ifndef ACTOR_ACTOR_OPS_H
#define ACTOR_ACTOR_OPS_H

#include "actor/render_ops.h"

struct ActorLook;
struct ActorHeadExtent { float width, height; };

struct GameActorOps {
    struct RenderObjectOps base;
    unsigned char (*process_state)(struct GameActor *);
    void (*set_state)(struct GameActor *, int, unsigned char);
    void (*set_unit_cell)(struct GameActor *);
    void (*clear_unit_cell)(struct GameActor *);
    void (*set_job_model)(struct GameActor *, int, int);
    void (*set_look)(struct GameActor *, const struct ActorLook *);
    void (*set_look_with_state)(struct GameActor *, const struct ActorLook *, int);
    void (*set_spr_job)(struct GameActor *, int);
    void (*set_head)(struct GameActor *, int);
    void (*set_weapon)(struct GameActor *, int64_t);
    void (*set_weapon_look)(struct GameActor *, int64_t);
    void (*set_accessory)(struct GameActor *, int);
    void (*set_accessory2)(struct GameActor *, int);
    void (*set_accessory3)(struct GameActor *, int);
    void (*set_shield)(struct GameActor *, int);
    void (*set_appearance_value)(struct GameActor *, int);
    void (*set_imf_name)(struct GameActor *);
    void (*set_head_pal_name)(struct GameActor *);
    void (*set_body_pal_name)(struct GameActor *);
    void (*set_body_palette)(struct GameActor *, int);
    void (*set_head_palette)(struct GameActor *, int);
    int64_t (*get_weapon)(struct GameActor *);
    void (*walk_motion)(struct GameActor *);
    int (*get_attack_motion)(struct GameActor *);
    void (*make_corpse)(struct GameActor *);
    void (*set_attack_speed)(struct GameActor *, int);
    void (*set_honor)(struct GameActor *, int);
    void (*set_honor_virtue)(struct GameActor *, int, unsigned int);
    int (*get_char_id)(struct GameActor *);
    void (*set_robe)(struct GameActor *, int, unsigned char);
    void (*set_body_style)(struct GameActor *, int, unsigned char);
    void (*set_mounted)(struct GameActor *, unsigned char);
    int (*get_sprite_variant)(struct GameActor *);
    void (*set_sprite_variant)(struct GameActor *, int);
    unsigned char (*get_skip_arrow)(struct GameActor *);
    void (*set_skip_arrow)(struct GameActor *, unsigned char);
    void (*refresh_accessory3)(struct GameActor *);
    struct ActorHeadExtent (*get_head_extent)(struct GameActor *);
};

extern const struct GameActorOps game_actor_ops;
extern const struct GameActorOps c3d_granny_game_actor_ops;
extern const struct GameActorOps granny_pc_ops;
extern const struct GameActorOps pc_ops;
extern const struct GameActorOps player_ops;
extern const struct GameActorOps npc_ops;
extern const struct GameActorOps blur_pc_ops;
extern const struct GameActorOps hallu_pc_ops;
extern const struct GameActorOps hallu_pc2_ops;
extern const struct GameActorOps hallu_npc_ops;
extern const struct GameActorOps hallu_plant_ops;
extern const struct GameActorOps g_characterNpcVtbl_ops;
extern const struct GameActorOps graffi_skill_ops;
extern const struct GameActorOps skill_ops;

static __inline const struct GameActorOps *actor_ops_of(const void *actor)
{
    return (const struct GameActorOps *)((const struct GameActor *)actor)->ops;
}

static __inline int actor_get_effect_state(void *object)
{
    struct RenderObject *render = object;
    return render_object_ops_of(render)->get_effect_state(render);
}

static __inline unsigned char actor_process_state(void *object)
{
    struct GameActor *actor = object;
    return actor_ops_of(actor)->process_state(actor);
}

static __inline void actor_set_state(void *object, int state, unsigned char snap)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_state(actor, state, snap);
}

static __inline void actor_set_unit_cell(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_unit_cell(actor);
}

static __inline void actor_clear_unit_cell(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->clear_unit_cell(actor);
}

static __inline void actor_set_job_model(void *object, int job, int sex)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_job_model(actor, job, sex);
}

static __inline void actor_set_spr_job(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_spr_job(actor, value);
}

static __inline void actor_set_head(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_head(actor, value);
}

static __inline void actor_set_weapon(void *object, int64_t value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_weapon(actor, value);
}

static __inline void actor_set_weapon_look(void *object, int64_t value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_weapon_look(actor, value);
}

static __inline void actor_set_accessory(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_accessory(actor, value);
}

static __inline void actor_set_accessory2(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_accessory2(actor, value);
}

static __inline void actor_set_accessory3(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_accessory3(actor, value);
}

static __inline void actor_set_shield(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_shield(actor, value);
}

static __inline void actor_set_appearance_value(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_appearance_value(actor, value);
}

static __inline void actor_set_imf_name(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_imf_name(actor);
}

static __inline void actor_set_head_pal_name(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_head_pal_name(actor);
}

static __inline void actor_set_body_pal_name(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_body_pal_name(actor);
}

static __inline void actor_set_body_palette(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_body_palette(actor, value);
}

static __inline void actor_set_head_palette(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_head_palette(actor, value);
}

static __inline int64_t actor_get_weapon(void *object)
{
    struct GameActor *actor = object;
    return actor_ops_of(actor)->get_weapon(actor);
}

static __inline void actor_walk_motion(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->walk_motion(actor);
}

static __inline int actor_get_attack_motion(void *object)
{
    struct GameActor *actor = object;
    return actor_ops_of(actor)->get_attack_motion(actor);
}

static __inline void actor_make_corpse(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->make_corpse(actor);
}

static __inline void actor_set_attack_speed(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_attack_speed(actor, value);
}

static __inline void actor_set_honor(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_honor(actor, value);
}

static __inline void actor_set_honor_virtue(void *object, int honor, unsigned int virtue)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_honor_virtue(actor, honor, virtue);
}

static __inline int actor_get_char_id(void *object)
{
    struct GameActor *actor = object;
    return actor_ops_of(actor)->get_char_id(actor);
}

static __inline void actor_set_robe(void *object, int robe, unsigned char keep)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_robe(actor, robe, keep);
}

static __inline void actor_set_body_style(void *object, int style, unsigned char apply)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_body_style(actor, style, apply);
}

static __inline void actor_set_mounted(void *object, unsigned char value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_mounted(actor, value);
}

static __inline int actor_get_sprite_variant(void *object)
{
    struct GameActor *actor = object;
    return actor_ops_of(actor)->get_sprite_variant(actor);
}

static __inline void actor_set_sprite_variant(void *object, int value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_sprite_variant(actor, value);
}

static __inline unsigned char actor_get_skip_arrow(void *object)
{
    struct GameActor *actor = object;
    return actor_ops_of(actor)->get_skip_arrow(actor);
}

static __inline void actor_set_skip_arrow(void *object, unsigned char value)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->set_skip_arrow(actor, value);
}

static __inline void actor_refresh_accessory3(void *object)
{
    struct GameActor *actor = object;
    actor_ops_of(actor)->refresh_accessory3(actor);
}

static __inline struct ActorHeadExtent actor_get_head_extent(void *object)
{
    struct GameActor *actor = object;
    return actor_ops_of(actor)->get_head_extent(actor);
}

void actor_set_look(void *actor, const struct ActorLook *look);
void actor_set_look_with_state(void *actor, const struct ActorLook *look, int effect_state);

void c3d_granny_game_actor_make_corpse(struct GameActor *actor);
void c3d_granny_game_actor_set_job_model(struct GameActor *actor, int value0, int value1);
void game_actor_clear_unit_cell(struct GameActor *actor);
int game_actor_get_attack_motion(struct GameActor *actor);
int game_actor_get_guild_id(struct GameActor *actor);
int game_actor_get_npc_location_radius(struct GameActor *actor);
int *game_actor_get_request(struct GameActor *actor, int value0);
int *game_actor_get_request2(struct GameActor *actor, int value0);
void game_actor_make_corpse(struct GameActor *actor);
unsigned char game_actor_process_state(struct GameActor *actor);
void game_actor_set_attack_speed(struct GameActor *actor, int value0);
void game_actor_set_guild(struct GameActor *actor, int value0, int value1);
void game_actor_set_guild_id(struct GameActor *actor, int value0);
void game_actor_set_mounted(struct GameActor *actor, unsigned char value0);
void game_actor_set_request(struct GameActor *actor, const int *value0, int value1);
void game_actor_set_request2(struct GameActor *actor, const int *value0, int value1);
void game_actor_set_state(struct GameActor *actor, int value0, unsigned char value1);
void game_actor_set_unit_cell(struct GameActor *actor);
void game_actor_walk_motion(struct GameActor *actor);
int64_t granny_pc_get_weapon(struct GameActor *actor);
unsigned char granny_pc_process_state(struct GameActor *actor);
void granny_pc_set_accessory(struct GameActor *actor, int value0);
void granny_pc_set_accessory2(struct GameActor *actor, int value0);
void granny_pc_set_accessory3(struct GameActor *actor, int value0);
void granny_pc_set_delay_factor(struct GameActor *actor, int value0);
void granny_pc_set_head(struct GameActor *actor, int value0);
void granny_pc_set_head_pal_name(struct GameActor *actor);
void granny_pc_set_honor(struct GameActor *actor, int value0);
void granny_pc_set_honor_virtue(struct GameActor *actor, int value0, unsigned int value1);
void granny_pc_set_imf_name(struct GameActor *actor);
void granny_pc_set_spr_act(struct GameActor *actor, const struct ActorLook *value0);
void granny_pc_set_spr_act_with_state(struct GameActor *actor, const struct ActorLook *value0, int value1);
void granny_pc_set_spr_job(struct GameActor *actor, int value0);
void granny_pc_set_state(struct GameActor *actor, int value0, unsigned char value1);
void granny_pc_set_weapon(struct GameActor *actor, int64_t value0);
void npc_clear_unit_cell(struct GameActor *actor);
void npc_set_job_and_sex(struct GameActor *actor, int value0, int value1);
void npc_set_state(struct GameActor *actor, int value0, unsigned char value1);
void npc_set_unit_cell(struct GameActor *actor);
void npc_set_spr_job_(struct GameActor *actor, int value0);
int pc_get_char_id(struct GameActor *actor);
unsigned char pc_get_skip_arrow(struct GameActor *actor);
int64_t pc_get_weapon(struct GameActor *actor);
unsigned char pc_process_state(struct GameActor *actor);
void pc_refresh_accessory3(struct GameActor *actor);
void pc_set_accessory(struct GameActor *actor, int value0);
void pc_set_accessory2(struct GameActor *actor, int value0);
void pc_set_accessory3(struct GameActor *actor, int value0);
void pc_set_body_pal_name(struct GameActor *actor);
void pc_set_body_palette(struct GameActor *actor, int value0);
void pc_set_body_style(struct GameActor *actor, int value0, unsigned char value1);
void pc_set_delay_factor(struct GameActor *actor, int value0);
void pc_set_head(struct GameActor *actor, int value0);
void pc_set_head_pal_name(struct GameActor *actor);
void pc_set_head_palette(struct GameActor *actor, int value0);
void pc_set_honor(struct GameActor *actor, int value0);
void pc_set_honor_virtue(struct GameActor *actor, int value0, unsigned int value1);
void pc_set_imf_name(struct GameActor *actor);
void pc_set_riding(struct GameActor *actor, unsigned char value0);
void pc_set_robe(struct GameActor *actor, int value0, unsigned char value1);
void pc_set_shield(struct GameActor *actor, int value0);
void pc_set_skip_arrow(struct GameActor *actor, unsigned char value0);
void pc_set_spr_act(struct GameActor *actor, const struct ActorLook *value0);
void pc_set_spr_act_with_state(struct GameActor *actor, const struct ActorLook *value0, int value1);
void pc_set_spr_job(struct GameActor *actor, int value0);
void pc_set_state(struct GameActor *actor, int value0, unsigned char value1);
void pc_set_weapon(struct GameActor *actor, int64_t value0);
void pc_set_weapon_look(struct GameActor *actor, int64_t value0);
struct ActorHeadExtent game_actor_get_head_extent(struct GameActor *actor);
unsigned char player_process_state(struct GameActor *actor);
void player_set_state(struct GameActor *actor, int value0, unsigned char value1);
void pc_set_shoes(struct GameActor *actor, int value0);
struct ActorHeadExtent pc_get_head_extent(struct GameActor *actor);
int pc_get_sprite_variant(struct GameActor *actor);
void pc_set_sprite_variant(struct GameActor *actor, int value0);
void granny_pc_set_shield(struct GameActor *actor, int value0);

void actor_default_bool(struct GameActor *actor, unsigned char value0);
unsigned char actor_default_false(struct GameActor *actor);
void actor_default_int(struct GameActor *actor, int value0);
void actor_default_int_bool(struct GameActor *actor, int value0, unsigned char value1);
void actor_default_int_flags(struct GameActor *actor, int value0, unsigned int value1);
void actor_default_look(struct GameActor *actor, const struct ActorLook *value0);
void actor_default_look_state(struct GameActor *actor, const struct ActorLook *value0, int value1);
int64_t actor_default_no_weapon(struct GameActor *actor);
void actor_default_noop(struct GameActor *actor);
void actor_default_pair(struct GameActor *actor, int value0, int value1);
void actor_default_weapon(struct GameActor *actor, int64_t value0);
int actor_default_zero(struct GameActor *actor);

struct MousePointer *mouse_pointer_construct(struct MousePointer *self);
void *render_object_get_spr_res(struct RenderObject *self);

void render_object_set_force_anim_end_tick(struct RenderObject *self, int ms);

struct BlurPC *blur_pc_construct(struct BlurPC *self, struct Pc *master);
struct HalluNPC *hallu_npc_construct(struct HalluNPC *self, struct GameActor *src);
struct HalluPC2 *hallu_pc2_construct(struct HalluPC2 *self, struct Pc *master);
struct HalluPlant *hallu_plant_construct(struct HalluPlant *self);
struct Npc *npc_construct(struct Npc *self);
struct SoundMaker *world_sound_construct(struct SoundMaker *self);

void npc_destruct(struct Npc *self);

#endif
