#ifndef UI_MAIL_RODEX_BOX_H
#define UI_MAIL_RODEX_BOX_H

#include "platform/win_types.h"

struct Rodex;

void rodex_clear(struct Rodex *self);
unsigned char rodex_create_instance(void);
void set_hwnd_to_game_mon(void);

#endif
