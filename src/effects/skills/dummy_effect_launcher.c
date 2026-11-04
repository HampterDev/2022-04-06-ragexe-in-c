#include "effects/rag/rag_effect.h"
#include "effects/skills/dummy_effect_launcher.h"
#include "effects/effect_mgr.h"
#include "effects/render_object.h"
#include "platform/clock.h"

#define MSG_START 0xe
#define DEFAULT_FRAMES 300
#define FRAMES_WITH_VALUE 0x14d

void dummy_effect_launcher_deleting_destruct(struct GameObject *object)
{
    able_to_make_effect_dc_able_to_make_effect((struct AbleToMakeEffect *)object);
    mem_free(object);
}

unsigned char dummy_effect_launcher_on_process(struct GameObject *object)
{
    struct DummyEffectLauncher *self = (struct DummyEffectLauncher *)object;
    float frames = render_object_frames_since(self->base.start_tick);
    return !(frames > (float)self->frames);
}

void dummy_effect_launcher_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct DummyEffectLauncher *self = (struct DummyEffectLauncher *)object;
    struct RenderObject *me = (struct RenderObject *)&self->base;
    int effect_id;
    struct RagEffect *own;
    Vec3 zero;

    if (msg != MSG_START) {
        able_to_make_effect_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
    me->pos = *(const Vec3 *)MSG_PTR(par1);
    me->visible = 0;
    me->start_tick = platform_ticks_ms();
    effect_id = (int)par2;
    if (effect_mgr_spawn(g_effectMgr, 0, effect_id, me->pos, 0, -1, 0, 0))
        return;

    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    own = game_actor_launch_effect((struct GameActor *)me, effect_id, zero, 0);
    self->frames = par3 ? (int)par3 : DEFAULT_FRAMES;
    if (par3 == FRAMES_WITH_VALUE) {
        Vec3 offset;

        offset.x = (float)(double)par4;
        offset.y = 0.0f;
        offset.z = 0.0f;
        if (own)
            rag_effect_send_msg(own, 0, MSG_START, (intptr_t)&offset, 0, 0);
    }
}
