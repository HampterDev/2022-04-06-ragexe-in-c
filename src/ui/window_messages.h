#ifndef UI_WINDOW_MESSAGES_H
#define UI_WINDOW_MESSAGES_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "platform/win_types.h"
#include <stdint.h>
struct UIWindowMgr;

#define UIWM_MSG_CHAT_LINE 1

int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type,
                                      int arg3, int arg4, int arg5, const char *title, int width, int height,
                                      int arg9);

BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id);

intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, intptr_t val1, intptr_t val2,
                                   intptr_t val3, intptr_t val4);

#endif
