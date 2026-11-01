#ifndef WORLD_MODE_TYPES_H
#define WORLD_MODE_TYPES_H

#include "base.h"

struct UIWindow;

struct Mode {
    const struct ModeOps *ops;
    int sub_mode;
    int sub_mode_count;
    int next_sub_mode;
    int fade_in_start;
    int loop_cond;
    int connected;
    struct UIWindow *help_balloon;
    unsigned help_balloon_tick;
    unsigned cursor_anim_start;
    int attack_in_progress;
    int screenshot_pending;
    float cursor_offset_x, cursor_offset_y;
    int cursor_action, cursor_motion;
};

struct ModeOps {
    void (*on_update)(struct Mode *mode);
    int (*send_message)(struct Mode *mode, int message, int a, int b, int c, int d);
};

#endif
