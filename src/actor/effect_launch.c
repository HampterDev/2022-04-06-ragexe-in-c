#include "actor/render_ops.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "diag/diag.h"
#include "core/memory.h"
#include "actor/actor.h"
#include "game/context.h"
#include "effects/rag/rag_effect.h"

unsigned g_launchedEffectIds[32];

/* Effect IDs of the removed 4th-class jobs, traits, ABR/Bionic summons and grade
 * enchanting, and the effect classes of removed 3rd-class and expanded-class skills.
 * Their factories, STR files and per-ID processes are gone, so a server request naming
 * one launches nothing. */
static const struct { int first, last; } k_retiredEffects[] = {
    { 0x423, 0x423 }, { 0x42d, 0x42f }, { 0x44b, 0x44c }, { 0x450, 0x450 }, { 0x452, 0x452 },
    { 0x457, 0x458 }, { 0x465, 0x465 }, { 0x468, 0x468 }, { 0x474, 0x474 }, { 0x4b0, 0x4b0 },
    { 0x4b2, 0x4b3 }, { 0x4fc, 0x4fc }, { 0x52c, 0x52c }, { 0x54d, 0x54d }, { 0x647, 0x649 },
    { 0x67d, 0x67f }, { 0x686, 0x68a }, { 0x69e, 0x6a1 }, { 0x6ae, 0x6c1 }, { 0x6c4, 0x6cc },
    { 0x6ce, 0x730 }, { 0x732, 0x732 }, { 0x736, 0x74c }, { 0x750, 0x768 }, { 0x76a, 0x77f },
    { 0x781, 0x7a3 }, { 0x7a5, 0x89a }, { 0x89c, 0x8e2 }, { 0x8e7, 0x8e8 }, { 0x8f0, 0x8fc },
    { 0x92c, 0x930 }, { 0x932, 0x933 }, { 0x93e, 0x941 }, { 0x945, 0x945 },
};

static int effect_is_retired(int effect_id)
{
    size_t i;

    for (i = 0; i < sizeof k_retiredEffects / sizeof k_retiredEffects[0]; i++)
        if (effect_id >= k_retiredEffects[i].first && effect_id <= k_retiredEffects[i].last)
            return 1;
    return 0;
}

struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id,
                                         Vec3 offset, float rotation)
{
    struct AbleToMakeEffect *me = (struct AbleToMakeEffect *)self;
    int history[8];
    struct RagEffect *effect;
    unsigned char force = 0;
    int deg;
    float s, c, x, z;
    struct List *objects = game_current_object_list();

    if (!me || !objects || (effect_id == 0xa6 && !me->is_pc))
        return NULL;

    if (effect_id < 0 || effect_id > 60000 || effect_is_retired(effect_id))
        return NULL;

    history[0] = effect_id;
    memcpy(&history[1], g_launchedEffectIds, 7 * sizeof(int));
    memcpy(g_launchedEffectIds, history, sizeof history);

    effect = mem_alloc(sizeof *effect);
    if (effect)
        effect = rag_effect_construct(effect);
    if (!effect)
        return NULL;

    client_lua_call(g_session1.luaState, "IsForceRenderEffect", "d>b", effect_id, &force);
    effect->always_draw = force;

    deg = (int)rotation;
    s = get_sin_deg(deg);
    c = get_cos_deg(deg);
    x = offset.x;
    z = offset.z;
    offset.z = z * c + x * s;
    offset.x = x * c - z * s;

    rag_effect_init(effect, (struct RenderObject *)me, effect_id, offset);
    list_push_back(objects, &effect, sizeof effect);
    list_push_back(&me->effects, &effect, sizeof effect);
    return effect;
}

struct RagEffect *able_to_make_effect_launch_show_effect(struct GameActor *self, int effect_id, int show_id)
{
    Vec3 zero = { 0, 0, 0 };
    struct RagEffect *effect = game_actor_launch_effect(self, effect_id, zero, 0.0f);

    if (effect)
        effect->show_id = show_id;
    return effect;
}
