#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "session/state.h"
#include "game/context.h"
#include <stdlib.h>
#include "resources/world_records.h"
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "actor/sprite_render.h"
#include "actor/pc.h"
#include "platform/language.h"
#include "core/msg_str.h"
#include "platform/clientinfo.h"
#include "session/session_skills.h"

#define LO(v) ((int)(v))
#define HI(v) ((int)((unsigned long long)(v) >> 32))

static int draw_hidden(const struct GameActor *self)
{
    return is_effect_state_star2(self->effect_state) || effect_state_bit6(self->effect_state) || is_effect_state_burrow(self->effect_state);
}

static void draw_rand4(void)
{
    rand();
    rand();
    rand();
    rand();
}

void sound_maker_render(struct GameObject *object, const float *matrix)
{
}

void blur_pc_render(struct GameObject *object, const float *matrix)
{
    struct BlurPC *self = (struct BlurPC *)object;

    struct RenderObject *ro = (struct RenderObject *)self;

    if (draw_hidden((struct GameActor *)self))
        return;
    draw_rand4();
    if (!self->visible) {
        render_object_render_projected(ro, matrix);
        return;
    }
    if (self->state != 6 && self->state != 3)
        render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
    if (!self->revealed) {
        render_object_render_projected(ro, matrix);
        return;
    }
    if (self->single_sprite) {
        render_object_render_sprite((struct RenderObject *)ro, matrix, 0, 0.0f);
        return;
    }
    pc_render_body_layers((struct Pc *)self, matrix, 1, 2);
}

void corpse_render(struct GameObject *object, const float *matrix)
{
    struct Corpse *self = (struct Corpse *)object;

    if (!self->visible)
        return;
    render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
    render_object_render_sprite((struct RenderObject *)self, matrix, 0, self->extra_scale);
}

void hallu_npc_render(struct GameObject *object, const float *matrix)
{
    struct HalluNPC *self = (struct HalluNPC *)object;

    struct RenderObject *ro = (struct RenderObject *)self;

    if (draw_hidden((struct GameActor *)self))
        return;
    draw_rand4();
    if (game_actor_get_job((struct GameActor *)self) >= 0x1781 &&
        game_actor_get_job((struct GameActor *)self) <= 0x179e) {
        if (!self->visible) {
            render_object_render_projected(ro, matrix);
            return;
        }
        if ((self->argb & 0xff000000) == 0xff000000)
            render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
        if (self->revealed)
            render_object_render_mercenary((struct RenderObject *)ro, matrix, game_actor_get_job((struct GameActor *)self));
        else
            render_object_render_projected(ro, matrix);
        return;
    }
    if (!self->visible) {
        render_object_render_projected(ro, matrix);
        return;
    }
    if (self->state != 6 && self->state != 3)
        render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
    if (self->revealed)
        render_object_render_sprite((struct RenderObject *)ro, matrix,
                                   job_is_monster(game_actor_get_job((struct GameActor *)self)),
                                   0.0f);
    else
        render_object_render_projected(ro, matrix);
}

void hallu_pc2_render(struct GameObject *object, const float *matrix)
{
    struct HalluPC2 *self = (struct HalluPC2 *)object;

    if (draw_hidden((struct GameActor *)self))
        return;
    draw_rand4();
    if (self->visible && self->revealed)
        pc_render_body_layers((struct Pc *)self, matrix, 1, 1);
    else
        render_object_render_projected((struct RenderObject *)self, matrix);
}

void hallu_plant_render(struct GameObject *object, const float *matrix)
{
    struct HalluPlant *self = (struct HalluPlant *)object;

    render_object_render_sprite((struct RenderObject *)self, matrix, 1, 0.0f);
}

void corpse_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
}

void sound_maker_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct SoundMaker *self = (struct SoundMaker *)object;

    const struct RswSound *rec;

    if (msg != 0xe) {
        render_object_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
    rec = (const struct RswSound *)LO(par1);
    string_assign_cstr(&self->wave, rec->waveName);
    self->volume = rec->volume;
    self->area_w = rec->width;
    self->area_h = rec->height;
    self->range = rec->range;
    memcpy(&self->pos, rec->pos, sizeof rec->pos);
    self->area_reach = float_sqrt((float)(self->area_w * self->area_w + self->area_h * self->area_h));
    self->interval = (unsigned)(__int64)(rec->cycle * 1000.0f);
    self->last_play = timeGetTime() - self->interval;
}

#define COPY_MSG(self, msg, par1, par2, par3, fade_step, fade_float, colour)                                     \
    if (HI(msg) == 0 && (unsigned)LO(msg) <= 3) {                                                                \
        switch (LO(msg)) {                                                                                       \
        case 0:                                                                                                  \
            colour;                                                                                              \
            (fade_step) = 0x8000000;                                                                             \
            return;                                                                                              \
        case 1:                                                                                                  \
            (self)->argb &= ((unsigned)LO(par1) << 24) + 0xffffff;                                               \
            (fade_step) = (unsigned)LO(par2) << 24;                                                              \
            return;                                                                                              \
        case 2:                                                                                                  \
            (fade_float) = LO(par1);                                                                             \
            return;                                                                                              \
        case 3:                                                                                                  \
            (self)->motion_held = 1;                                                                             \
            return;                                                                                              \
        }                                                                                                        \
    }

void blur_pc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct BlurPC *self = (struct BlurPC *)object;

    COPY_MSG(self, msg, par1, par2, par3, self->fade_step, self->fade_param,
             render_object_set_argb((struct RenderObject *)self, 0xb4, LO(par1), LO(par2), LO(par3)))
    pc_send_msg(object, source, msg, par1, par2, par3, par4);
}

void hallu_pc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct HalluPC *self = (struct HalluPC *)object;

    COPY_MSG(self, msg, par1, par2, par3, self->fade_step, self->fade_float,
             render_object_set_argb((struct RenderObject *)self, 0xb4, LO(par1), LO(par2), LO(par3)))
    pc_send_msg(object, source, msg, par1, par2, par3, par4);
}

void hallu_pc2_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct HalluPC2 *self = (struct HalluPC2 *)object;

    COPY_MSG(self, msg, par1, par2, par3, self->fade_step, self->fade_float,
             render_object_set_argb((struct RenderObject *)self, 0xb4, 0xff, 0x37, 0x37))
    pc_send_msg(object, source, msg, par1, par2, par3, par4);
}

void hallu_npc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct HalluNPC *self = (struct HalluNPC *)object;

    COPY_MSG(self, msg, par1, par2, par3, self->fade_step, self->fade_level,
             render_object_set_argb((struct RenderObject *)self, 0xb4, LO(par1), LO(par2), LO(par3)))
    npc_send_msg(object, source, msg, par1, par2, par3, par4);
}

void pc_set_skip_arrow(struct GameActor * base_actor, unsigned char value)
{
    struct Pc * self = (struct Pc *)base_actor;

    self->skip_arrow = value;
}
void pc_set_sprite_variant(struct GameActor * base_actor, int value)
{
    struct Pc * self = (struct Pc *)base_actor;

    self->madogear_type = value;
}
