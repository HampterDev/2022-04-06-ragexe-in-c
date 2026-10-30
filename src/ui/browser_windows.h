#ifndef UI_BROWSER_WINDOWS_H
#define UI_BROWSER_WINDOWS_H

#include "browser/web_browser.h"
struct UIWindowMgr;

struct WebBrowser *ui_window_mgr_new_web_browser(struct UIWindowMgr *self, int width, int height);
struct WebBrowser *ui_window_mgr_get_login_browser(struct UIWindowMgr *self, int width, int height);
struct WebBrowser *ui_window_mgr_find_web_browser_by_hwnd(struct UIWindowMgr *self, HWND hwnd);
struct WebBrowser *ui_window_mgr_find_web_browser(struct UIWindowMgr *self, HWND owner);
BYTE ui_window_mgr_add_web_browser(struct UIWindowMgr *self, HWND hwnd, struct WebBrowser *browser);
void ui_window_mgr_delete_web_browser(struct UIWindowMgr *self, HWND hwnd);
void ui_window_mgr_queue_delete_web_browser(struct UIWindowMgr *self, HWND hwnd);

#endif
