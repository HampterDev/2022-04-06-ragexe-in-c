#ifndef UI_MAIL_RODEX_BOX_H
#define UI_MAIL_RODEX_BOX_H

#include <windows.h>

struct Rodex;

void rodex_clear(struct Rodex *self);
unsigned char rodex_create_instance(void);
void rodex_destroy_instance(void);

#endif
