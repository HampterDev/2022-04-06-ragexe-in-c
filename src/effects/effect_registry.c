#include "effects/effect_allocator.h"
#include "effects/effect_registry.h"

/* The composition point of the effect types: every effect class's table (see
   effects/effect_types.h), in the order effect_mgr_create registers them. The rows belong to
   the classes; this list only says which tables exist and when each is registered.

   Order. The fixed tables come first, then the types known only at startup (the headgear
   effects from the Lua hatEffectTable, then the EZ2STR file), then the three late tables.
   A fixed table's types are distinct from every other fixed type and from the headgear range
   (from 0x951 on), so the order of the fixed tables and of the headgear range among
   themselves changes nothing. What the order decides:
   - the EZ2STR file comes after every fixed table except the late ones and after the
     headgear range, so a file type never replaces one of them (it is skipped), and a type
     the file lists twice keeps its first row;
   - the late tables come after the EZ2STR file: a file type that one of them lists stops
     the registration at that row, and effect_mgr_create fails.
   Within a table the rows are registered in their order.

   A table missing from this list is not registered: none of its types is created (see
   effects/effect_types.h). The fixed tables are listed in the order in which their first
   row appeared in the single row list this file held before; that order is kept only for
   reading. */
static const struct EffectTypeSource k_effect_type_sources[] = {
    /* Fixed tables. */
    { &level99_effect_types },
    { &level99_orb1_effect_types },
    { &level99_orb2_effect_types },
    { &smoke_effect_types },
    { &emitter_effect_types },
    { &level150_effect_types },
    { &level150_sub_effect_types },
    { &map_chain_effect_types },
    { &magic_floor_effect_types },
    { &ez2_str_effect_types },
    { &throw_item_effect_types },
    { &rotate_item_effect_types },
    { &rotate_line_effect_types },
    { &energy_orb_effect_types },
    { &sprite_effect_types },
    { &square_range_effect_types },
    { &scatter_effect_types },
    { &cell_range_effect_types },
    { &cherry_blossom_effect_types },
    { &storm_kick_effect_types },
    { &evils_paw_effect_types },
    { &common_floor_effect_types },
    { &animated_emitter_effect_types },
    { &cloud_kill_effect_types },
    { &sprite_mable_effect_types },
    { &tuna_party_effect_types },
    { &root_twist_effect_types },
    { &heal_effect_types },
    { &warning_plane_effect_types },
    { &cloud_types },
    { &random_opt_effect_types },
    { &new_cloud_kill_effect_types },
    { &light_sphere_effect_types },
    { &gravity_control_effect_types },
    { &sun_stance_effect_types },
    { &soul_unity_effect_types },
    { &tetra_ball_effect_types },
    { &evil_land_effect_types },
    { &axe_boomerang_types },
    { &item_drop_effect_types },
    { &curse_of_cube_effect_types },
    { &cane_of_evil_eye_effect_types },
    { &blast_forge_types },
    { &wind_effect_types },
    /* Types known only at startup. */
    { NULL, ez2_str_effect_add_hat_types },
    { NULL, ez2_str_effect_ex_add_file_types },
    /* Late tables, after the EZ2STR file. */
    { &ez2_str_effect_late_types },
    { &change_size_effect_types },
    { &show_effect_types },
};

const struct EffectTypeSources g_effectTypes = {
    k_effect_type_sources, sizeof(k_effect_type_sources) / sizeof(k_effect_type_sources[0])
};
