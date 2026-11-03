#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "effects/effect.h"
#include "effects/effect_allocator.h"
#include "effects/effect_mgr.h"
#include "effects/ez2str_effect_mgr.h"
#include "std/vec.h"

struct EffectTypeRow {
    int first, last;
    struct Effect *(*create)(void);
};
#define ONE(type, create)          { type, type, create }
#define RANGE(first, last, create) { first, last, create }
#define COUNT(a)                (sizeof(a) / sizeof((a)[0]))

static const struct EffectTypeRow k_effects_first[] = {
    ONE(0x0c8, level99_effect_new),
    ONE(0x3d0, level99_orb1_effect_new),
    ONE(0x3d1, level99_orb2_effect_new),
    ONE(0x02c, smoke_effect_new),
    ONE(0x3ce, emitter_effect_new),
    ONE(0x3d2, level150_effect_new),
    ONE(0x3d3, level150_sub_effect_new),
    ONE(0x400, map_chain_effect_new),
    ONE(0x401, magic_floor_effect_new),
    ONE(0x3fe, level150_effect_new),
    ONE(0x3ff, level150_sub_effect_new),
    RANGE(0x0cc, 0x0d3, ez2_str_effect_new),
    RANGE(0x12a, 0x12b, throw_item_effect_new),
    ONE(0x134, throw_item_effect_new),
    ONE(0x21b, throw_item_effect_new),
    ONE(0x3d4, throw_item_effect_new),
    ONE(0x21d, throw_item_effect_new),
    ONE(0x258, throw_item_effect_new),
    RANGE(0x265, 0x268, throw_item_effect_new),
    ONE(0x392, throw_item_effect_new),
    ONE(0x3d7, throw_item_effect_new),
    RANGE(0x3d5, 0x3d6, throw_item_effect_new),
    ONE(0x3d8, rotate_item_effect_new),
    ONE(0x3da, rotate_line_effect_new),
    ONE(0x3e8, rotate_line_effect_new),
    ONE(0x0e4, energy_orb_effect_new),
    RANGE(0x2e1, 0x2e2, energy_orb_effect_new),
    RANGE(0x2df, 0x2e0, energy_orb_effect_new),
    ONE(0x3dd, throw_item_effect_new),
    ONE(0x3de, sprite_effect_new),
    ONE(0x3df, square_range_effect_new),
    RANGE(0x3e4, 0x3e7, ez2_str_effect_new),
    ONE(0x3ed, energy_orb_effect_new),
    ONE(0x3ea, scatter_effect_new),
    ONE(0x3ef, cell_range_effect_new),
    RANGE(0x3f0, 0x3f2, ez2_str_effect_new),
    ONE(0x3e9, ez2_str_effect_new),
    ONE(0x3f3, ez2_str_effect_new),
    ONE(0x3e0, ez2_str_effect_new),
    ONE(0x3e3, ez2_str_effect_new),
    ONE(0x3e2, ez2_str_effect_new),
    ONE(0x3e1, ez2_str_effect_new),
    ONE(0x3f5, cherry_blossom_effect_new),
    ONE(0x1b3, storm_kick_effect_new),
    ONE(0x3f4, ez2_str_effect_new),
    ONE(0x40a, ez2_str_effect_new),
    ONE(0x437, ez2_str_effect_new),
    ONE(0x43f, ez2_str_effect_new),
    RANGE(0x40b, 0x40c, ez2_str_effect_new),
    ONE(0x40f, evils_paw_effect_new),
    ONE(0x420, common_floor_effect_new),
    ONE(0x426, ez2_str_effect_new),
    ONE(0x428, ez2_str_effect_new),
    ONE(0x427, ez2_str_effect_new),
    RANGE(0x42a, 0x42c, ez2_str_effect_new),
    ONE(0x432, ez2_str_effect_new),
    ONE(0x431, animated_emitter_effect_new),
    ONE(0x425, ez2_str_effect_new),
    RANGE(0x433, 0x436, ez2_str_effect_new),
    ONE(0x438, cloud_kill_effect_new),
    RANGE(0x43b, 0x43e, ez2_str_effect_new),
    RANGE(0x441, 0x442, ez2_str_effect_new),
    RANGE(0x443, 0x444, ez2_str_effect_new),
    RANGE(0x446, 0x447, ez2_str_effect_new),
    ONE(0x448, sprite_mable_effect_new),
    ONE(0x449, tuna_party_effect_new),
    ONE(0x44a, ez2_str_effect_new),
    RANGE(0x44d, 0x44f, ez2_str_effect_new),
    ONE(0x451, ez2_str_effect_new),
    RANGE(0x453, 0x455, ez2_str_effect_new),
    ONE(0x456, root_twist_effect_new),
    ONE(0x45c, ez2_str_effect_new),
    RANGE(0x48a, 0x48b, ez2_str_effect_new),
    ONE(0x489, ez2_str_effect_new),
};

static const struct EffectTypeRow k_effects_middle[] = {
    RANGE(0x138, 0x139, heal_effect_new),
    ONE(0x140, heal_effect_new),
    ONE(0x145, heal_effect_new),
    ONE(0x0fd, heal_effect_new),
    ONE(0x459, heal_effect_new),
    RANGE(0x477, 0x478, heal_effect_new),
    ONE(0x45a, warning_plane_effect_new),
    ONE(0x45f, ez2_str_effect_new),
    ONE(0x461, ez2_str_effect_new),
    RANGE(0x463, 0x464, ez2_str_effect_new),
    RANGE(0x466, 0x467, ez2_str_effect_new),
    ONE(0x469, ez2_str_effect_new),
    ONE(0x08d, ez2_str_effect_new),
    ONE(0x46a, sprite_effect_new),
    RANGE(0x46d, 0x46e, ez2_str_effect_new),
    RANGE(0x470, 0x473, ez2_str_effect_new),
    RANGE(0x475, 0x476, ez2_str_effect_new),
    ONE(0x47a, cloud_new),
    ONE(0x47b, sprite_effect_new),
    RANGE(0x47f, 0x488, sprite_effect_new),
    ONE(0x4a0, sprite_effect_new),
    RANGE(0x4a3, 0x4a4, random_opt_effect_new),
    ONE(0x4a7, random_opt_effect_new),
    ONE(0x4a8, sprite_effect_new),
    ONE(0x4ac, new_cloud_kill_effect_new),
    RANGE(0x4ad, 0x4af, light_sphere_effect_new),
    ONE(0x4b1, gravity_control_effect_new),
    RANGE(0x4bb, 0x4bd, sprite_effect_new),
    RANGE(0x4be, 0x4c1, sun_stance_effect_new),
    ONE(0x4c6, soul_unity_effect_new),
    RANGE(0x4c2, 0x4c5, ez2_str_effect_new),
    ONE(0x4c7, storm_kick_effect_new),
    ONE(0x4ca, level150_effect_new),
    ONE(0x4cb, level150_sub_effect_new),
    RANGE(0x4cc, 0x4ce, ez2_str_effect_new),
    RANGE(0x4cf, 0x4d5, sprite_effect_new),
    RANGE(0x4d6, 0x4d7, ez2_str_effect_new),
    ONE(0x4da, sprite_effect_new),
    RANGE(0x4db, 0x4e7, ez2_str_effect_new),
    RANGE(0x4ea, 0x4ed, ez2_str_effect_new),
    RANGE(0x4ee, 0x4f1, tetra_ball_effect_new),
    RANGE(0x4f2, 0x4f9, ez2_str_effect_new),
    RANGE(0x4fd, 0x501, ez2_str_effect_new),
    RANGE(0x4fa, 0x4fb, ez2_str_effect_new),
    ONE(0x512, ez2_str_effect_new),
    ONE(0x505, ez2_str_effect_new),
    RANGE(0x502, 0x504, ez2_str_effect_new),
    ONE(0x507, ez2_str_effect_new),
    ONE(0x50f, ez2_str_effect_new),
    RANGE(0x508, 0x50a, ez2_str_effect_new),
    RANGE(0x50d, 0x50e, ez2_str_effect_new),
    RANGE(0x510, 0x511, ez2_str_effect_new),
    RANGE(0x51e, 0x52a, ez2_str_effect_new),
    RANGE(0x513, 0x51d, ez2_str_effect_new),
    ONE(0x52b, ez2_str_effect_new),
    ONE(0x544, ez2_str_effect_new),
    ONE(0x543, ez2_str_effect_new),
    ONE(0x546, ez2_str_effect_new),
    ONE(0x545, ez2_str_effect_new),
    ONE(0x548, ez2_str_effect_new),
    ONE(0x547, ez2_str_effect_new),
    ONE(0x54a, ez2_str_effect_new),
    ONE(0x549, ez2_str_effect_new),
    RANGE(0x54b, 0x54c, ez2_str_effect_new),
    RANGE(0x568, 0x569, ez2_str_effect_new),
    RANGE(0x54e, 0x551, ez2_str_effect_new),
    RANGE(0x58e, 0x58f, ez2_str_effect_new),
    RANGE(0x552, 0x560, ez2_str_effect_new),
    ONE(0x561, sprite_effect_new),
    RANGE(0x562, 0x567, ez2_str_effect_new),
    RANGE(0x56a, 0x575, ez2_str_effect_new),
    RANGE(0x578, 0x580, ez2_str_effect_new),
    RANGE(0x582, 0x589, ez2_str_effect_new),
    ONE(0x58d, ez2_str_effect_new),
    ONE(0x58b, sprite_effect_new),
    RANGE(0x590, 0x5a7, ez2_str_effect_new),
    RANGE(0x5aa, 0x5b9, ez2_str_effect_new),
    RANGE(0x656, 0x657, ez2_str_effect_new),
    RANGE(0x5ba, 0x5f4, ez2_str_effect_new),
    RANGE(0x5f6, 0x5fa, ez2_str_effect_new),
    RANGE(0x5fc, 0x608, ez2_str_effect_new),
    RANGE(0x60a, 0x612, ez2_str_effect_new),
    RANGE(0x615, 0x617, ez2_str_effect_new),
    RANGE(0x619, 0x61c, ez2_str_effect_new),
    ONE(0x613, evil_land_effect_new),
    RANGE(0x61e, 0x61f, ez2_str_effect_new),
    ONE(0x639, ez2_str_effect_new),
    RANGE(0x622, 0x626, ez2_str_effect_new),
    RANGE(0x628, 0x62f, ez2_str_effect_new),
    RANGE(0x631, 0x638, ez2_str_effect_new),
    RANGE(0x63b, 0x63f, ez2_str_effect_new),
    RANGE(0x64b, 0x650, ez2_str_effect_new),
    RANGE(0x651, 0x653, sprite_effect_new),
    RANGE(0x654, 0x655, ez2_str_effect_new),
    RANGE(0x658, 0x676, ez2_str_effect_new),
    ONE(0x679, axe_boomerang_new),
    RANGE(0x67a, 0x67c, ez2_str_effect_new),
    RANGE(0x681, 0x685, ez2_str_effect_new),
    RANGE(0x68b, 0x68c, ez2_str_effect_new),
    RANGE(0x68f, 0x69d, ez2_str_effect_new),
    RANGE(0x6a2, 0x6ac, ez2_str_effect_new),
    RANGE(0x6c2, 0x6c3, ez2_str_effect_new),
    ONE(0x6cd, ez2_str_effect_new),
    RANGE(0x74d, 0x74f, item_drop_effect_new),
    RANGE(0x4a5, 0x4a6, item_drop_effect_new),
    ONE(0x4a2, item_drop_effect_new),
    ONE(0x8e3, level150_effect_new),
    ONE(0x8e4, level150_sub_effect_new),
    RANGE(0x8fe, 0x900, ez2_str_effect_new),
    RANGE(0x901, 0x902, curse_of_cube_effect_new),
    ONE(0x903, cane_of_evil_eye_effect_new),
    ONE(0x904, ez2_str_effect_new),
    RANGE(0x907, 0x921, ez2_str_effect_new),
    RANGE(0x923, 0x924, ez2_str_effect_new),
    ONE(0x925, blast_forge_new),
    ONE(0x926, ez2_str_effect_new),
    ONE(0x927, wind_effect_new),
};

static const struct EffectTypeRow k_effects_last[] = {
    RANGE(0x943, 0x944, ez2_str_effect_new),
    ONE(0x946, change_size_effect_new),
    ONE(0x947, show_effect_new),
};

#define EFFECT_HAT_BASE 0x950

static unsigned char register_rows(struct EffectMgr *mgr, const struct EffectTypeRow *rows, size_t count)
{
    size_t i;
    int type;

    for (i = 0; i < count; i++)
        for (type = rows[i].first; type <= rows[i].last; type++)
            if (!effect_mgr_register_factory(mgr, type, rows[i].create))
                return 0;
    return 1;
}

unsigned char effect_mgr_register_effect_types(struct EffectMgr *self)
{
    struct Vec ez2str_ids;
    int hat_count, i;
    const int *id;

    if (!register_rows(self, k_effects_first, COUNT(k_effects_first)))
        return 0;

    hat_count = 0;

    client_lua_call(g_session1.luaState, "GetTableSizeForC", "s>d", "hatEffectTable", &hat_count);
    for (i = 1; i <= hat_count; i++)
        if (!effect_mgr_register_factory(self, EFFECT_HAT_BASE + i, ez2_str_effect_new))
            return 0;

    if (!register_rows(self, k_effects_middle, COUNT(k_effects_middle)))
        return 0;

    ez2_str_effect_mgr_get_effect_ids(ez2str_effect_mgr_instance(), &ez2str_ids);
    for (id = ez2str_ids.first; id != ez2str_ids.last; id++)
        effect_mgr_register_factory(self, *id, ez2_str_effect_ex_new);
    vec_free(&ez2str_ids);

    return register_rows(self, k_effects_last, COUNT(k_effects_last));
}
