#include "effects/effect_allocator.h"
#include "actor/actor_ops.h"
#include "contents/client_lua_calls.h"
#include "contents/sound.h"
#include "effects/durations.h"
#include "session/state.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effects/skills/ez2str_effect.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_math.h"
#include "effects/render_object.h"
#include "render/render_api.h"
#include "actor/game_actor.h"
#include "render/screen_draw.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"

struct Effect;

static void ez2_str_effect_destroy(struct Effect *effect);

static const struct EffectOps ez2_str_effect_ops = {
    .set_submit = ez2_str_effect_set_submit,
    .init = ez2_str_effect_init,
    .update_range = effect_update_range,
    .process = ez2_str_effect_process,
    .render = ez2_str_effect_render,
    .release = ez2_str_effect_release,
    .is_alive = ez2_str_effect_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = ez2_str_effect_after_init,
    .skip = ez2_str_effect_skip,
    .set_stop_frame = ez2_str_effect_set_stop_frame,
    .set_start_delay = ez2_str_effect_set_start_delay,
    .set_spin = ez2_str_effect_set_spin,
    .set_str_name = ez2_str_effect_set_str_name,
    .set_list_flags = ez2_str_effect_set_list_flags,
    .set_zoom = ez2_str_effect_set_zoom,
    .set_alpha_scale = ez2_str_effect_set_alpha_scale,
    .destroy = ez2_str_effect_destroy,
    .fade_out = effect_fade_out,
};

#define CEZ2STR_DEFAULT_STR "빨간포션.str"
#define HAT_FIRST 0x950

#define RENDERER_WIDTH(r)  ((r)->width)
#define RENDERER_HEIGHT(r) ((r)->height)
#define ACTOR_STATE_BITS(a) ((const struct RenderObject *)(a))->overlay_flags
#define ACTOR_RIDING(a)    ((const struct GameActor *)(a))->mounted
#define ACTOR_VISIBLE(a) ((const struct RenderObject *)(a))->visible
#define ACTOR_REVEALED(a) ((const struct RenderObject *)(a))->revealed
#define ACTOR_LIFT(a)      ((const struct RenderObject *)(a))->hover_height
#define PARTICLE_LIFETIME(p) ((p)->base.life_seconds)

struct TypeRange { short first, last; };

static int type_in(const struct TypeRange *set, int count, int type)
{
    int i;
    for (i = 0; i < count; i++)
        if (type >= set[i].first && type <= set[i].last)
            return 1;
    return 0;
}
#define TYPE_IN(set, type) type_in((set), (int)(sizeof(set) / sizeof((set)[0])), (type))

static const struct TypeRange HEADGEAR_TYPES[] = {
    {0x3f4, 0x3f4}, {0x40a, 0x40c}, {0x433, 0x433}, {0x437, 0x437}, {0x43b, 0x43f}, {0x441, 0x441},
    {0x443, 0x444}, {0x447, 0x447},
};

static const struct TypeRange HEAD_TYPES[] = {
    {0x425, 0x425}, {0x42c, 0x42c}, {0x442, 0x442}, {0x446, 0x446}, {0x44a, 0x44a}, {0x44e, 0x44f},
    {0x451, 0x451}, {0x453, 0x455}, {0x45c, 0x45c}, {0x45f, 0x45f}, {0x463, 0x464}, {0x467, 0x467},
    {0x46d, 0x46e}, {0x470, 0x470}, {0x472, 0x473}, {0x475, 0x476}, {0x4c2, 0x4c5}, {0x4cc, 0x4ce},
    {0x4d6, 0x4d7}, {0x4db, 0x4db}, {0x4ea, 0x4fb}, {0x4fd, 0x4fd}, {0x522, 0x522}, {0x543, 0x54a},
    {0x552, 0x552}, {0x590, 0x593}, {0x5c5, 0x5c5}, {0x5ce, 0x5d1}, {0x62d, 0x62f}, {0x631, 0x639},
    {0x63b, 0x63f}, {0x64b, 0x650}, {0x657, 0x660}, {0x663, 0x676}, {0x686, 0x68a}, {0x6a2, 0x6a9},
    {0x6ac, 0x6ac}, {0x6ae, 0x6d7}, {0x6d9, 0x6da}, {0x6dd, 0x6df}, {0x6e1, 0x6e3}, {0x6e9, 0x705},
    {0x707, 0x707}, {0x709, 0x70b}, {0x70d, 0x712}, {0x714, 0x714}, {0x716, 0x716}, {0x718, 0x724},
    {0x726, 0x72c}, {0x72e, 0x730}, {0x732, 0x732}, {0x736, 0x73a}, {0x73c, 0x74a}, {0x751, 0x753},
    {0x755, 0x768}, {0x76a, 0x76d}, {0x76f, 0x775}, {0x77a, 0x77d}, {0x77f, 0x77f}, {0x782, 0x782},
    {0x784, 0x78b}, {0x78e, 0x791}, {0x793, 0x79e}, {0x7a0, 0x7a3}, {0x7ac, 0x7af}, {0x7b1, 0x7b4},
    {0x7b6, 0x7c1}, {0x7c3, 0x7cd}, {0x7cf, 0x7e9}, {0x7eb, 0x819}, {0x81b, 0x81c}, {0x81e, 0x81f},
    {0x821, 0x821}, {0x823, 0x83c}, {0x83e, 0x841}, {0x843, 0x869}, {0x86b, 0x874}, {0x876, 0x886},
    {0x888, 0x892}, {0x894, 0x89a}, {0x89c, 0x89d}, {0x89f, 0x8a5}, {0x8a7, 0x8ad}, {0x8af, 0x8b5},
    {0x8b7, 0x8bf}, {0x8c1, 0x8c8}, {0x8ca, 0x8ce}, {0x8d0, 0x8df}, {0x8e1, 0x8e2}, {0x8e7, 0x8e8},
    {0x900, 0x900}, {0x904, 0x904}, {0x910, 0x921}, {0x923, 0x924}, {0x926, 0x926}, {0x943, 0x944},
};

static const struct TypeRange OWNER_BOUND_TYPES[] = {
    {0x08d, 0x08d}, {0x3e6, 0x3e6}, {0x45f, 0x45f}, {0x470, 0x470}, {0x473, 0x473}, {0x4c5, 0x4c5},
    {0x4ee, 0x4f1}, {0x514, 0x51d}, {0x552, 0x552}, {0x5bb, 0x5c4}, {0x63f, 0x63f}, {0x64b, 0x650},
    {0x676, 0x676}, {0x6ae, 0x6ae}, {0x6be, 0x6bf}, {0x6cd, 0x6cd}, {0x6d7, 0x6d7}, {0x6eb, 0x6ec},
    {0x6f0, 0x6f1}, {0x701, 0x703}, {0x719, 0x719}, {0x72f, 0x72f}, {0x752, 0x752}, {0x756, 0x756},
    {0x75a, 0x75a}, {0x765, 0x766}, {0x76c, 0x76c}, {0x77f, 0x77f}, {0x7aa, 0x7aa}, {0x7b7, 0x7b7},
    {0x7ba, 0x7ba}, {0x7bd, 0x7be}, {0x7c1, 0x7c1}, {0x7c7, 0x7c7}, {0x7d2, 0x7d2}, {0x7ef, 0x7ef},
    {0x7f9, 0x7fa}, {0x803, 0x804}, {0x807, 0x808}, {0x80c, 0x80c}, {0x81e, 0x81f}, {0x84e, 0x84e},
    {0x851, 0x852}, {0x876, 0x876}, {0x881, 0x881}, {0x886, 0x886}, {0x8a2, 0x8a3}, {0x8aa, 0x8ab},
    {0x8b2, 0x8b3}, {0x8ba, 0x8bb}, {0x8dc, 0x8de}, {0x8e1, 0x8e1}, {0x8e8, 0x8e8}, {0x8fe, 0x8fe},
};

static const struct TypeRange LOOPING_TYPES[] = {
    {0x3e6, 0x3e6}, {0x442, 0x442}, {0x45f, 0x45f}, {0x470, 0x470}, {0x473, 0x473}, {0x4c5, 0x4c5},
    {0x4dc, 0x4e5}, {0x4ee, 0x4f1},
};

static const struct TypeRange KEYED_LOOPING_TYPES[] = {
    {0x552, 0x552}, {0x584, 0x584}, {0x5a6, 0x5a6}, {0x607, 0x607}, {0x63f, 0x63f}, {0x64b, 0x650},
    {0x676, 0x676}, {0x6ae, 0x6ae}, {0x6be, 0x6bf}, {0x6cd, 0x6cd}, {0x6d7, 0x6d7}, {0x6eb, 0x6ec},
    {0x6f0, 0x6f1}, {0x701, 0x703}, {0x712, 0x712}, {0x719, 0x719}, {0x72f, 0x72f}, {0x73c, 0x73d},
    {0x752, 0x752}, {0x756, 0x756}, {0x75a, 0x75a}, {0x765, 0x766}, {0x76c, 0x76c}, {0x77f, 0x77f},
    {0x7aa, 0x7aa}, {0x7b7, 0x7b7}, {0x7ba, 0x7ba}, {0x7bd, 0x7be}, {0x7c1, 0x7c1}, {0x7c7, 0x7c7},
    {0x7d2, 0x7d2}, {0x7ef, 0x7ef}, {0x7f9, 0x7fa}, {0x803, 0x804}, {0x807, 0x808}, {0x80c, 0x80c},
    {0x81e, 0x81f}, {0x84e, 0x84e}, {0x851, 0x852}, {0x876, 0x876}, {0x881, 0x881}, {0x886, 0x886},
    {0x8a2, 0x8a3}, {0x8aa, 0x8ab}, {0x8b2, 0x8b3}, {0x8ba, 0x8bb}, {0x8dc, 0x8de}, {0x8e1, 0x8e1},
    {0x8e8, 0x8e8}, {0x8fe, 0x8fe},
};

static const struct TypeRange BEFORE_ACTORS_TYPES[] = {
    {0x4e7, 0x4e7}, {0x4eb, 0x4eb}, {0x4f6, 0x4f9}, {0x4fb, 0x4fb}, {0x4fd, 0x4fd}, {0x503, 0x504},
    {0x507, 0x507}, {0x509, 0x50a}, {0x511, 0x511}, {0x51f, 0x51f}, {0x521, 0x521}, {0x544, 0x544},
    {0x54a, 0x54a}, {0x54c, 0x54c}, {0x54f, 0x54f}, {0x551, 0x551}, {0x556, 0x556}, {0x558, 0x558},
    {0x55a, 0x55a}, {0x55c, 0x55c}, {0x563, 0x563}, {0x569, 0x569}, {0x56b, 0x56b}, {0x571, 0x571},
    {0x575, 0x575}, {0x586, 0x587}, {0x589, 0x589}, {0x595, 0x595}, {0x59c, 0x5a0}, {0x5a2, 0x5a2},
    {0x5ab, 0x5ad}, {0x5af, 0x5b0}, {0x5b4, 0x5b4}, {0x5b6, 0x5b6}, {0x5b8, 0x5b8}, {0x5ce, 0x5ce},
    {0x5d1, 0x5d1}, {0x5d3, 0x5d3}, {0x5d5, 0x5d5}, {0x5d9, 0x5da}, {0x5de, 0x5de}, {0x5e0, 0x5e0},
    {0x5e2, 0x5e2}, {0x5ff, 0x600}, {0x602, 0x602}, {0x60c, 0x60c}, {0x60f, 0x60f}, {0x612, 0x612},
    {0x616, 0x616}, {0x61a, 0x61a}, {0x61c, 0x61c}, {0x623, 0x623}, {0x629, 0x629}, {0x62b, 0x62b},
    {0x62e, 0x62e}, {0x632, 0x632}, {0x634, 0x634}, {0x636, 0x636}, {0x63c, 0x63c}, {0x63e, 0x63e},
    {0x655, 0x656}, {0x659, 0x659}, {0x65c, 0x65c}, {0x65e, 0x65e}, {0x660, 0x660}, {0x662, 0x662},
    {0x664, 0x665}, {0x668, 0x668}, {0x66b, 0x66b}, {0x675, 0x675}, {0x67b, 0x67b}, {0x682, 0x682},
    {0x684, 0x684}, {0x68c, 0x68c}, {0x698, 0x698}, {0x69b, 0x69c}, {0x6a0, 0x6a1}, {0x6a8, 0x6a8},
    {0x6ab, 0x6ab}, {0x6b3, 0x6b3}, {0x6b5, 0x6b5}, {0x6bb, 0x6bb}, {0x6bd, 0x6bd}, {0x6bf, 0x6bf},
    {0x6c1, 0x6c1}, {0x6c3, 0x6c3}, {0x6c5, 0x6c5}, {0x6d2, 0x6d2}, {0x6e6, 0x6e6}, {0x6ec, 0x6ec},
    {0x6f1, 0x6f1}, {0x701, 0x701}, {0x703, 0x703}, {0x70b, 0x70b}, {0x71b, 0x71b}, {0x71f, 0x71f},
    {0x73d, 0x73d}, {0x741, 0x741}, {0x747, 0x747}, {0x768, 0x768}, {0x773, 0x774}, {0x786, 0x786},
    {0x78a, 0x78a}, {0x790, 0x790}, {0x7b7, 0x7b7}, {0x7ba, 0x7ba}, {0x7bd, 0x7be}, {0x7c1, 0x7c1},
    {0x7df, 0x7df}, {0x7e4, 0x7e4}, {0x7f4, 0x7f4}, {0x7fa, 0x7fa}, {0x7ff, 0x7ff}, {0x804, 0x804},
    {0x808, 0x808}, {0x80e, 0x80e}, {0x813, 0x813}, {0x816, 0x816}, {0x818, 0x818}, {0x81f, 0x81f},
    {0x828, 0x828}, {0x82a, 0x82a}, {0x82c, 0x82c}, {0x830, 0x830}, {0x833, 0x833}, {0x846, 0x846},
    {0x849, 0x849}, {0x850, 0x850}, {0x852, 0x852}, {0x856, 0x856}, {0x85c, 0x85c}, {0x85f, 0x85f},
    {0x862, 0x862}, {0x865, 0x865}, {0x868, 0x868}, {0x86e, 0x86e}, {0x872, 0x872}, {0x874, 0x874},
    {0x876, 0x876}, {0x878, 0x878}, {0x880, 0x880}, {0x88d, 0x88d}, {0x890, 0x890}, {0x892, 0x892},
    {0x895, 0x895}, {0x897, 0x897}, {0x899, 0x899}, {0x89d, 0x89d}, {0x8a1, 0x8a1}, {0x8a3, 0x8a3},
    {0x8a5, 0x8a5}, {0x8a9, 0x8a9}, {0x8ab, 0x8ab}, {0x8ad, 0x8ad}, {0x8b1, 0x8b1}, {0x8b3, 0x8b3},
    {0x8b5, 0x8b5}, {0x8b9, 0x8b9}, {0x8bb, 0x8bb}, {0x8c3, 0x8c3}, {0x8d3, 0x8d3}, {0x8d5, 0x8d5},
    {0x8d7, 0x8d7}, {0x8d9, 0x8d9}, {0x8db, 0x8db}, {0x8e7, 0x8e8}, {0x8fe, 0x8fe}, {0x911, 0x911},
    {0x914, 0x914}, {0x917, 0x917}, {0x91c, 0x91d}, {0x923, 0x923},
};

struct TypeShift { short first, last; signed char shift; };
static const struct TypeShift HEAD_HEIGHT_SHIFTS[] = {
    {0x40b, 0x40b, 5}, {0x45f, 0x45f, 5}, {0x7d6, 0x7d6, 5},
    {0x425, 0x425, -6}, {0x46d, 0x46d, -6}, {0x476, 0x476, -6}, {0x4c2, 0x4c2, -6}, {0x4c5, 0x4c5, -6},
    {0x4cd, 0x4ce, -6}, {0x4d6, 0x4d6, -6}, {0x545, 0x548, -6}, {0x5cf, 0x5d0, -6}, {0x63f, 0x63f, -6},
    {0x64b, 0x650, -6}, {0x6c9, 0x6c9, -6}, {0x6cc, 0x6cc, -6}, {0x6d0, 0x6d0, -6}, {0x6f8, 0x6f8, -6},
    {0x722, 0x722, -6}, {0x726, 0x726, -6}, {0x752, 0x752, -6}, {0x756, 0x756, -6}, {0x75a, 0x75a, -6},
    {0x77a, 0x77a, -6}, {0x7b2, 0x7b2, -6}, {0x802, 0x802, -6}, {0x859, 0x859, -6}, {0x86c, 0x86c, -6},
    {0x870, 0x870, -6},
    {0x42c, 0x42c, -8}, {0x43b, 0x43b, -8}, {0x442, 0x442, -8}, {0x44a, 0x44a, -8}, {0x44e, 0x44f, -8},
    {0x451, 0x451, -8}, {0x453, 0x455, -8}, {0x45c, 0x45c, -8}, {0x463, 0x463, -8}, {0x46e, 0x46e, -8},
    {0x470, 0x470, -8}, {0x472, 0x473, -8}, {0x765, 0x766, -8},
    {0x43c, 0x43c, -7}, {0x7e2, 0x7e2, -7},
    {0x43d, 0x43d, -5}, {0x441, 0x441, -5}, {0x443, 0x443, -5}, {0x447, 0x447, -5}, {0x4ed, 0x4ed, -5},
    {0x552, 0x552, -5}, {0x75e, 0x75e, -5}, {0x803, 0x803, -5}, {0x805, 0x806, -5}, {0x81e, 0x81e, -5},
    {0x821, 0x821, -5}, {0x83e, 0x83f, -5}, {0x853, 0x853, -5},
    {0x464, 0x464, 8}, {0x467, 0x467, 8},
    {0x475, 0x475, -4}, {0x4c3, 0x4c3, -4}, {0x522, 0x522, -4}, {0x543, 0x544, -4}, {0x549, 0x54a, -4},
    {0x5c5, 0x5c5, -4}, {0x5ce, 0x5ce, -4}, {0x5d1, 0x5d1, -4}, {0x6d5, 0x6d5, -4}, {0x6df, 0x6df, -4},
    {0x6e3, 0x6e3, -4}, {0x6ef, 0x6ef, -4}, {0x6f4, 0x6f4, -4}, {0x707, 0x707, -4}, {0x72e, 0x72e, -4},
    {0x736, 0x738, -4}, {0x742, 0x742, -4}, {0x74a, 0x74a, -4}, {0x753, 0x753, -4}, {0x757, 0x757, -4},
    {0x75b, 0x75b, -4}, {0x76a, 0x76a, -4}, {0x771, 0x771, -4}, {0x77f, 0x77f, -4}, {0x782, 0x782, -4},
    {0x79e, 0x79e, -4}, {0x7a0, 0x7a0, -4}, {0x7a2, 0x7a3, -4}, {0x7af, 0x7af, -4}, {0x7b6, 0x7b6, -4},
    {0x7ca, 0x7ca, -4}, {0x7d5, 0x7d5, -4}, {0x7d9, 0x7d9, -4}, {0x7db, 0x7db, -4}, {0x7e0, 0x7e1, -4},
    {0x7eb, 0x7eb, -4}, {0x800, 0x800, -4}, {0x827, 0x828, -4}, {0x82b, 0x82d, -4}, {0x843, 0x843, -4},
    {0x889, 0x889, -4}, {0x900, 0x900, -4}, {0x904, 0x904, -4}, {0x913, 0x921, -4}, {0x923, 0x924, -4},
    {0x926, 0x926, -4}, {0x943, 0x944, -4},
    {0x4c4, 0x4c4, -10}, {0x4d7, 0x4d7, -10}, {0x4db, 0x4db, -10}, {0x4ee, 0x4f1, -10},
    {0x4cc, 0x4cc, -16},
    {0x4ec, 0x4ec, -2}, {0x4f2, 0x4f4, -2}, {0x4f6, 0x4f9, -2}, {0x4fd, 0x4fd, -2}, {0x63b, 0x63e, -2},
    {0x686, 0x68a, -2}, {0x6ae, 0x6c8, -2}, {0x6ca, 0x6cb, -2}, {0x6cd, 0x6cf, -2}, {0x6d1, 0x6d4, -2},
    {0x6d9, 0x6da, -2}, {0x6dd, 0x6de, -2}, {0x6e1, 0x6e2, -2}, {0x6e9, 0x6ee, -2}, {0x6f0, 0x6f3, -2},
    {0x701, 0x705, -2}, {0x709, 0x70b, -2}, {0x70d, 0x70f, -2}, {0x747, 0x749, -2}, {0x755, 0x755, -2},
    {0x758, 0x759, -2}, {0x762, 0x764, -2}, {0x767, 0x768, -2}, {0x76b, 0x76c, -2}, {0x7a1, 0x7a1, -2},
    {0x7b1, 0x7b1, -2}, {0x7b3, 0x7b3, -2}, {0x7c8, 0x7c9, -2}, {0x7d3, 0x7d4, -2}, {0x7da, 0x7da, -2},
    {0x7dc, 0x7dd, -2}, {0x7e3, 0x7e9, -2}, {0x7ec, 0x7fd, -2}, {0x807, 0x816, -2}, {0x81b, 0x81c, -2},
    {0x829, 0x82a, -2}, {0x832, 0x83c, -2}, {0x844, 0x84d, -2}, {0x854, 0x858, -2}, {0x85a, 0x869, -2},
    {0x86d, 0x86f, -2}, {0x871, 0x874, -2}, {0x876, 0x886, -2}, {0x888, 0x888, -2}, {0x88a, 0x892, -2},
    {0x894, 0x89a, -2}, {0x8bc, 0x8bf, -2}, {0x8c1, 0x8c8, -2}, {0x8ca, 0x8cd, -2}, {0x8d2, 0x8df, -2},
    {0x8e1, 0x8e2, -2}, {0x8e8, 0x8e8, -2},
    {0x639, 0x639, -3}, {0x657, 0x657, -3}, {0x672, 0x673, -3}, {0x676, 0x676, -3}, {0x6a7, 0x6a9, -3},
    {0x71a, 0x71b, -3}, {0x72f, 0x730, -3}, {0x732, 0x732, -3}, {0x774, 0x774, -3}, {0x89c, 0x89d, -3},
    {0x89f, 0x8a5, -3}, {0x8a7, 0x8ad, -3}, {0x8af, 0x8b5, -3}, {0x8b7, 0x8bb, -3},
    {0x6f5, 0x6f7, -1}, {0x739, 0x73a, -1}, {0x73c, 0x73d, -1}, {0x746, 0x746, -1}, {0x75f, 0x75f, -1},
    {0x7ac, 0x7ad, -1}, {0x7b4, 0x7b4, -1}, {0x7c3, 0x7c3, -1}, {0x7cf, 0x7d2, -1}, {0x851, 0x852, -1},
    {0x743, 0x743, 4},
    {0x788, 0x788, 1}, {0x7de, 0x7df, 1},
};

static void lua_ask_hat(const char *func, const char *fmt, int hat, void *out)
{
    client_lua_call(g_session1.luaState, func, fmt, hat, out);
}

static void cue(struct Ez2StrEffect *self, const char *wave, int frame)
{
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, wave, frame, 0.0f);
}

static void cue_message(struct Ez2StrEffect *self, int frame)
{
    effect_add_sound_cue(&self->base, SOUND_CUE_GAME_MESSAGE, 0, frame, 0.0f);
}

static void set_list(struct Ez2StrEffect *self, int list_flags)
{
    self->particle->base.list_flags = list_flags;
}

struct Ez2StrEffect *ez2_str_effect_construct(struct Ez2StrEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &ez2_str_effect_ops;
    self->particle = 0;
    string_init(&self->str_name);
    string_assign(&self->str_name, CEZ2STR_DEFAULT_STR, 12);
    self->adjust_position_when_shrunk = 0;
    self->screen_shade = 0;
    self->turn_with_camera = 0;
    self->camera_angle = 0.0;
    self->attached_to_head = 0;
    return self;
}

struct Effect *ez2_str_effect_new(void)
{
    struct Ez2StrEffect *self = mem_try_alloc(sizeof *self);

    return self ? &ez2_str_effect_construct(self)->base : NULL;
}

void ez2_str_effect_destruct(struct Ez2StrEffect *self)
{
    self->base.ops = &ez2_str_effect_ops;
    string_free(&self->str_name);
    effect_destruct(&self->base);
}

static void ez2_str_effect_destroy(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    ez2_str_effect_destruct(self);
    mem_free(self);
}

unsigned char ez2_str_effect_is_headgear_type(struct Ez2StrEffect *self)
{
    int type = self->base.type;
    if (TYPE_IN(HEADGEAR_TYPES, type))
        return 1;
    return type >= HAT_FIRST;
}

unsigned char ez2_str_effect_is_head_type(struct Ez2StrEffect *self)
{
    return (unsigned char)TYPE_IN(HEAD_TYPES, self->base.type);
}

float ez2str_head_x_offset(int type, const char *str_path)
{
    float x = 0.0f;
    float result;

    switch (type) {
    case 0x40b: case 0x40c:
        x = 5.0f;
        break;
    case 0x43e:
        x = 8.0f;
        break;
    case 0x4c3:
        x = 0.5f;
        break;
    case 0x4cd: case 0x4ce: case 0x543: case 0x544: case 0x545: case 0x546: case 0x5ce: case 0x5cf:
        x = 1.0f;
        break;
    case 0x4f2: case 0x4f5:
        x = -0.5f;
        break;
    case 0x547: case 0x548: case 0x549: case 0x54a: case 0x5d0: case 0x5d1:
        x = -1.0f;
        break;
    }
    if (strstr(str_path, "strangelights"))
        x = 5.0f;
    else if (strstr(str_path, "superstar"))
        x = 5.0f;
    else if (strstr(str_path, "ljosalfar"))
        x = 8.0f;

    result = x;
    if (type >= HAT_FIRST) {
        double hat_x = 0.0;
        lua_ask_hat("GetHatEfPosX", "d>l", type - HAT_FIRST, &hat_x);
        result = (float)((double)x + hat_x);
    }
    return result;
}

int ez2str_head_height(int type, void *actor)
{
    unsigned char ignore_riding = 0;
    int height = 0;
    int i;

    if (type >= HAT_FIRST)
        lua_ask_hat("IsIgnoredRidingState_Include_AttachedToHead", "d>b", type - HAT_FIRST, &ignore_riding);
    if (type == 0x719)
        ignore_riding = 1;

    if (actor) {
        if (session_is_baby_job(&g_session1, game_actor_get_job(actor))) {
            if (ignore_riding == 1)
                height = 16;
            else if (ACTOR_RIDING(actor) != 0)
                height = session_is_doram_job(&g_session1, game_actor_get_job(actor)) != 1 ? 21 : 17;
            else
                height = game_actor_get_mount_height(actor) + 16;
        } else {
            if (ignore_riding == 1)
                height = 18;
            else if (ACTOR_RIDING(actor) != 0)
                height = session_is_doram_job(&g_session1, game_actor_get_job(actor)) == 1 ? 18 : 23;
            else
                height = game_actor_get_mount_height(actor) + 18;
        }
    }

    for (i = 0; i < (int)(sizeof(HEAD_HEIGHT_SHIFTS) / sizeof(HEAD_HEIGHT_SHIFTS[0])); i++)
        if (type >= HEAD_HEIGHT_SHIFTS[i].first && type <= HEAD_HEIGHT_SHIFTS[i].last) {
            height += HEAD_HEIGHT_SHIFTS[i].shift;
            break;
        }

    if (type >= HAT_FIRST) {
        int hat_height = 0;
        lua_ask_hat("GetHatEfPos", "d>d", type - HAT_FIRST, &hat_height);
        height += hat_height;
    }
    return height - 7;
}

unsigned char ez2_str_effect_place_at_head(struct Ez2StrEffect *self,
                                                    struct Ez2StrParticle *p)
{
    void *actor;
    float x_offset, height, scale, x_shift, y_shift;
    float view[12], from_view[12];
    float x, y, z, a, b, c;

    if (ez2_str_effect_is_headgear_type(self) != 1 && ez2_str_effect_is_head_type(self) != 1)
        return 0;
    actor = effect_find_actor(self->base.owner_aid);
    if (!actor)
        return 0;

    x_offset = ez2str_head_x_offset(self->base.type, string_cstr(&p->path));
    height = (float)ez2str_head_height(self->base.type, actor);
    if (self->attached_to_head == 1 && session_is_doram_job(&g_session1, game_actor_get_job(actor)) == 1)
        height = height - 3.0f;
    scale = session_get_baby_job_scale(&g_session1, game_actor_get_job(actor));
    if (((ACTOR_STATE_BITS(actor) >> 4) & 1) && self->adjust_position_when_shrunk == 1) {
        float own = 1.0f;
        render_object_apply_shrink_scale(actor, &own);
        scale = own * scale;
    }
    x_shift = scale * x_offset;
    y_shift = scale * height;
    if (!(x_shift >= 0.0f) && !(y_shift >= 0.0f))
        return 0;

    x = p->base.pos.x;
    y = p->base.pos.y;
    z = p->base.pos.z;
    if (!game_copy_view_matrix(view))
        return 0;
    mat_invert(from_view, view);

    a = (view[3] * y + view[0] * x) + view[6] * z + view[9] + x_shift;
    b = (view[4] * y + view[1] * x) + view[7] * z + view[10] - y_shift;
    c = (view[5] * y + view[2] * x) + view[8] * z + view[11];
    p->base.pos.x = (b * from_view[3] + a * from_view[0]) + c * from_view[6] + from_view[9];
    p->base.pos.y = (b * from_view[4] + a * from_view[1]) + c * from_view[7] + from_view[10];
    p->base.pos.z = (a * from_view[2] + from_view[5] * b) + from_view[8] * c + from_view[11];
    return 1;
}

void ez2_str_effect_set_submit(struct Effect *effect, ParticleSubmitFn fn)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    self->particle->base.submit = fn;
}

void ez2_str_effect_init(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, EZ2STR_LIST_DEFAULT);
    int type;

    if (!p)
        return;
    p->actor_aid = self->base.owner_aid;

    if (ez2_str_effect_is_headgear_type(self) == 1) {
        unsigned char adjust_size = 0, adjust_position = 0;
        self->base.has_instance_key = 1;
        p->loop_forever = 1;
        lua_ask_hat("IsAdjustSizeWhenShrinkState", "d>b", self->base.type - HAT_FIRST, &adjust_size);
        p->scale_by_actor = adjust_size;
        lua_ask_hat("IsAdjustPositionWhenShrinkState", "d>b", self->base.type - HAT_FIRST, &adjust_position);
        self->adjust_position_when_shrunk = adjust_position;
    }

    type = self->base.type;
    switch (type) {
    case 0x6ec:
        cue(self, "effect\\AG_MYSTERY_ILLUSION.wav", 0);
        self->base.has_instance_key = 1;
        p->loop_forever = 1;
        break;
    case 0x6f0:
        cue(self, "effect\\AG_TORNADO_STORM.wav", 0);
        self->base.has_instance_key = 1;
        p->loop_forever = 1;
        break;
    case 0x710:
        cue(self, "effect\\CD_MEDIALE_VOTUM.wav", 0);
        break;
    default:
        if (TYPE_IN(LOOPING_TYPES, type)) {
            p->loop_forever = 1;
        } else if (TYPE_IN(KEYED_LOOPING_TYPES, type)) {
            self->base.has_instance_key = 1;
            p->loop_forever = 1;
        }
        break;
    }
    if (self->base.instance_key > 0)
        p->loop_forever = 1;

    particle_quad_unit_corners(p->base.corners);
    self->particle = p;
    ez2_str_effect_open_for_type(self, p, self->base.type);
    self->camera_angle = (double)game_camera_heading();
    if (self->base.type >= HAT_FIRST)
        lua_ask_hat("IsAttachedToHead", "d>b", self->base.type - HAT_FIRST, &self->attached_to_head);
}

void ez2_str_effect_follow_camera(struct Ez2StrEffect *self)
{
    double now, spin;

    if (!self->particle)
        return;
    now = (double)game_camera_heading();
    if (now == self->camera_angle)
        return;
    spin = (self->camera_angle - now) + (double)self->particle->spin_degrees;
    if (0.0 > spin)
        spin += 360.0;
    else if (spin > 360.0)
        spin -= 360.0;
    self->particle->spin_degrees = (float)spin;
    self->camera_angle = now;
}

void ez2_str_effect_step_particle(struct Ez2StrEffect *self, struct Ez2StrParticle *p)
{
    Vec3 offset;

    if (!p)
        return;
    offset.x = 0.0f;
    offset.z = 0.0f;
    if (!ez2_str_effect_place_at_head(self, p) && self->base.type != 0x584)
        offset.y = -80.0f;
    else
        offset.y = 0.0f;
    ez2_str_particle_process(p, offset);
}

void ez2_str_effect_process(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    Vec3 pos;
    void *actor;

    if (self->turn_with_camera == 1 && self->particle)
        ez2_str_effect_follow_camera(self);

    pos = self->base.pos;
    self->particle->base.pos = pos;
    ez2_str_effect_step_particle(self, self->particle);
    self->particle->base.age++;

    if (self->base.type == 0x851) {
        int since = self->particle->base.age - 30;
        if (since >= 0 && since % 64 == 0) {
            struct RenderObject *player;
            pos = self->base.pos;
            player = (struct RenderObject *)game_current_player();
            if (player)
                sound_play_wave("effect\\ABC_ABYSS_SQUARE.wav",
                                player->pos.x - pos.x, 0.0f, player->pos.z - pos.z, 250, 40, 1.0f);
        }
    }

    actor = effect_find_actor(self->base.owner_aid);
    if (!actor)
        return;
    if (self->attached_to_head == 1 && game_actor_get_transform_job(actor) <= 0) {
        float scale;
        struct ActorHeadExtent extent;

        scale = session_get_baby_job_scale(&g_session1, game_actor_get_job(actor));
        if (self->adjust_position_when_shrunk == 1)
            render_object_apply_shrink_scale(actor, &scale);
        extent = actor_get_head_extent(actor);
        self->particle->screen_dx = extent.width * scale;
        self->particle->screen_dy = extent.height * scale;
    }
    self->particle->lift = ACTOR_LIFT(actor);
}

void ez2_str_effect_draw_particle(struct Ez2StrEffect *self, struct Ez2StrParticle *p)
{
    int type;

    if (!p)
        return;
    type = self->base.type;
    if (type == 0x647 || type == 0x907 || type == 0x90a || type == 0x90d) {
        self->screen_shade += 0x2000000;
        if (self->screen_shade >= 0xaa000000u)
            self->screen_shade = 0xaa000000u;
        screen_fill_rect(0, 0, RENDERER_WIDTH(g_renderer), RENDERER_HEIGHT(g_renderer), self->screen_shade);
    }
    ez2_str_particle_render_current(p);
}

void ez2_str_effect_render(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    unsigned char *actor = effect_find_actor(self->base.owner_aid);

    if (actor) {
        if (ez2_str_effect_is_headgear_type(self)) {
            if (is_effect_state_cloaking(actor_get_effect_state(actor)))
                return;
        }
        if (ez2_str_effect_is_headgear_type(self) && !ACTOR_REVEALED(actor))
            return;
        if (ez2_str_effect_is_headgear_type(self) && !ACTOR_VISIBLE(actor))
            return;
    }
    if (!self->particle)
        return;
    ez2_str_effect_draw_particle(self, self->particle);
}

void ez2_str_effect_release(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    particle_free(&self->particle->base);
}

unsigned char ez2_str_effect_is_alive(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    float limit;

    if (ez2_str_effect_is_headgear_type(self) == 1 || TYPE_IN(OWNER_BOUND_TYPES, self->base.type))
        return effect_is_owner_alive(&self->base);
    limit = PARTICLE_LIFETIME(self->particle);
    if (limit == 0.0f)
        return 1;
    return limit * 60.0f > (float)self->particle->base.age;
}

static double screen_angle_reversed(const struct Ez2StrEffect *self)
{
    float view[12];
    float x1, y1, rhw1 = 0.0f, x2, y2, rhw2 = 0.0f;
    float dx, dy, cosine, degrees;
    double angle;

    if (!game_copy_view_matrix(view))
        return 0;
    renderer_project_point(g_renderer, &self->base.pos, view, &x1, &y1, &rhw1);

    renderer_project_point(g_renderer, &self->base.target_pos, view, &x2, &y2, &rhw2);
    dx = x2 - x1;
    dy = y2 - y1;
    cosine = (float)((double)dx / (double)float_sqrt(dy * dy + dx * dx));
    degrees = (float)acos((double)-cosine) / g_pi * g_halfTurnDegrees;
    angle = (double)degrees;
    if (y2 > y1)
        angle = 360.0 - angle;
    return angle;
}

static void hat_render_order(struct Ez2StrEffect *self)
{
    unsigned char before = 0;

    if (self->base.type < HAT_FIRST)
        return;
    lua_ask_hat("IsRenderBeforeCharacter", "d>b", self->base.type - HAT_FIRST, &before);
    if (before)
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
}

#define WAVE_BEGIN_SPELL "effect\\EF_BeginSpell.wav"
#define WAVE_BEGIN_SPELL_LC "effect\\ef_beginspell.wav"

void ez2_str_effect_after_init(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    int type = self->base.type;
    double angle;
    int i;

    switch (type) {
    case 0xcc: case 0xcd: case 0xce: case 0xcf: case 0xd2:
        cue(self, "_heal_effect.wav", 0);
        break;
    case 0xd0: case 0xd1: case 0xd3:
        cue(self, "effect\\흡기.wav", 0);
        break;
    case 0x426:
        cue(self, "effect\\RL_HAMMER_OF_GOD.wav", 0);
        break;
    case 0x44d: cue(self, "effect\\su_stemspear.wav", 0); break;
    case 0x44e: cue(self, "effect\\su_tunabelly.wav", 0); break;
    case 0x44f: cue(self, "effect\\su_scratch.wav", 0); break;
    case 0x453: cue(self, "effect\\su_scaroftarou.wav", 0); break;
    case 0x455: cue(self, "effect\\su_pickypeck.wav", 0); break;
    case 0x45c: cue(self, "effect\\su_brunchofshrimp.wav", 0); break;
    case 0x463: cue(self, "effect\\su_grooming.wav", 0); break;
    case 0x464: cue(self, "effect\\su_chattering.wav", 0); break;
    case 0x466: cue(self, "effect\\su_hiss.wav", 0); break;
    case 0x467: cue(self, "effect\\su_powerofflock.wav", 0); break;
    case 0x469: cue(self, "effect\\su_svg_spirit.wav", 0); break;
    case 0x489:
        if (rand() % 12 == 0)
            effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\wizard_quagmire.wav", 0, -100.0f);
        break;
    case 0x48a:
        cue(self, "effect\\wizard_meteo.wav", 0);
        cue(self, "effect\\hunter_blastmine.wav", 0);
        cue(self, "effect\\hunter_blastmine.wav", 10);
        cue(self, "effect\\wizard_meteo.wav", 20);
        cue(self, "effect\\wizard_meteo.wav", 50);
        cue(self, "effect\\wizard_meteo.wav", 80);
        cue(self, "effect\\wizard_meteo.wav", 100);
        cue(self, "effect\\wizard_meteo.wav", 130);
        cue(self, "effect\\wizard_meteo.wav", 140);
        cue(self, "effect\\wizard_meteo.wav", 180);
        cue(self, "effect\\wizard_meteo.wav", 200);
        cue_message(self, 50);
        cue_message(self, 100);
        cue_message(self, 150);
        cue_message(self, 200);
        cue_message(self, 250);
        break;
    case 0x48b:
        cue(self, "effect\\wizard_meteor.wav", 0);
        cue_message(self, 40);
        break;
    case 0x554: case 0x5d7:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "ice_titan_attack.wav", 5);
        break;
    case 0x573:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\EF_Bash.wav", 0);
        cue(self, "effect\\rog_intimidate.wav", 35);
        break;
    case 0x580: case 0x5ef:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\EF_hit2.wav", 10);
        cue(self, "effect\\EF_hit4.wav", 20);
        break;
    case 0x583:
        cue(self, "effect\\EF_hit2.wav", 0);
        break;
    case 0x585: case 0x5fe:
        cue(self, "effect\\g크레이지위드.wav", 0);
        break;
    case 0x590:
        cue(self, "effect\\mon_맹룡과강.wav", 5);
        cue(self, "effect\\mon_맹룡과강.wav", 5);
        cue(self, "effect\\mon_맹룡과강.wav", 5);
        break;
    case 0x60a: case 0x60b: case 0x64b: case 0x64c: case 0x64d: case 0x64e: case 0x64f: case 0x650:
        set_list(self, EZ2STR_LIST_DEFAULT);
        break;
    case 0x60d:
        cue(self, "effect\\EF_Bash.wav", 0);
        break;
    case 0x60e:
        cue(self, "hell_in.wav", 0);
        break;
    case 0x615:
        cue(self, "effect\\샤프슈팅.wav", 0);
        cue(self, "effect\\흡기.wav", 0);
        break;
    case 0x619:
        cue(self, "effect\\프레셔.wav", 3);
        cue(self, "effect\\S바레티르스피어.wav", 22);
        cue_message(self, 22);
        break;
    case 0x61e:
        cue(self, "effect\\EF_StoneCurse.wav", 0);
        break;
    case 0x638:
        cue(self, "effect\\매직 크래쉬.wav", 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x647: case 0x648: case 0x907: case 0x908: case 0x90a: case 0x90b: case 0x90d: case 0x90e:
        set_list(self, EZ2STR_LIST_SCREEN_FILL);
        break;
    case 0x649:
        set_list(self, EZ2STR_LIST_SCREEN_FILL);
        cue_message(self, 180);
        break;
    case 0x666:
        cue(self, "chainlight.wav", 0);
        break;
    case 0x66c:
        cue(self, "effect\\priest_lexaeterna.wav", 0);
        break;
    case 0x676:
        cue(self, "effect\\EF_GlassWall.wav", 30);
        break;
    case 0x67e:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\DK_HACKANDSLASHER.wav", 0);
        break;
    case 0x686: case 0x687: case 0x688: case 0x689: case 0x68a:
        for (i = 0; i < type - 0x686 + 1; i++) {
            char wave[64];
            sprintf(wave, "effect\\DK_STORMSLASH%d.wav", rand() % 4 + 1);
            cue(self, wave, i * 15);
        }
        break;
    case 0x69a:
        cue(self, "effect\\wl_siennaexecrate.wav", 0);
        break;
    case 0x6a6: case 0x75d:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\wizard_fire_pillar_b.wav", 15);
        cue_message(self, 20);
        break;
    case 0x6aa:
        cue(self, "effect\\wizard_earthspike.wav", 20);
        cue_message(self, 20);
        break;
    case 0x6b0:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\DK_SERVANT_W_SIGN.wav", 40);
        break;
    case 0x6b2: cue(self, "effect\\DK_SERVANT_W_PHANTOM.wav", 0); break;
    case 0x6b7: cue(self, "effect\\DK_SERVANT_W_DEMOL.wav", 0); break;
    case 0x6ba: cue(self, "effect\\DK_DRAGONIC_AURA1.wav", 0); break;
    case 0x6bc: cue(self, "effect\\DK_DRAGONIC_AURA2.wav", 1); break;
    case 0x6c0: cue(self, "effect\\DK_MADNESS_CRUSHER.wav", 1); break;
    case 0x6c4: cue(self, "effect\\AG_DESTRUCTIVE_HURRICANE.wav", 0); break;
    case 0x6cc: {
        const struct GameActor *owner = effect_find_actor(self->base.owner_aid);
        if (owner && owner->body_state == 0 && owner->health_state == 0)
            cue(self, "effect\\AG_DEADLY_PROJECTION.wav", 0);
        break;
    }
    case 0x6d1: cue(self, "effect\\AG_ROCK_DOWN.wav", 0); break;
    case 0x6d6: cue(self, "effect\\DK_VIGOR.wav", 0); break;
    case 0x6e7: cue(self, "effect\\DK_CHARGINGPIERCE1.wav", 0); break;
    case 0x6f5: cue(self, "effect\\AG_FROZEN_SLASH.wav", 0); break;
    case 0x713: case 0x715: cue(self, "effect\\CD_ARGUTUS_VITA.wav", 0); break;
    case 0x717: cue(self, "effect\\CD_PRESENS_ACIES.wav", 0); break;
    case 0x71c:
        cue(self, "effect\\MT_D_MACHINE.wav", 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x721:
        cue(self, "effect\\CD_ARBITRIUM.wav", 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x737: cue(self, "effect\\CD_EFFLIGO.wav", 0); break;
    case 0x73a:
        cue(self, "effect\\CD_COMPETENTIA.wav", 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x73e: cue(self, "effect\\AG_CRYSTAL_IMPACT.wav", 0); break;
    case 0x744: cue(self, "effect\\CD_RELIGIO.wav", 0); break;
    case 0x748: cue(self, "effect\\SHC_FATAL_SHADOW_CROW.wav", 0); break;
    case 0x752: case 0x756: case 0x75a: case 0x77f:
        cue(self, "effect\\WH_TRAP.wav", 0);
        break;
    case 0x753: cue(self, "effect\\WH_DEEP_BLIND_TRAP_HIT.wav", 0); break;
    case 0x757: cue(self, "effect\\WH_SWIFT_TRAP_HIT.wav", 0); break;
    case 0x75b: cue(self, "effect\\WH_SOLID_TRAP_HIT.wav", 0); break;
    case 0x75e: cue(self, "effect\\MT_RUSH_QUAKE.wav", 0); break;
    case 0x75f: cue(self, "effect\\CD_BENEDICTUM.wav", 0); break;
    case 0x765:
        cue(self, "effect\\AG_STRANTUM_TREMOR1.wav", 0);
    case 0x766:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        if (self->base.level == 1)
            ez2_str_effect_set_zoom(&self->base, 0.7f);
        else if ((unsigned)(self->base.level - 2) <= 3)
            ez2_str_effect_set_zoom(&self->base, 1.25f);
        break;
    case 0x76a: cue(self, "effect\\SHC_IMPACT_CRATER.wav", 0); break;
    case 0x76b: cue(self, "effect\\WH_WIND_SIGN.wav", 0); break;
    case 0x76f:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\MT_ABR_NET_REPAIR.wav", 0);
        break;
    case 0x770:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\MT_ABR_NET_SUPPORT.wav", 0);
        break;
    case 0x771: cue(self, "effect\\WH_HAWKRUSH.wav", 0); break;
    case 0x77a: cue(self, "effect\\SHC_ETERNAL_SLASH.wav", 0); break;
    case 0x782: cue(self, "effect\\WH_FLAMETRAP_HIT.wav", 0); break;
    case 0x788:
        cue(self, "effect\\CD_DILECTIO_HEAL.wav", 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x789: cue(self, "effect\\CD_FRAMEN.wav", 0); break;
    case 0x78d: cue(self, "effect\\SHC_POTENT_VENOM.wav", 0); break;
    case 0x794:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue_message(self, 20);
        break;
    case 0x795: cue(self, "effect\\MT_AXE_STOMP.wav", 0); break;
    case 0x7a1:
        angle = effect_screen_heading(self->base.pos, self->base.target_pos);
        if (self->particle)
            self->particle->spin_degrees = (float)angle;
        break;
    case 0x7a2: cue(self, "effect\\SHC_SHADOW_STAB.wav", 0); break;
    case 0x7a8: cue(self, "effect\\WH_CALAMITYGALE.wav", 0); break;
    case 0x7ac:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\IQ_FAITH.wav", 0);
        break;
    case 0x7ae: cue(self, "effect\\ABC_ABYSS_DAGGER.wav", 0); break;
    case 0x7b9: cue(self, "effect\\IQ_FIRST_FAITH_POWER.wav", 0); break;
    case 0x7bc: cue(self, "effect\\IQ_JUDGE.wav", 0); break;
    case 0x7c0: cue(self, "effect\\IQ_THIRD_EXOR_FLAME.wav", 0); break;
    case 0x7c3: case 0x7cf: cue(self, "effect\\IQ_FAITH.wav", 0); break;
    case 0x7c4: cue(self, "effect\\IQ_OLEUM_SANCTUM.wav", 50); break;
    case 0x7c8:
        self->particle->loops = 2;
        cue(self, "effect\\EM_INCREASING_ACTIVITY.wav", 0);
        break;
    case 0x7cb: cue(self, "effect\\IQ_EXPOSION_BLASTER.wav", 0); break;
    case 0x7d0: cue(self, "effect\\TR_MYSTIC_SYMPHONY.wav", 0); break;
    case 0x7d3: cue(self, "effect\\EM_ACTIVITY_BURN.wav", 0); break;
    case 0x7d8:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\TR_KVASIR_SONATA.wav", 0);
        break;
    case 0x7d9:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\EM_SUMMON_ELEMENTAL_SERPENS_2.wav", 0);
        break;
    case 0x7e2:
        angle = screen_angle_reversed(self);
        if (self->particle)
            self->particle->spin_degrees = (float)angle;
        break;
    case 0x7e3: cue(self, "effect\\EM_SUMMON_ELEMENTAL_DILUVIO_2.wav", 0); break;
    case 0x7e9: cue(self, "effect\\EM_SPELL_ENCHANTING.wav", 0); break;
    case 0x7eb:
        PARTICLE_LIFETIME(self->particle) = (float)g_effectDurations[0x7eb] / 60.0f;
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\EM_DIAMOND_STORM.wav", 0);
        break;
    case 0x7ef:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\EM_VENOM_SWAMP.wav", 0);
        break;
    case 0x7f3: cue(self, "effect\\EM_SUMMON_ELEMENTAL_ARDOR_2.wav", 0); break;
    case 0x7f9: cue(self, "effect\\EM_LIGHTNING_LAND.wav", 0); break;
    case 0x801: cue(self, "effect\\TR_ROSEBLOSSOM_ROSE.wav", 0); break;
    case 0x802:
        angle = screen_angle_reversed(self);
        if (self->particle)
            self->particle->spin_degrees = (float)angle;
        cue(self, "effect\\TR_ROSEBLOSSOM_SHOT.wav", 0);
        break;
    case 0x806: cue(self, "effect\\IQ_FIRST_BRAND.wav", 0); break;
    case 0x807: cue(self, "effect\\EM_CONFLAGRATION.wav", 0); break;
    case 0x80c: cue(self, "effect\\TR_SOUNDBLEND_SHOT.wav", 0); break;
    case 0x80f: cue(self, "effect\\TR_SOUNDBLEND_HIT.wav", 0); break;
    case 0x810: cue(self, "effect\\TR_METALIC_FURY.wav", 0); break;
    case 0x811:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\TR_JAWAII_SERENADE.wav", 0);
        break;
    case 0x814: cue(self, "effect\\TR_PRON_MARCH.wav", 0); break;
    case 0x81b: cue(self, "effect\\TR_NIPELHEIM_REQUIEM.wav", 0); break;
    case 0x81e: cue(self, "effect\\IQ_SECOND_FAITH.wav", 20); break;
    case 0x823: cue(self, "effect\\IQ_SECOND_FLAME.wav", 0); break;
    case 0x824:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\wizard_fire_pillar_b.wav", 15);
        cue_message(self, 40);
        break;
    case 0x826:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\IQ_THIRD_FLAME_BOMB.wav", 0);
        break;
    case 0x827: cue(self, "effect\\ABC_ABYSS_STRIKE.wav", 0); break;
    case 0x82e:
        cue(self, "effect\\IQ_MASSIVE_F_BLASTER.wav", 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x831: cue(self, "effect\\IQ_THIRD_CONSECRATION.wav", 0); break;
    case 0x832: cue(self, "effect\\EM_SUMMON_ELEMENTAL_PROCELLA_2.wav", 0); break;
    case 0x838: cue(self, "effect\\EM_SUMMON_ELEMENTAL_TERREMOTUS_2.wav", 0); break;
    case 0x83e:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\ABC_FROM_THE_ABYSS.wav", 0);
        break;
    case 0x841:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\IQ_THIRD_PUNISH.wav", 0);
        break;
    case 0x844: cue(self, "effect\\TR_GEF_NOCTURN.wav", 0); break;
    case 0x847: cue(self, "effect\\TR_ROKI_CAPRICCIO.wav", 0); break;
    case 0x84a: cue(self, "effect\\TR_MUSICAL_INTERLUDE.wav", 0); break;
    case 0x84c: cue(self, "effect\\TR_AIN_RHAPSODY.wav", 0); break;
    case 0x854: cue(self, "effect\\ABC_DEFT_STAB1.wav", 0); break;
    case 0x855: cue(self, "effect\\EM_TERRA_DRIVE.wav", 0); break;
    case 0x85b: cue(self, "effect\\EM_ELEMETAL_BUSTER_FIRE.wav", 0); break;
    case 0x85e: cue(self, "effect\\EM_ELEMETAL_BUSTER_GROUND.wav", 0); break;
    case 0x861: cue(self, "effect\\EM_ELEMETAL_BUSTER_POISON.wav", 0); break;
    case 0x864: cue(self, "effect\\EM_ELEMETAL_BUSTER_WATER.wav", 0); break;
    case 0x867: cue(self, "effect\\EM_ELEMETAL_BUSTER_WIND.wav", 0); break;
    case 0x86d: cue(self, "effect\\IG_OVERSLASH.wav", 0); break;
    case 0x871: cue(self, "effect\\BO_ADVANCE_PROTECTION.wav", 0); break;
    case 0x879: case 0x87c: cue(self, "effect\\IG_GUARD_STANCE.wav", 0); break;
    case 0x87f: cue(self, "effect\\IG_GRAND_JUDGEMENT.wav", 0); break;
    case 0x885: cue(self, "effect\\IG_HOLY_SHIELD1.wav", 0); break;
    case 0x886: cue(self, "effect\\IG_HOLY_SHIELD2.wav", 34); break;
    case 0x88a: cue(self, "apocalips_attack.wav", 0); break;
    case 0x88e: cue(self, "effect\\IG_GUARDIAN_SHIELD.wav", 0); break;
    case 0x894: cue(self, "effect\\IG_REBOUND_SHIELD.wav", 0); break;
    case 0x898: cue(self, "effect\\IG_ULTIMATE_SACRIFICE.wav", 0); break;
    case 0x8bf: cue(self, "effect\\BO_HELL_HOWLING.wav", 0); break;
    case 0x8c1: cue(self, "effect\\BO_RESEARCHREPORT.wav", 0); break;
    case 0x8ca:
        self->particle->spin_degrees = (float)(effect_screen_heading(self->base.pos, self->base.target_pos) + 180.0);
        cue(self, "effect\\BO_WOODEN_ATTACK.wav", 0);
        break;
    case 0x8cc:
        self->particle->spin_degrees = (float)(effect_screen_heading(self->base.pos, self->base.target_pos) + 180.0);
        cue(self, "effect\\BO_THROWROCK.wav", 0);
        break;
    case 0x8d1: cue(self, "effect\\BO_FAIRY_DUSTY.wav", 0); break;
    case 0x8d2:
        cue(self, "effect\\ef_detoxication.wav", 0);
        cue_message(self, 0);
        break;
    case 0x8d4:
        cue(self, "effect\\EF_IceArrow1.wav", 0);
        cue_message(self, 0);
        break;
    case 0x8d6:
        cue(self, "effect\\EF_FireArrow1.wav", 0);
        cue_message(self, 0);
        break;
    case 0x8d8:
        cue(self, "effect\\EF_LightBolt3.wav", 0);
        cue_message(self, 0);
        break;
    case 0x8da:
        cue(self, "effect\\wizard_earthspike.wav", 0);
        cue_message(self, 0);
        break;
    case 0x909: case 0x90c: case 0x90f:
        set_list(self, EZ2STR_LIST_SCREEN_FILL);
        cue_message(self, 0);
        break;
    case 0x943: cue(self, "effect\\drop_effect_1.wav", 0); break;
    case 0x944: cue(self, "effect\\drop_effect_2.wav", 0); break;

    case 0x625: case 0x62a: case 0x62d: case 0x633: case 0x654: case 0x65d: case 0x663: case 0x669:
    case 0x66a: case 0x681: case 0x683: case 0x685: case 0x711: case 0x714: case 0x716: case 0x718:
    case 0x71e: case 0x723: case 0x745: case 0x746: case 0x74c: case 0x758: case 0x760: case 0x785:
    case 0x79e: case 0x7a6: case 0x7b1: case 0x7b4: case 0x7b8: case 0x7bb: case 0x7bf: case 0x86f:
    case 0x877: case 0x884: case 0x888: case 0x891: case 0x896:
        cue(self, WAVE_BEGIN_SPELL, 0);
        break;
    case 0x58f: case 0x592: case 0x5dc: case 0x610: case 0x6c7: case 0x6d4: case 0x6e2: case 0x6ee:
    case 0x6f3: case 0x705: case 0x70a: case 0x740: case 0x763: case 0x7e6: case 0x7f1: case 0x7fc:
    case 0x85a:
    case 0x671: case 0x6cb: case 0x6cf: case 0x6da: case 0x6de: case 0x6ea: case 0x6f7: case 0x70e:
    case 0x7a9: case 0x7ca: case 0x7d5: case 0x7db: case 0x7f6: case 0x835: case 0x83a: case 0x858:
    case 0x87b: case 0x87e: case 0x883: case 0x8bd: case 0x8c8: case 0x8d0:
        cue(self, WAVE_BEGIN_SPELL, 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x7d7: case 0x7de: case 0x7fe: case 0x80d: case 0x812: case 0x815: case 0x817: case 0x81c:
    case 0x829: case 0x845: case 0x848: case 0x84b: case 0x84d: case 0x84f: case 0x86b: case 0x873:
    case 0x88b: case 0x89c: case 0x8a4: case 0x8ac: case 0x8b4: case 0x8c2:
        cue(self, WAVE_BEGIN_SPELL_LC, 0);
        break;
    case 0x7d1: case 0x7ed: case 0x80a:
        cue(self, WAVE_BEGIN_SPELL_LC, 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;

    default:
        if (TYPE_IN(BEFORE_ACTORS_TYPES, type))
            set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        else
            hat_render_order(self);
        break;
    }
}

void ez2_str_effect_skip(struct Effect *effect, float fraction)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        ez2_str_particle_skip(self->particle, fraction);
}

void ez2_str_effect_set_stop_frame(struct Effect *effect, float frame)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->stop_frame = (int)frame;
}

void ez2_str_effect_set_start_delay(struct Effect *effect, int ms)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->start_delay = ms;
}

void ez2_str_effect_set_spin(struct Effect *effect, float degrees)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->spin_degrees = degrees;
}

void ez2_str_effect_set_str_name(struct Effect *effect, const char *name)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    string_assign_cstr(&self->str_name, name);
    particle_free(&self->particle->base);
    ez2_str_effect_init(&self->base);
}

void ez2_str_effect_set_list_flags(struct Effect *effect, int list_flags)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    self->particle->base.list_flags = list_flags;
}

void ez2_str_effect_set_zoom(struct Effect *effect, float zoom)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->zoom = zoom;
}

void ez2_str_effect_set_alpha_scale(struct Effect *effect, float scale)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->alpha_scale = scale;
}
