#include "render/rp_pool.h"
#include "effects/rag/rag_effect.h"

void rag_str_rewind(struct RagEffect *self)
{
    self->str_layers_done = 0;
    self->frame = 0;
    self->str_layers_to_play = str_animation_rewind(self->str_anim, self->str_key_index,
                                                   self->str_state, self->str_drawn);
}

unsigned char rag_str_step(struct RagEffect *self, float dx, float dy, float dz)
{
    struct TexDrawVertex projected;
    float view[12];

    if (!self->str_res || !self->str_anim || !self->master || !game_copy_view_matrix(view))
        return 0;
    if (self->str_layers_done >= self->str_layers_to_play) {
        if (++self->str_loops > self->str_max_loops)
            return 0;
        rag_str_rewind(self);
    }

    prim_project_vertex(&projected, view, self->master->pos);
    self->screen_pos.x = projected.x + dx;
    self->screen_pos.y = projected.y + dy;
    self->screen_pos.z = projected.z + dz;
    self->has_str_offset = !(dx == 0.0f && dy == -80.0f && dz == 0.0f);

    self->str_layers_done += str_animation_step(self->str_anim, self->frame,
            self->str_key_index, self->str_state, self->str_drawn);
    return 1;
}
