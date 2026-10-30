#include <stdlib.h>
#include "effects/rag/rag_effect.h"

unsigned char prim_bounce_process(struct EffectPrim *self)
{
    struct C3dAttr *ground_map = game_current_map_attr();
    const Vec3 *at = &self->master->base.pos;
    float speed;

    self->master_pos.x = at->x + self->delta2.x;
    self->master_pos.y = at->y + self->delta2.y;
    self->master_pos.z = at->z + self->delta2.z;
    self->fall_speed = self->gravity + self->fall_speed;
    speed = self->accel + self->speed;
    self->speed = (0.0f > speed) ? 0.0f : speed;
    self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->speed);
    self->delta.y = self->delta.y + self->fall_speed;
    self->delta.x = self->velocity.x + self->delta.x;
    self->delta.z = self->velocity.z + self->delta.z;
    self->base.pos.x = self->delta.x + self->master_pos.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;
    self->scale = self->scale_speed + self->scale;
    if (self->fall_speed > 0.0f && ground_map) {
        float ground = c3d_attr_get_height(ground_map, self->base.pos.x, self->base.pos.z);

        if (self->base.pos.y > ground) {
            self->fall_speed = self->fall_speed / -1.5f;
            if (self->bounce_clink == 1) {
                char wave[0x34];

                sprintf(wave, "effect\\EF_Coin%d.wav", prim_rand_below(3) + 1);
                effect_play_wave_at(wave, &self->base.pos, 0.0f);
            }
        }
    }
    prim_update_alpha(self);
    prim_shift_trail(self);
    return prim_step_motion(self) != 0;
}
