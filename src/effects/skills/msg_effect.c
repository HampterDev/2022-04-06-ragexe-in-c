#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "render/math3d.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "resources/attr_queries.h"
#include "effects/skills/msg_effect.h"
#include "actor/game_actor.h"

static unsigned char g_showNumberMsgs = 1u;

#define NUMBER_DIR "이팩트\\"
#define NUMBER_SPR NUMBER_DIR "숫자.spr"
#define NUMBER_ACT NUMBER_DIR "숫자.act"

#define MSG_START 0xe
#define MSG_SET_STYLE 0x16
#define MSG_MOVE_TO 0x1d
#define MSG_OWNER_GONE 0x31
#define MSG_SET_OWNER 0x32
#define MSG_RELEASE 0x35
#define MSG_NUMBER 0x40
#define MSG_OWNER_LEAVES 0x30

struct GameObject;
struct MsgEffect;
struct RenderObject;

const struct RenderObjectOps msg_effect_ops = {
    .base = {
        .destroy = msg_effect_delete,
        .on_process = msg_effect_on_process,
        .send_msg = msg_effect_send_msg,
        .render = msg_effect_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

static void msg_effect_reset(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    self->owner = 0;
    me->base_action = 0;
    me->motion = 0;
    me->fixed_depth = 1;
    self->subject_aid = 0;
    self->style = 0;
    self->released = 0;
    self->walked = 0.0f;
    self->base_y = 1.0f;
    self->base_x = 1.0f;
    self->base_zoom = 1.0f;
    self->steps = 0;
    self->step_action = 0;
    me->depth_layer = -1;
    self->value = 0;
    self->moving = 0;
    self->caught_up = 1;
    self->arc_dx = 0.0f;
    self->height_bias = 0.0f;
    self->arc_dz = 0.0f;
    self->drift.x = 0.0f;
    self->drift.y = 0.0f;
    self->drift.z = 0.0f;
    self->drawn = 1;
}

struct MsgEffect *msg_effect_construct(struct MsgEffect *self)
{
    able_to_make_effect_construct(&self->base);
    self->base.ops = &msg_effect_ops.base;
    self->arc_dx = 0.0f;
    self->height_bias = 0.0f;
    self->arc_dz = 0.0f;
    self->arc_extra_x = 0;
    self->arc_extra_y = 0;
    self->arc_extra_z = 0;
    self->drift.x = 0.0f;
    msg_effect_reset(self);
    return self;
}

void msg_effect_leave_owner(struct MsgEffect *self)
{
    if (self->owner)
        render_object_send(self->owner, (struct RenderObject *)&self->base, MSG_OWNER_LEAVES, 0, 0, 0, 0);
}

void msg_effect_destruct(struct MsgEffect *self)
{
    self->base.ops = &msg_effect_ops.base;
    msg_effect_leave_owner(self);
    able_to_make_effect_dc_able_to_make_effect(&self->base);
}

void msg_effect_delete(struct GameObject *object)
{
    struct MsgEffect *self = (struct MsgEffect *)object;

    msg_effect_destruct(self);
    mem_free(self);
}

static void normalize2(float *v)
{
    float inv = 1.0f / float_sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] = v[0] * inv;
    v[1] = inv * v[1];
}

static float walked_from(const struct RenderObject *me, const Vec3 *from)
{
    float dx = me->pos.x - from->x;
    float dz = me->pos.z - from->z;
    return float_sqrt(dz * dz + dx * dx);
}

static void follow_close(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    struct RenderObject *owner = self->owner;
    void *attr;
    float dist;
    int dir;

    if (!owner)
        return;
    attr = game_current_map_attr();
    if (!attr)
        return;
    dist = render_object_calc_ground_dist(owner, me->pos.x, me->pos.z);
    dir = render_object_camera_dir(me->roty, 1);
    me->action = me->base_action + dir;
    if (dist > 5.0f) {
        Vec3 from = me->pos;
        float d[2];
        int count;

        d[0] = me->pos.x - owner->pos.x;
        d[1] = me->pos.z - owner->pos.z;
        normalize2(d);
        me->pos.x = d[0] * 5.0f + owner->pos.x;
        me->pos.z = d[1] * 5.0f + owner->pos.z;
        self->walked = walked_from(me, &from) + self->walked;
        count = (int)actres_frame_count(render_object_get_act_res(me), me->action);
        me->motion = (int)(self->walked * 0.23f * 4.0f / me->motion_delay) % count;
    }
    render_object_turn_toward(me, owner->pos.x, owner->pos.z);
    me->pos.y = c3d_attr_get_height(attr, me->pos.x, me->pos.z);
}

static void follow_float(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    struct RenderObject *owner = self->owner;
    void *attr;
    float dist;
    Vec3 from;

    if (!owner || self->moving)
        return;
    attr = game_current_map_attr();
    if (!attr)
        return;
    dist = render_object_calc_ground_dist(owner, me->pos.x, me->pos.z);
    me->action = me->base_action + render_object_camera_dir(me->roty, 1);

    if ((dist > 25.0f && !self->caught_up) || (25.0f >= dist && !self->caught_up)) {
        if (!(dist > 25.0f))
            self->caught_up = 1;
        from = me->pos;
        me->pos.x = (owner->pos.x - me->pos.x) / 25.0f + me->pos.x;
        me->motion = 8;
        me->pos.z = (owner->pos.z - me->pos.z) / 25.0f + me->pos.z;
        self->walked = walked_from(me, &from) + self->walked;
    } else if (dist > 12.0f && !self->moving) {
        from = me->pos;
        me->pos.x = (owner->pos.x - me->pos.x) / dist + me->pos.x;
        me->pos.z = (owner->pos.z - me->pos.z) / dist + me->pos.z;
        self->walked = walked_from(me, &from) + self->walked;
    } else if (dist > 3.0f) {
        from = me->pos;
        me->pos.x = (owner->pos.x - me->pos.x) / 20.0f + me->pos.x;
        me->pos.z = (owner->pos.z - me->pos.z) / 20.0f + me->pos.z;
        self->walked = walked_from(me, &from) + self->walked;
    }
    render_object_turn_toward(me, owner->pos.x, owner->pos.z);
    me->pos.y = c3d_attr_get_height(attr, me->pos.x, me->pos.z) - 25.0f;
}

static void follow_walk(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    struct RenderObject *owner = self->owner;
    void *attr;
    float dist;
    int dir;

    if (!owner)
        return;
    attr = game_current_map_attr();
    if (!attr)
        return;
    dist = render_object_calc_ground_dist(owner, me->pos.x, me->pos.z);
    dir = render_object_camera_dir(me->roty, 1);

    if (self->moving == 1) {
        me->action = ((dir + 4 < 8) * 8 + 12) + me->base_action + dir;
    } else if (dist > 15.0f) {
        Vec3 from = me->pos;
        float d[2], s, ox, oz;

        d[0] = me->pos.x - owner->pos.x;
        d[1] = me->pos.z - owner->pos.z;
        me->action = me->base_action + 8 + dir;
        normalize2(d);
        s = dist - 0.7f;
        if (15.0f >= s) {
            ox = d[0] * 14.9f;
            oz = d[1] * 14.9f;
        } else {
            oz = s * d[1];
            ox = s * d[0];
        }
        me->pos.x = owner->pos.x + ox;
        me->pos.z = owner->pos.z + oz;
        self->walked = walked_from(me, &from) + self->walked;
        if (owner->state == 0 && 16.0f > dist)
            me->action = me->base_action + dir;
    } else {
        int owner_state = owner->state;
        if (owner_state == 4)
            me->action = me->base_action + 0x18 + dir;
        else if (owner_state == 3)
            me->action = me->base_action + 0x20 + dir;
        else if (owner_state == 0 || 14.0f >= dist)
            me->action = me->base_action + dir;
    }
    render_object_turn_toward(me, owner->pos.x, owner->pos.z);
    me->pos.y = c3d_attr_get_height(attr, me->pos.x, me->pos.z);
}

static int sit_on_owner(struct MsgEffect *self, float up)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    if (!self->owner)
        return 0;
    me->pos = self->owner->pos;
    me->pos.y = me->pos.y - up;
    return 1;
}

void msg_effect_follow(struct MsgEffect *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    struct RenderObject *owner = self->owner;

    switch (self->style) {
    case 1: case 2: case 8: case 9: case 0xa: case 0x25: case 0x26:
        sit_on_owner(self, me->state == ACTION_STATE_WARNING ? 10.0f : 20.0f);
        return;
    case 3:
        follow_close(self);
        return;
    case 4: case 0xb:
        sit_on_owner(self, 15.0f);
        return;
    case 5:
        sit_on_owner(self, 0.0f);
        return;
    case 6:
        return;
    case 7: {
        int dir;
        if (!sit_on_owner(self, 15.0f))
            return;
        me->roty = self->owner->roty;
        dir = game_object_dir_of((struct GameObject *)me, me->roty);
        me->action = me->base_action + dir;
        me->depth_layer = (unsigned)(dir - 2) > 3 ? -3 : 3;
        return;
    }
    case 0xc:
        sit_on_owner(self, 20.0f);
        return;
    case 0xd: case 0xe: case 0xf: case 0x10: case 0x11: case 0x15: case 0x16: case 0x18: case 0x19:
        if (owner) {
            me->pos.x = owner->pos.x;
            self->drift.x = owner->pos.x;
            me->pos.z = owner->pos.z;
            self->drift.z = owner->pos.z;
        }
        me->prim_list = 0x200;
        return;
    case 0x12:
        follow_float(self);
        return;
    case 0x1a:
        sit_on_owner(self, 20.0f);
        return;
    case 0x1b: case 0x1f:
        follow_walk(self);
        return;
    case 0x1c: case 0x1d: case 0x1e: case 0x22: case 0x23: case 0x24:
        if (!sit_on_owner(self, me->state == ACTION_STATE_WARNING ? 10.0f : 20.0f))
            return;
        me->pos.x = me->pos.x - 3.0f;
        return;
    case 0x20:
        sit_on_owner(self, 25.0f);
        return;
    case 0x21:
        if (!owner)
            return;
        me->pos = owner->pos;
        me->pos.y = me->pos.y - (float)((struct GameActor *)owner)->sprite_height - 10.0f;
        return;
    }
    if (self->style == STYLE_HP_SP) {
        me->pos.x = self->owner->pos.x;
        me->pos.z = self->owner->pos.z;
    }
}

static void msg_effect_set_style(struct MsgEffect *self, int style)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    self->style = style;
    switch (style) {
    case 3: case 0x1f:
        me->fixed_depth = 0;
        me->depth_layer = 0;
        me->has_shadow = 1;
        me->shadow_scale = 1.0f;
        return;
    case 0x1b:
        me->fixed_depth = 0;
        me->depth_layer = 0;
        me->has_shadow = 1;
        me->shadow_scale = 2.0f;
        return;
    case 4:
        render_object_set_argb(me, 0x7f, 0xff, 0, 0);
        return;
    case 7:
        render_object_set_argb(me, 0x7f, -1, -1, -1);
    case 5: case 6:
        me->fixed_depth = 0;
        me->depth_layer = 5;
        return;
    case 0x12:
        me->fixed_depth = 0;
        me->depth_layer = 0;
        return;
    }
}

static void msg_effect_place(struct MsgEffect *self, const Vec3 *at)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    switch (self->style) {
    case 3: case 0x1b: case 0x1f:
        return;
    case 6:
        me->pos = *at;
        return;
    case 0xb: case 0xc:
        me->motion = 1;
        me->visible = 0;
        return;
    case 0xd:
        me->pos = *at;
        me->pos.y = me->pos.y - 12.0f;
        self->base_x = me->pos.x;
        me->motion = 0;
        self->base_y = me->pos.y;
        me->zoom = 5.0f;
        self->base_zoom = 5.0f;
        me->depth_layer = -5;
        return;
    case 0xf: case 0x19:
        me->pos = *at;
        me->pos.y = me->pos.y - 12.0f;
        me->motion = 1;
        me->zoom = 3.0f;
        self->base_zoom = 3.0f;
        self->base_y = me->pos.y;
        me->depth_layer = -5;
        me->argb = 0xffaaaaaa;
        return;
    case 0x13:
        me->pos = *at;
        me->pos.y = me->pos.y - 12.0f;
        me->motion = 0;
        me->depth_layer = -5;
        self->base_y = me->pos.y;
        return;
    case 0x18:
        me->pos = *at;
        me->pos.y = me->pos.y - 2.0f;
        me->motion = 0;
        me->zoom = 1.0f;
        self->base_zoom = 1.0f;
        self->base_y = me->pos.y;
        me->depth_layer = -5;
        return;
    }
    me->pos = *at;
    me->pos.y = me->pos.y - 20.0f;
}

static void msg_effect_number(struct MsgEffect *self, const Vec3 *at, int motion, int value, int lift, int keep_x)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    render_object_load_spr_res(me, NUMBER_SPR);
    render_object_load_act_res(me, NUMBER_ACT);
    me->base_action = 0;
    me->action = 0;
    me->pos = *at;
    me->pos.y = me->pos.y - (float)lift;
    if (keep_x)
        self->base_x = me->pos.x;
    me->visible = 1;
    self->base_y = me->pos.y;
    me->motion = motion;
    if (motion < 0 || motion > 0xb)
        me->motion = 0;
    me->shift_x = value;
    self->value = value;
    me->zoom = 5.0f;
    self->base_zoom = 5.0f;
    me->depth_layer = -2;
    me->start_tick = timeGetTime();
}

void msg_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct MsgEffect *self = (struct MsgEffect *)object;
    struct RenderObject *sender = (struct RenderObject *)source;

    struct RenderObject *me = (struct RenderObject *)&self->base;

    switch (msg) {
    case MSG_START: {
        const struct String *names = (const struct String *)MSG_PTR(par2);

        me->visible = 1;
        me->start_tick = timeGetTime();
        if (par2) {
            render_object_load_spr_res(me, string_cstr(&names[0]));
            render_object_load_act_res(me, string_cstr(&names[1]));
        }
        if (par3 == ACTION_WARNING) {
            render_object_set_action_base(me, 10, 0);
            me->state = ACTION_STATE_WARNING;
            me->argb = 0xffff0000;
        } else {
            render_object_set_action_base(me, (int)par3, 1);
        }
        msg_effect_place(self, (const Vec3 *)MSG_PTR(par1));
        return;
    }
    case MSG_SET_STYLE:
        self->subject_aid = (int)par1;
        msg_effect_set_style(self, (int)par2);
        return;
    case MSG_MOVE_TO: {
        const Vec3 *to = (const Vec3 *)MSG_PTR(par1);

        if (self->style != 0x12 && self->style != 0x1b && self->style != 0x1f)
            break;
        self->drift.x = (to->x - me->pos.x) / 10.0f;
        self->drift.y = (to->y - me->pos.y) / 10.0f;
        self->drift.z = (to->z - me->pos.z) / 10.0f;
        self->moving = 1;
        if (self->style == 0x12)
            self->caught_up = 0;
        else
            self->steps = 0;
        return;
    }
    case MSG_OWNER_GONE:
        self->owner = 0;
        switch (self->style) {
        case 6: case 0xd: case 0xe: case 0xf: case 0x10: case 0x11: case 0x15: case 0x16: case 0x19:
            return;
        }
        self->released = 1;
        return;
    case MSG_SET_OWNER:
        self->owner = sender;
        switch (self->style) {
        case 3: case 0x12: case 0x1b: case 0x1f:
            if (sender) {
                me->pos.x = sender->pos.x - get_sin_deg((int)sender->roty) * 5.0f;
                me->pos.z = self->owner->pos.z + get_cos_deg((int)self->owner->roty) * 5.0f;
            }
            return;
        case 6:
            if (sender)
                me->pos = sender->pos;
            return;
        }
        return;
    case MSG_RELEASE:
        self->released = 1;
        return;
    case MSG_NUMBER: {
        const Vec3 *at = (const Vec3 *)MSG_PTR(par1);

        switch (self->style) {
        case 0xd:
            self->base_x = me->pos.x;
            return;
        case 0xe: case 0x10: case 0x15:
            msg_effect_number(self, at, (int)par2, (int)par3, 12, 0);
            return;
        case 0x16:
            msg_effect_number(self, at, (int)par2, (int)par3, 0xf + 12, 0);
            return;
        case STYLE_HP_SP:
            msg_effect_number(self, at, (int)par2, (int)par3, 12, 1);
            return;
        }
        return;
    }
    }
    able_to_make_effect_send_msg((struct GameObject *)me, source, msg, par1, par2, par3, par4);
}

void msg_effect_render(struct GameObject *object, const float *view)
{
    struct MsgEffect *self = (struct MsgEffect *)object;

    struct RenderObject *me = (struct RenderObject *)&self->base;

    switch (self->style) {
    case 0xd: case 0xe: case 0xf: case 0x10: case 0x15: case 0x16: case STYLE_HP_SP:
        if (!g_showNumberMsgs)
            return;
        break;
    }
    if (self->drawn)
        render_object_render((struct GameObject *)me, view);
}
