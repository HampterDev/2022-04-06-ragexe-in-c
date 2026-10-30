#ifndef UI_WINDOW_ACCESS_H
#define UI_WINDOW_ACCESS_H

struct UIWindow;
struct UIWindowMgr;

struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *manager, int id);
struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *manager, int id);

void ui_window_mgr_add_window(struct UIWindowMgr *manager, struct UIWindow *window);
void ui_window_mgr_add_to_window_list(struct UIWindowMgr *manager, struct UIWindow *window);
void ui_window_mgr_raise_window(struct UIWindowMgr *manager, struct UIWindow *window);
void ui_window_mgr_post_quit2(struct UIWindowMgr *manager, struct UIWindow *window);

struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *manager);

struct UIWindow *ui_window_find(int id);
struct UIWindow *ui_window_open(int id);

int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
                   int a, int b, int c, int d);
void ui_window_show(struct UIWindow *window, int visible);
void ui_window_raise(struct UIWindow *window);
void ui_window_move_safe(struct UIWindow *window, int x, int y);
void ui_window_refresh(struct UIWindow *window);

#endif
