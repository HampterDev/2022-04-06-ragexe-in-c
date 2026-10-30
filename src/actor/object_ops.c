#include "actor/object_ops.h"
#include "core/abstract_method.h"
#include "core/memory.h"

const struct GameObjectOps game_object_ops = {
    game_object_delete,
    game_object_on_process,
    game_object_send_msg,
    game_object_render_base,
    game_object_dir_of,
};

void game_object_delete(struct GameObject *object)
{
    object->ops = &game_object_ops;
    mem_free(object);
}

unsigned char game_object_on_process(struct GameObject *object)
{
    return 1;
}

unsigned char render_object_on_process(struct GameObject *object)
{
    return 0;
}

void render_object_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
}

void game_object_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
}

void game_object_render_base(struct GameObject *object, const float *view)
{
}

unsigned char game_object_abstract_process(struct GameObject *object)
{
    (void)object;
    abstract_method_called();
}
