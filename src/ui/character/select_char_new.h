#ifndef UI_CHARACTER_SELECT_CHAR_NEW_H
#define UI_CHARACTER_SELECT_CHAR_NEW_H

#include <stdint.h>

struct UIWindow;
struct UIWindow *ui_new_select_char_wnd_create(void);

intptr_t ui_new_select_char_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                          int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

#endif
