#ifndef WORLD_MODE_DISPATCH_H
#define WORLD_MODE_DISPATCH_H

#include <stdint.h>

struct Mode;

intptr_t mode_send_message(struct Mode *mode, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d);
void mode_update(struct Mode *mode);

intptr_t mode_send_msg_base(struct Mode *mode, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d);

#endif
