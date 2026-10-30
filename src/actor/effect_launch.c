#include "actor/render_ops.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "diag/diag.h"
#include "core/memory.h"
#include "actor/actor.h"
#include "game/context.h"
#include "ui/misc/rolling_cutter.h"
#include "effects/rag/rag_effect.h"

unsigned g_launchedEffectIds[32];

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

    if (effect_id < 0 || effect_id > 60000)
        return NULL;

    history[0] = effect_id;
    memcpy(&history[1], g_launchedEffectIds, 7 * sizeof(int));
    memcpy(g_launchedEffectIds, history, sizeof history);

    if (effect_id == 0x345) {
        struct RollingCutterRagEffect *cutter = mem_try_alloc(sizeof *cutter);

        effect = cutter ? &rolling_cutter_rag_effect_construct(cutter)->base : NULL;
    } else {
        effect = mem_alloc(sizeof *effect);
        if (effect)
            effect = rag_effect_construct(effect);
    }
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

struct RagEffect *game_actor_launch_effect_timed(struct GameActor *self, int effect_id, int arg)
{
    Vec3 zero = { 0, 0, 0 };
    struct RagEffect *effect = game_actor_launch_effect(self, effect_id, zero, 0.0f);

    if (effect)
        rag_effect_send_msg(effect, NULL, 0x50, arg, 0, 0);
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
