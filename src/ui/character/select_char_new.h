#ifndef UI_CHARACTER_SELECT_CHAR_NEW_H
#define UI_CHARACTER_SELECT_CHAR_NEW_H

struct UIWindow;
struct UIWindow *ui_new_select_char_wnd_create(void);

int ui_new_select_char_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                          int msg, int val1, int val2, int val3, int val4);

#endif
