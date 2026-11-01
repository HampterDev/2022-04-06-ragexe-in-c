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
