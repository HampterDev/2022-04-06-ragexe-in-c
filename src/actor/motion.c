#include "actor/actor_ops.h"
#include "contents/sound.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/render_ops.h"
#include "actor/game_actor.h"

void game_actor_walk_motion(struct GameActor * self)
{
    int count = actres_frame_count(self->act, self->action);

    self->motion = (int)(self->walked * 0.37f * 4.0f / self->motion_delay) % count;
}

static void motion_sound(struct RenderObject *self, void *act, int motion)
{
    int event = act_res_get_motion(act, self->action, motion)->eventId;
    const char *name;
    struct String s;

    if (event == -1)
        return;
    name = act_res_get_event_name(act, event);
    string_init(&s);
    string_assign_cstr(&s, name);
    if (string_find(&s, ".wav", 0) != STRING_NPOS) {
        struct RenderObject *player =
            (struct RenderObject *)game_current_player();

        if (player)
            sound_play_wave(string_cstr(&s), self->pos.x - player->pos.x, 0.0f,
                            self->pos.z - player->pos.z, 0xfa, 0x28, 1.0f);
    }
    string_free(&s);
}

void render_object_process_sound(struct RenderObject *self)
{
    int last = self->sound_motion, motion = self->motion, done;
    void *act;
    int i, count;

    done = motion;
    if (last == motion && self->sound_action == self->base_action) {
    } else {
        if (self->sound_action != self->base_action) {
            self->sound_motion = 0;
            last = 0;
        }
        act = self->act;
        if (last < motion) {
            for (i = last + 1; i <= done; i++) {
                motion_sound(self, act, i);
                done = self->motion;
            }
            done = self->motion;
        } else if (last != motion) {
            count = actres_frame_count(act, self->action);
            last = self->sound_motion;
            if (last >= count) {
                last = count - 1;
                self->sound_motion = last;
            }
            for (i = last + 1; i < count; i++)
                motion_sound(self, act, i);
            for (i = 0; i <= self->motion; i++)
                motion_sound(self, act, i);
            done = self->motion;
        }
    }
    self->sound_motion = done;
    self->sound_action = self->base_action;
}

static void script_timer(struct RenderObject *self, int timed, int duration)
{
    self->script_end_tick = timed ? timeGetTime() + duration : 0;
    self->script_frame = 1;
}

void render_object_play_motion_steps(struct RenderObject *self, unsigned char a0, unsigned char m0,
                           unsigned char a1, unsigned char m1, unsigned char a2, unsigned char m2,
                           unsigned char a3, unsigned char m3, unsigned char a4, unsigned char m4, int timed,
                           int duration, int loop)
{
    self->script_actions[1] = a1;
    self->script_motions[1] = m1;
    self->script_actions[2] = a2;
    self->script_motions[2] = m2;
    self->script_actions[3] = a3;
    self->script_motions[3] = m3;
    self->script_actions[4] = a4;
    self->script_motions[4] = m4;
    self->script_loop = loop;
    self->script_actions[0] = a0;
    self->script_motions[0] = m0;
    self->script_action = a0;
    self->script_motion = m0;
    self->script_step = 0;
    self->script_last = 4;
    self->script_timed = timed;
    self->script_closing = 0;
    self->script_steps_on = 1;
    self->script_range_on = 0;
    self->script_pingpong_on = 0;
    script_timer(self, timed, duration);
}

void render_object_play_motion_range(struct RenderObject *self, unsigned char action,
                           unsigned char motion, unsigned char last, unsigned char close_action,
                           unsigned char close_motion, int timed, int duration, int loop)
{
    self->script_action = action;
    self->script_motion = motion;
    self->script_step = motion;
    self->script_last = last;
    self->script_close_action = close_action;
    self->script_close_motion = close_motion;
    self->script_loop = loop;
    self->script_timed = timed;
    self->script_closing = 0;
    self->script_range_on = 1;
    self->script_pingpong_on = 0;
    self->script_steps_on = 0;
    script_timer(self, timed, duration);
}

void render_object_set_force_anim_end_tick(struct RenderObject *self, int ms)
{
    self->script_end_tick = timeGetTime() + ms;
}

void render_object_set_script_step_frames(struct RenderObject *self, unsigned char frames)
{
    self->script_step_frames = frames;
}

void render_object_set_motion_delay(struct RenderObject *self, float delay)
{
    if (delay > 0.0f)
        self->motion_delay = delay;
}
