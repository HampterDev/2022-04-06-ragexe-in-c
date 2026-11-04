#include "world/mode_callbacks.h"
#include "platform/clock.h"

struct Mode *mode_construct(struct Mode *self)
{
    self->loop_cond = 1;
    self->next_sub_mode = -1;
    self->connected = 1;
    self->help_balloon = NULL;
    self->help_balloon_tick = platform_ticks_ms();
    self->cursor_action = 0;
    self->cursor_motion = 0;
    self->cursor_anim_start = platform_ticks_ms();
    self->attack_in_progress = 0;
    self->screenshot_pending = 0;
    self->cursor_offset_x = 0;
    self->cursor_offset_y = 0;
    self->fade_in_start = 0;
    return self;
}
