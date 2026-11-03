#ifndef EFFECTS_EFFECT_ALLOCATOR_H
#define EFFECTS_EFFECT_ALLOCATOR_H

#include "effects/effect_types.h"

struct Effect;
struct EffectMgr;

struct Effect *animated_emitter_effect_new(void);
struct Effect *axe_boomerang_new(void);
struct Effect *blast_forge_new(void);
struct Effect *cane_of_evil_eye_effect_new(void);
struct Effect *cell_range_effect_new(void);
struct Effect *change_size_effect_new(void);
struct Effect *cherry_blossom_effect_new(void);
struct Effect *cloud_new(void);
struct Effect *cloud_kill_effect_new(void);
struct Effect *common_floor_effect_new(void);
struct Effect *curse_of_cube_effect_new(void);
struct Effect *ez2_str_effect_new(void);
struct Effect *ez2_str_effect_ex_new(void);
struct Effect *emitter_effect_new(void);
struct Effect *energy_orb_effect_new(void);
struct Effect *evil_land_effect_new(void);
struct Effect *evils_paw_effect_new(void);
struct Effect *gravity_control_effect_new(void);
struct Effect *heal_effect_new(void);
struct Effect *item_drop_effect_new(void);
struct Effect *level150_effect_new(void);
struct Effect *level150_sub_effect_new(void);
struct Effect *level99_effect_new(void);
struct Effect *level99_orb1_effect_new(void);
struct Effect *level99_orb2_effect_new(void);
struct Effect *light_sphere_effect_new(void);
struct Effect *magic_floor_effect_new(void);
struct Effect *map_chain_effect_new(void);
struct Effect *new_cloud_kill_effect_new(void);
struct Effect *random_opt_effect_new(void);
struct Effect *root_twist_effect_new(void);
struct Effect *rotate_item_effect_new(void);
struct Effect *rotate_line_effect_new(void);
struct Effect *scatter_effect_new(void);
struct Effect *show_effect_new(void);
struct Effect *smoke_effect_new(void);
struct Effect *soul_unity_effect_new(void);
struct Effect *sprite_effect_new(void);
struct Effect *sprite_mable_effect_new(void);
struct Effect *square_range_effect_new(void);
struct Effect *storm_kick_effect_new(void);
struct Effect *sun_stance_effect_new(void);
struct Effect *tetra_ball_effect_new(void);
struct Effect *throw_item_effect_new(void);
struct Effect *tuna_party_effect_new(void);
struct Effect *warning_plane_effect_new(void);
struct Effect *wind_effect_new(void);


/* The effect types of each class, defined next to its constructor (see effects/effect_types.h);
   effects/effect_registry.c registers them in the order of g_effectTypes. */
extern const struct EffectTypeTable animated_emitter_effect_types;
extern const struct EffectTypeTable axe_boomerang_types;
extern const struct EffectTypeTable blast_forge_types;
extern const struct EffectTypeTable cane_of_evil_eye_effect_types;
extern const struct EffectTypeTable cell_range_effect_types;
extern const struct EffectTypeTable change_size_effect_types;
extern const struct EffectTypeTable cherry_blossom_effect_types;
extern const struct EffectTypeTable cloud_kill_effect_types;
extern const struct EffectTypeTable cloud_types;
extern const struct EffectTypeTable common_floor_effect_types;
extern const struct EffectTypeTable curse_of_cube_effect_types;
extern const struct EffectTypeTable emitter_effect_types;
extern const struct EffectTypeTable energy_orb_effect_types;
extern const struct EffectTypeTable evil_land_effect_types;
extern const struct EffectTypeTable evils_paw_effect_types;
extern const struct EffectTypeTable ez2_str_effect_types;
extern const struct EffectTypeTable ez2_str_effect_late_types;
extern const struct EffectTypeTable gravity_control_effect_types;
extern const struct EffectTypeTable heal_effect_types;
extern const struct EffectTypeTable item_drop_effect_types;
extern const struct EffectTypeTable level150_effect_types;
extern const struct EffectTypeTable level150_sub_effect_types;
extern const struct EffectTypeTable level99_effect_types;
extern const struct EffectTypeTable level99_orb1_effect_types;
extern const struct EffectTypeTable level99_orb2_effect_types;
extern const struct EffectTypeTable light_sphere_effect_types;
extern const struct EffectTypeTable magic_floor_effect_types;
extern const struct EffectTypeTable map_chain_effect_types;
extern const struct EffectTypeTable new_cloud_kill_effect_types;
extern const struct EffectTypeTable random_opt_effect_types;
extern const struct EffectTypeTable root_twist_effect_types;
extern const struct EffectTypeTable rotate_item_effect_types;
extern const struct EffectTypeTable rotate_line_effect_types;
extern const struct EffectTypeTable scatter_effect_types;
extern const struct EffectTypeTable show_effect_types;
extern const struct EffectTypeTable smoke_effect_types;
extern const struct EffectTypeTable soul_unity_effect_types;
extern const struct EffectTypeTable sprite_effect_types;
extern const struct EffectTypeTable sprite_mable_effect_types;
extern const struct EffectTypeTable square_range_effect_types;
extern const struct EffectTypeTable storm_kick_effect_types;
extern const struct EffectTypeTable sun_stance_effect_types;
extern const struct EffectTypeTable tetra_ball_effect_types;
extern const struct EffectTypeTable throw_item_effect_types;
extern const struct EffectTypeTable tuna_party_effect_types;
extern const struct EffectTypeTable warning_plane_effect_types;
extern const struct EffectTypeTable wind_effect_types;

/* Effect types known only at startup, defined next to their class's constructor. */
unsigned char ez2_str_effect_add_hat_types(struct EffectMgr *mgr);
unsigned char ez2_str_effect_ex_add_file_types(struct EffectMgr *mgr);

#endif
