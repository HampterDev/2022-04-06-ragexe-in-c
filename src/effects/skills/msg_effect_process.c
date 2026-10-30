#include "actor/object_ops.h"
#include "resources/animation.h"
#include "session/state.h"
#include "game/context.h"
#include "effects/skills/msg_effect.h"
#include "game/mode_callbacks.h"

#define MSG_OWNER_TICK 0x4d
#define GM_MSG_EMOTION_ANSWERED 0x21

static float frames_now(const struct RenderObject *me)
{
    return render_object_frames_since(me->start_tick);
}

static int motion_count(struct RenderObject *me)
{
    return (int)actres_frame_count(render_object_get_act_res(me), me->action);
}

static void fade_to(struct RenderObject *me, int alpha)
{
    render_object_set_argb(me, alpha, -1, -1, -1);
}

static void shrink_zoom(struct MsgEffect *self, float frames, float floor)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    if (me->zoom > floor)
        me->zoom = self->base_zoom - frames * 0.24f;
    else if (floor > me->zoom)
        me->zoom = floor;
}

static void number_arc(struct MsgEffect *self, float frames, float speed)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float t, y;

    if (self->arc_dx == 0.0f && self->owner) {
        int facing = (int)self->owner->roty;

        if (facing >= 0 && facing < 90) {
            self->arc_dx = -speed;
            self->arc_dz = speed;
        } else if (facing >= 90 && facing < 180) {
            self->arc_dx = -speed;
            self->arc_dz = -speed;
        } else if (facing >= 180 && facing < 270) {
            self->arc_dx = speed;
            self->arc_dz = -speed;
        } else {
            self->arc_dx = speed;
            self->arc_dz = speed;
        }
    }
    t = frames / 3.0f;
    me->pos.x = t * self->arc_dx + self->drift.x + self->arc_dx * 3.0f;
    me->pos.z = self->arc_dz * t + self->drift.z + self->arc_dz * 3.0f;
    y = self->base_y + 8.0f - (2.0f - frames / 30.0f) * frames + self->height_bias;
    me->pos.y = y;
    if (y > 0.0f || 0.0f > y)
        self->height_bias = y / (y * 3.0f * 0.25f);
}

static unsigned char step_play_once(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    void *act = render_object_get_act_res(me);
    float frames = frames_now(me);
    int count = (int)actres_frame_count(act, me->action);
    float played;

    if (!count)
        return 1;
    played = frames / me->motion_delay;
    me->motion = (int)played % count;
    if (self->style == 1 && me->state == ACTION_STATE_WARNING) {
        struct Motion *motion;

        me->motion = 15;
        motion = act_res_get_motion(act, me->action, 15);
        if (motion->clip_count)
            motion->clips[0].flags = 0;
        render_object_set_argb(me, 0xff, 0xfa, 0x50, 0x50);
        return 1;
    }
    if (!(played / (float)count >= 1.0f))
        return 1;
    if (self->style == 1 && self->subject_aid == g_session1.ownAid) {
        game_send_message(GM_MSG_EMOTION_ANSWERED, 0, 0, 0, 0);
    }
    return 0;
}

static unsigned char step_play_once_fast(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);
    int count = motion_count(me);
    float played = frames * 1.2f;

    me->motion = (int)played % count;
    if (!(played / (float)count >= 1.0f))
        return 1;
    return 0;
}

static unsigned char step_loop(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);
    int count = motion_count(me);

    me->motion = (int)(frames / me->motion_delay) % count;
    return 1;
}

static unsigned char step_loop_checked(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    void *act = render_object_get_act_res(me);
    float frames = frames_now(me);
    int count = (int)actres_frame_count(act, me->action);

    if (!count)
        return 1;
    me->motion = (int)(frames / me->motion_delay) % count;
    return 1;
}

static unsigned char step_message_owner(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    int frame;

    if (!self->owner)
        return 1;
    frame = (int)frames_now(me);
    if (frame % 34 == 0)
        render_object_send(self->owner, 0, MSG_OWNER_TICK, 0, 0, 0, 0);
    return 1;
}

static unsigned char step_combat_text(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);

    me->action = me->base_action;
    me->pos.y = self->base_y - frames * 0.54f;
    fade_to(me, (int)(250.0f - frames * 3.0f));
    me->zoom = 1.0f;
    render_object_get_act_res(me);
    return frames > 80.0f ? 0 : 1;
}

static unsigned char step_number_fly(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);

    me->action = me->base_action;
    number_arc(self, frames, 0.8f);
    shrink_zoom(self, frames, 1.2f);
    fade_to(me, (int)(250.0f - frames * 3.4f));
    return frames > 70.0f ? 0 : 1;
}

static unsigned char step_number_fly_slow(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);

    me->action = me->base_action;
    number_arc(self, frames, 0.5f);
    shrink_zoom(self, frames, 1.3f);
    fade_to(me, (int)(250.0f - frames * 3.4f));
    return frames > 70.0f ? 0 : 1;
}

static unsigned char step_number_fly_animated(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);
    void *act;

    me->action = me->base_action;
    number_arc(self, frames, 0.5f);
    if (me->zoom > 1.0f)
        me->zoom = self->base_zoom - frames * 0.24f * 0.6f;
    else if (1.0f > me->zoom)
        me->zoom = 1.0f;
    act = render_object_get_act_res(me);
    if (me->motion < (int)actres_frame_count(act, me->action) - 1)
        me->motion = (int)(frames * 0.5f) % (int)actres_frame_count(act, me->action);
    fade_to(me, (int)(255.0f - frames * 3.45f));
    return frames > 70.0f ? 0 : 1;
}

static unsigned char step_number_grow(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);
    float zoom;
    int fade = 0;

    me->action = me->base_action;
    me->pos.y = self->base_y - frames * 0.1f;
    if (frames > 90.0f)
        fade = (int)((frames - 90.0f) * 8.0f);
    fade_to(me, 250 - fade);
    zoom = me->zoom;
    if (3.0f > zoom)
        me->zoom = frames * 0.18f + zoom;
    else if (zoom > 3.0f)
        me->zoom = 3.0f;
    return frames > 120.0f ? 0 : 1;
}

static unsigned char step_number_grow_blue(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);
    float zoom;
    int fade = 0;

    me->motion = 0;
    me->action = me->base_action;
    me->pos.y = self->base_y - frames * 0.1f - 15.0f;
    if (frames > 90.0f)
        fade = (int)((frames - 90.0f) * 8.0f);
    if (fade > 210)
        render_object_set_argb(me, 0, -1, -1, -1);
    else
        render_object_set_argb(me, 210 - fade, 150, 150, 255);
    zoom = me->zoom;
    if (2.0f > zoom)
        me->zoom = frames * 0.15f + zoom;
    else if (zoom > 2.0f)
        me->zoom = 2.0f;
    return frames > 120.0f ? 0 : 1;
}

static unsigned char step_kyrie_guard(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);

    me->action = me->base_action;
    if (me->motion == 0) {
        effect_play_wave_at("effect\\kyrie_guard.wav", &me->pos, 0.0f);
    }
    me->motion++;
    if (me->motion >= motion_count(me))
        me->motion = motion_count(me) - 1;
    fade_to(me, 250);
    me->zoom = 1.2f;
    self->base_zoom = 1.2f;
    return frames > 10.0f ? 0 : 1;
}

static unsigned char step_float_follower(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    int count, n;

    if (self->moving && self->owner) {
        struct RenderObject *owner = self->owner;
        float roty, drift_x;

        render_object_turn_toward(me, owner->pos.x, owner->pos.z);
        roty = me->roty - 180.0f;
        me->roty = roty;
        if (0.0f > roty) {
            roty = roty + 360.0f;
            me->roty = roty;
        }
        me->action = render_object_camera_dir(roty, 1) + 8;
        drift_x = self->drift.x;
        me->pos.x = drift_x + me->pos.x;
        n = self->steps;
        if (n <= 10 || n >= 25)
            me->pos.y = self->drift.y + me->pos.y;
        self->last_drift_x = drift_x;
        me->pos.z = self->drift.z + me->pos.z;
        if (n >= 25) {
            self->drift.y = self->drift.y * -1.9f;
            self->released = 1;
        }
        render_object_get_act_res(me);
        self->steps++;
        self->step_action = me->action;
        me->motion = 0;
        return 1;
    }

    count = motion_count(me);
    n = self->steps;
    if (n % (int)me->motion_delay == 0) {
        if (self->caught_up)
            me->motion++;
        n = 0;
    }
    if (me->motion >= count)
        me->motion = 0;
    self->step_action = me->action;
    self->steps = n + 1;
    return 1;
}

static unsigned char step_walk_follower(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    int count = motion_count(me);
    int n;

    if (!count)
        return 1;
    n = self->steps;
    me->motion = (int)((float)n / me->motion_delay) % count;
    if (self->moving == 1) {
        if (me->motion >= count - 1)
            self->moving = 0;
        if (n < 8) {
            me->pos.x = self->drift.x + me->pos.x;
            me->pos.y = self->drift.y + me->pos.y;
            me->pos.z = self->drift.z + me->pos.z;
        }
    }
    self->steps = n + 1;
    return 1;
}

static unsigned char step_hp_sp_number(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float frames = frames_now(me);

    me->action = me->base_action;
    me->pos.y = self->base_y - frames * 0.18f;
    fade_to(me, (int)(250.0f - (frames + frames)));
    shrink_zoom(self, frames, 1.0f);
    return frames > 120.0f ? 0 : 1;
}

unsigned char msg_effect_on_process(struct GameObject *object)
{
    struct MsgEffect *self = (struct MsgEffect *)object;

    struct RenderObject *me = (struct RenderObject *)&self->base;

    if (g_isSkipFrame == 0)
        return 1;

    if (self->released) {
        if (self->style != 0x12 || !self->drawn || !self->moving)
            return 0;
        if (self->steps > 0x23) {
            self->moving = 0;
            self->released = 0;
            return 1;
        }
    }

    msg_effect_follow(self);
    switch (self->style) {
    case 0xd: case 0xe: case 0xf: case 0x10: case 0x11: case 0x12: case 0x15: case 0x16: case 0x19:
    case STYLE_HP_SP:
        break;
    default:
        render_object_process_sound(me);
        break;
    }

    switch (self->style) {
    case 0: case 1: case 2: case 0x11:
        return step_play_once(self);
    case 4: case 5: case 7: case 8: case 9: case 0xa: case 0x1a: case 0x25: case 0x26:
        return step_loop(self);
    case 6:
        return step_play_once_fast(self);
    case 0xb: case 0xc:
        return step_message_owner(self);
    case 0xd:
        return step_combat_text(self);
    case 0xe: case 0x15:
        return step_number_fly(self);
    case 0xf:
        return step_number_fly_animated(self);
    case 0x10:
        return step_number_fly_slow(self);
    case 0x12:
        return step_float_follower(self);
    case 0x16:
        return step_number_grow(self);
    case 0x18:
        return step_kyrie_guard(self);
    case 0x19:
        return step_number_grow_blue(self);
    case 0x1b: case 0x1f:
        return step_walk_follower(self);
    case 0x1c: case 0x1d: case 0x1e: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24:
        return step_loop_checked(self);
    case STYLE_HP_SP:
        return step_hp_sp_number(self);
    }
    return 1;
}
