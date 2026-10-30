#ifndef UI_CHARACTER_SELECT_SERVER_H
#define UI_CHARACTER_SELECT_SERVER_H

struct UIWindow;
struct UIListBox;
struct Vec;

struct UIListBox *ui_select_server_list(struct UIWindow *wnd);
struct Vec *ui_server_list_balloons(struct UIListBox *list);

struct UISelectServerWnd *ui_select_server_wnd_construct(struct UISelectServerWnd *self);
struct UIWindow *ui_select_server_wnd_new(void);

#endif
