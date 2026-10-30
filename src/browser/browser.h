#include "platform/code_page.h"
#ifndef BROWSER_H
#define BROWSER_H

#include "browser/web_browser.h"
#include "ui/browser_windows.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"

#include "platform/client_config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "base.h"
#include "ui/draw.h"
#include "ui/window_mgr.h"

#include "render/textdc.h"

struct UIConnectTwitterWnd;
void ui_connect_twitter_wnd_set_authorised(struct UIConnectTwitterWnd *self, BYTE authorised);
extern WNDPROC g_addressEditOldProc;

LRESULT CALLBACK web_browser_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK web_address_bar_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
char web_browser_add_address_bar(struct WebBrowser *self, int width, int height);

void ui_window_draw_text_aligned(struct UIWindow *self, int x, int y, struct String text,
                                        int width, int align, COLORREF colour, COLORREF outline, int face,
                                        int height, int arg3, BYTE outlined);
void ui_window_text_out_line(struct UIWindow *self, int x, int y,
                                     const struct String *text, int width, int align, int face, int height,
                                     COLORREF colour, BYTE bold);

#endif
