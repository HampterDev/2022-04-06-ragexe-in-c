#include "actor/render_ops.h"
#include "actor/granny_pc.h"
#include "game/context.h"

static void set_action(struct RenderObject *self, int base)
{
    float roty = self->roty;

    self->base_action = base;
    self->action = render_object_direction((struct RenderObject *)self, roty)
                   + base;
}

static void end_timed(struct RenderObject *self, int *running)
{
    timeGetTime();
    if (timeGetTime() >= self->script_end_tick) {
        *running = 0;
        self->script_end_tick = 0;
        self->play_mode = 1;
        self->motion_finished = 1;
    }
}

void render_object_update_motion_base(struct RenderObject *self)
{
    void *act;
    float frames;
    int count;

    self->action = render_object_direction(self, self->roty) + self->base_action;
    frames = (float)(timeGetTime() - self->start_tick) / 24.0f;
    act = self->act;
    if (self->state == 0x2d)
        act = res_mgr_get(g_resMgr(), "인간족\\몸통\\여\\아크비숍_여""2.Act");
    if (!act)
        return;

    if (self->script_steps_on) {
        unsigned char step = self->script_step;

        self->script_action = self->script_actions[step];
        self->script_motion = self->script_motions[step];
        if (self->script_closing) {
            set_action(self, self->script_actions[4]);
            self->motion = self->script_motions[4];
            if (!self->script_end_tick) {
                self->script_steps_on = 0;
                self->play_mode = 1;
            }
        } else {
            set_action(self, self->script_action);
            self->motion = self->script_motion;
        }
        if (!self->script_timed)
            return;
        end_timed(self, &self->script_steps_on);
        if (!self->script_closing && self->script_frame % self->script_step_frames == 0)
            self->script_step++;
        if (self->script_step > self->script_last) {
            if (self->script_loop) {
                self->script_frame++;
                self->script_step = 0;
                return;
            }
            self->script_step = self->script_last;
            self->script_closing = 1;
        }
        self->script_frame++;
        return;
    }

    if (self->script_pingpong_on) {
        if (self->script_closing) {
            set_action(self, self->script_close_action);
            self->motion = self->script_close_motion;
            if (!self->script_end_tick) {
                self->script_range_on = 0;
                self->play_mode = 1;
            }
        } else {
            set_action(self, self->script_action);
            self->motion = self->script_motion;
        }
        if (!self->script_timed)
            return;
        end_timed(self, &self->script_range_on);
        if (!self->script_closing) {
            if (self->script_phase == 3)
                self->script_motion = (unsigned char)(~(int)(frames * -2.0f / self->motion_delay) - self->script_last);
            else
                self->script_motion = (unsigned char)(int)((frames + frames) / self->motion_delay);
        }
        if (self->script_motion > self->script_last) {
            if (self->script_phase == 2) {
                self->script_frame++;
                self->script_phase = 3;
                self->script_motion = 0;
                return;
            }
            self->script_phase = 0;
            self->script_motion = self->script_last;
            self->script_closing = 1;
        }
        self->script_frame++;
        return;
    }

    if (self->script_range_on) {
        if (self->script_closing) {
            set_action(self, self->script_close_action);
            self->motion = self->script_close_motion;
            if (!self->script_end_tick) {
                self->script_range_on = 0;
                self->play_mode = 1;
            }
        } else {
            set_action(self, self->script_action);
            self->motion = self->script_motion;
        }
        if (!self->script_timed)
            return;
        end_timed(self, &self->script_range_on);
        if (!self->script_closing && self->script_frame % self->script_step_frames == 0) {
            if (self->script_loop && !self->script_close_action && self->script_close_motion) {
                self->script_motion--;
                self->script_close_motion = 0;
            } else {
                self->script_motion++;
            }
        }
        if (self->script_motion > self->script_last) {
            if (!self->script_loop) {
                self->script_closing = 1;
                self->script_motion = self->script_last;
            } else if (!self->script_close_action) {
                self->script_close_motion = 1;
                self->script_frame++;
                self->script_motion = self->script_step + 1;
                return;
            } else {
                self->script_motion = self->script_step;
            }
        }
        self->script_frame++;
        return;
    }

    if (self->motion_finished || self->motion_held) {
        count = actres_frame_count(act, self->action) - 1;
        if (self->motion > count)
            self->motion = count;
        return;
    }
    switch (self->play_mode) {
    case 0:
        count = actres_frame_count(act, self->action);
        self->motion = (int)(frames / self->motion_delay) % count;
        break;
    case 1: {
        float played;

        count = actres_frame_count(act, self->action);
        played = frames / self->motion_delay;
        self->motion = (int)played % count;
        played = played / (float)count;
        if (!(played >= 1.0f))
            return;
        self->motion = count - 1;
        if (played >= self->plays)
            self->motion_finished = 1;
        break;
    }
    }
}

void c3d_granny_game_actor_update_motion(struct RenderObject *object)
{
    struct C3dGrannyGameActor *self = (struct C3dGrannyGameActor *)object;

    float elapsed, step;

    self->motion_finished = self->state != 2;
    if (!self->model || self->state != 2 || !(1.0f > self->attack_delay_factor))
        return;
    *(GrannyClock *)self->clock_now = _GrannyGetSystemSeconds();
    elapsed = _GrannyGetSecondsElapsed((const GrannyClock *)self->clock_last, (const GrannyClock *)self->clock_now);
    step = elapsed + elapsed;
    self->anim_time = step / self->attack_delay_factor + self->anim_time;
    granny_playback_pose(self->model->playback, self->anim_time);
    *(GrannyClock *)self->clock_last = *(GrannyClock *)self->clock_now;
    if (self->anim == 2 && granny_playback_finished(self->model->playback, 2)) {
        self->anim = 0;
        granny_playback_play(self->model->playback, 0, 0);
        self->attack_delay_factor = 1.0f;
    }
}

void granny_pc_update_motion(struct RenderObject *object)
{
    struct GrannyPc *self = (struct GrannyPc *)object;

    if (self->state == 2) {
        float left = granny_playback_remaining(self->model ? self->model->playback : NULL, 2);

        self->motion_finished = 0.1f > left;
        return;
    }
    self->motion_finished = 1;
}
