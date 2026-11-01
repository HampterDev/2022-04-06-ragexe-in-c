#include "actor/render_ops.h"
#include "actor/granny_pc.h"
#include "actor/actor.h"

static void set_action_delay(struct RenderObject *self, int action)
{
    float delay = act_res_get_delay(self->act, action);
    float d, v;

    if (delay > 0.0f)
        self->motion_delay = delay;
    d = self->motion_delay;
    if (1.0f > d) {
        self->motion_delay = 1.0f;
        d = 1.0f;
    }
    v = self->delay_factor * d;
    if (v > 0.0f) {
        self->motion_delay = v;
    }
}

static void set_action_reset(struct RenderObject *self, int action, int type)
{
    self->base_action = action;
    self->action = action;
    self->motion = 0;
    self->play_mode = type;
    self->motion_finished = 0;
    self->plays = 1.0f;
    self->delay_factor = 1.0f;
    self->script_range_on = 0;
    self->script_steps_on = 0;
    self->script_pingpong_on = 0;
}

void render_object_set_action_base(struct RenderObject *self, int action, int type)
{
    set_action_delay(self, action);
    set_action_reset(self, action, type);
}

void game_actor_set_action(struct RenderObject *object, int action, int type)
{
    struct GameActor *self = (struct GameActor *)object;

    if (self->object_type == 7) {
        int count = (int)animation_action_count(self->act);

        if (count <= action)
            return;
    }
    set_action_delay((struct RenderObject *)self, action);
    set_action_reset((struct RenderObject *)self, action, type);
    self->glide = 0;
    self->attack_motion_count = -1.0f;
}

void c3d_granny_game_actor_set_action(struct RenderObject *object, int action, int type)
{
    struct C3dGrannyGameActor *self = (struct C3dGrannyGameActor *)object;
    int anim = 0;

    self->state = 0;
    switch (action) {
    case 0:
        anim = 0;
        break;
    case 8:
        self->state = 1;
        anim = 1;
        break;
    case 0x18:
        self->state = 5;
        anim = 5;
        break;
    case 0x20:
        self->state = 0xb;
        anim = 0xb;
        break;
    case 0x28:
        self->state = 0xc;
        anim = 0xc;
        break;
    case 0x30:
        self->state = 4;
        anim = 4;
        break;
    case 0x40:
        self->state = 3;
        anim = 3;
        break;
    case 0x50:
    case 0x58:
        self->state = 2;
        anim = 2;
        break;
    }
    self->anim = anim;
    if (!self->model)
        return;
    granny_playback_stop(self->model->playback);
    self->anim_time = 0.0f;
    granny_playback_play(self->model->playback, self->anim, type == 1);
}

void granny_pc_set_action(struct RenderObject *object, int action, int loop)
{
    struct GrannyPc *self = (struct GrannyPc *)object;

    if (!self->model)
        return;
    self->state = 0;
    switch (action) {
    case 0x08: self->state = 1; break;
    case 0x10: self->state = 6; break;
    case 0x18: self->state = 5; break;
    case 0x20: self->state = 0xb; break;
    case 0x28: self->state = 0xc; break;
    case 0x30: self->state = 4; break;
    case 0x40: self->state = 3; break;
    case 0x50: case 0x58: self->state = 2; break;
    }
    granny_pc_play_state_anim(self, loop);
}
