#ifndef WORLD_MODE_DISPATCH_H
#define WORLD_MODE_DISPATCH_H

struct Mode;

int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d);
void mode_update(struct Mode *mode);

int mode_send_msg_base(struct Mode *mode, int message, int a, int b, int c, int d);

#endif
