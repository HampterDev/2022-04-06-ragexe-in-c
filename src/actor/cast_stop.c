#include "actor/cast_stop.h"
#include "actor/actor.h"
#include "actor/actor_ops.h"
#include "actor/render_object.h"
#include "actor/actor_states.h"
#include "effects/effect.h"
#include "effects/effect_mgr.h"

enum {
    ACTOR_MSG_PROGRESS_STOP = 0x53,
    ACTOR_MSG_CAST_EFFECT_STOP = 0x54,
    ACTOR_MSG_TARGET_EFFECT_STOP = 0x57,
    EF_WARNING_PLANE = 0x45a,
};

void game_actor_stop_cast(struct GameActor *actor, int own)
{
    render_object_send(actor, NULL, ACTOR_MSG_CAST_EFFECT_STOP, 0, 0, 0, 0);
    render_object_send(actor, NULL, ACTOR_MSG_TARGET_EFFECT_STOP, 0, 0, 0, 0);
    render_object_send(actor, NULL, ACTOR_MSG_PROGRESS_STOP, 0, 0, 0, 0);
    if (!own)
        effect_mgr_remove_type(g_effectMgr, actor->aid, EF_WARNING_PLANE);
    if (actor->state == ACTOR_STATE_CASTING)
        actor_set_state(actor, 0);
}
