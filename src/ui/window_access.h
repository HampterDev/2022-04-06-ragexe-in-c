#ifndef UI_WINDOW_ACCESS_H
#define UI_WINDOW_ACCESS_H

#include <stdint.h>

struct UIWindow;
struct UIWindowMgr;

struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *manager, int id);
struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *manager, int id);

void ui_window_mgr_add_window(struct UIWindowMgr *manager, struct UIWindow *window);
void ui_window_mgr_add_to_window_list(struct UIWindowMgr *manager, struct UIWindow *window);
void ui_window_mgr_raise_window(struct UIWindowMgr *manager, struct UIWindow *window);
void ui_window_mgr_post_quit2(struct UIWindowMgr *manager, struct UIWindow *window);

struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *manager);
struct UIWindow *ui_window_mgr_focus_root(struct UIWindowMgr *manager);

struct UIWindow *ui_window_find(int id);
struct UIWindow *ui_window_open(int id);

intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
                   intptr_t a, intptr_t b, intptr_t c, intptr_t d);
void ui_window_show(struct UIWindow *window, int visible);
void ui_window_raise(struct UIWindow *window);
void ui_window_move_safe(struct UIWindow *window, int x, int y);
void ui_window_refresh(struct UIWindow *window);

#endif
