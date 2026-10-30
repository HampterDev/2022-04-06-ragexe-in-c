#include "actor/object_ops.h"
#include "actor/actor.h"
#include "actor/sprite_render.h"

void render_object_render(struct GameObject *object, const float *matrix)
{
    struct RenderObject *self = (struct RenderObject *)object;

    if (self->visible) {
        render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
        render_object_render_sprite((struct RenderObject *)self, matrix, 0, 0.0f);
    }
}

void actor_set_motion_speed(struct RenderObject *self, float scale)
{
    float delay;

    delay = act_res_get_delay(self->act, self->base_action) * scale;
    if (g_replayPlaying != 0)
        delay = replay_recorder_duration_scale(replay_recorder_instance()) * delay;
    if (delay > 0.0f)
        self->motion_delay = delay;
}
