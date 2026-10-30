#ifndef EFFECTS_SKILLS_MSG_EFFECT_H
#define EFFECTS_SKILLS_MSG_EFFECT_H

#include "effects/render_object.h"
#include "effects/skills/skill_view.h"

#define STYLE_HP_SP 0x72
#define ACTION_WARNING 1000
#define ACTION_STATE_WARNING 10000

struct MsgEffect {
    struct AbleToMakeEffect base;
    unsigned char drawn;
    struct RenderObject *owner;
    float arc_dx;
    float height_bias;
    float arc_dz;
    int arc_extra_x, arc_extra_y, arc_extra_z;
    Vec3 drift;
    int subject_aid;
    int released;
    float walked;
    float base_y;
    float base_x;
    float base_zoom;
    float last_drift_x;
    int steps;
    int step_action;
    int style;
    int value;
    int caught_up;
    int moving;
};

void msg_effect_follow(struct MsgEffect *self);

struct MsgEffect *msg_effect_construct(struct MsgEffect *self);

#endif
