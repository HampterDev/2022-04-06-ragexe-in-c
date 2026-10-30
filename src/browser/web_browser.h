#ifndef BROWSER_WEB_BROWSER_H
#define BROWSER_WEB_BROWSER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct IWebBrowser2;
struct BrowserSite;

struct WebBrowser {
    struct IWebBrowser2 *browser;
    struct BrowserSite *site;
    HWND hwnd;
    HWND browser_hwnd;
    int cursor_shown;
    BYTE address_bar;
    int screen_width, screen_height;
    HWND rebar;
    HWND address_combo;
    int rebar_height;
    int width, height;
    BYTE mouse_released;
    char *auth_url;
    HWND owner;
};

struct WebBrowser *web_browser_construct(struct WebBrowser *self);
void web_browser_destruct(struct WebBrowser *self);
int web_browser_create(struct WebBrowser *self, HWND parent, HINSTANCE instance,
                       int width, int height);
int web_browser_create_child(struct WebBrowser *self, HWND parent, HINSTANCE instance,
                      int width, int height);
int web_browser_navigate(struct WebBrowser *self, const char *url);
void web_browser_update(struct WebBrowser *self);
void web_browser_show(struct WebBrowser *self, BYTE show);
int web_browser_owns_window(struct WebBrowser *self, HWND hwnd);
void web_browser_show_address_bar(struct WebBrowser *self, BYTE show);
void web_browser_resize(struct WebBrowser *self, int width, int height);

#endif
