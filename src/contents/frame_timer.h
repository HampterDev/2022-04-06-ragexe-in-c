#ifndef CONTENTS_FRAME_TIMER_H
#define CONTENTS_FRAME_TIMER_H

void frame_timer_start(unsigned rate);
void frame_timer_reset(void);
int frame_timer_step_count(void);

extern int g_frameskip;

#endif
