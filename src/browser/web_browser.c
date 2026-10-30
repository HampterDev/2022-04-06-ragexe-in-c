#include "platform/client_config.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "ui/window_access.h"
#include "browser/browser.h"
#include "browser/browser_address.h"
#include "browser/browser_site.h"
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#include "core/app.h"
#include "core/msg_str.h"
#include "ui/wndmgr/wndmgr.h"

WNDPROC g_addressEditOldProc;

#define WEB_ADDRESS_COMMAND 0x3e8
#define WEB_CLASS_NAME      "ROWebBrowser"

static void restore_mouse(void)
{
    POINT pt;

    GetCursorPos(&pt);
    ScreenToClient(g_hMainWnd, &pt);
    g_session1.mouseExclusiveOption = 1;
    mouse_set_x(&g_mouse, pt.x);
    mouse_set_y(&g_mouse, pt.y);
    SetActiveWindow(g_hMainWnd);
    app_activate(1);
}

static void release_mouse(struct WebBrowser *self)
{
    POINT pt;

    if (!g_session1.mouseExclusiveOption)
        return;
    pt.x = g_mouse.x;
    pt.y = g_mouse.y;
    g_session1.mouseExclusiveOption = 0;
    mouse_set_x(&g_mouse, pt.x);
    mouse_set_y(&g_mouse, pt.y);
    self->mouse_released = 1;
    ClientToScreen(g_hMainWnd, &pt);
    SetCursorPos(pt.x, pt.y);
}

static void hide_cursor(struct WebBrowser *self)
{
    SetCursor(NULL);
    mouse_reset_buttons(&g_mouse);
    ShowCursor(FALSE);
    self->cursor_shown = 0;
}

static void show_cursor(struct WebBrowser *self)
{
    SetCursor(LoadCursorA(NULL, IDC_ARROW));
    mouse_center(&g_mouse);
    mouse_set_x(&g_mouse, -100);
    mouse_set_y(&g_mouse, -100);
    mouse_reset_buttons(&g_mouse);
    ShowCursor(TRUE);
    self->cursor_shown = 1;
}

static void update_cursor(struct WebBrowser *self)
{
    POINT pt;

    GetCursorPos(&pt);
    if (g_hMainWnd != WindowFromPoint(pt)) {
        if (self->cursor_shown != 1)
            show_cursor(self);
    } else if (self->cursor_shown) {
        hide_cursor(self);
    }
}

static void release_browser(struct WebBrowser *self)
{
    IWebBrowser2 *browser = self->browser;
    HWND hwnd = self->hwnd;
    self->browser = NULL;
    self->hwnd = NULL;
    if (browser) {
        browser->lpVtbl->Stop(browser);
        SetWindowLongA(self->browser_hwnd, GWL_STYLE, WS_CHILD);
        SetParent(self->browser_hwnd, g_hMainWnd);
        browser->lpVtbl->Quit(browser);
        browser->lpVtbl->Release(browser);
    }
    if (hwnd) DestroyWindow(hwnd);
    self->browser_hwnd = NULL;
    self->rebar = NULL;
    self->address_combo = NULL;
}

struct WebBrowser *web_browser_construct(struct WebBrowser *self)
{
    if (!self) return NULL;
    memset(self, 0, sizeof *self);
    self->screen_width = GetSystemMetrics(SM_CXSCREEN);
    self->screen_height = GetSystemMetrics(SM_CYSCREEN);
    OutputDebugStringA("CWebBrowser() \n");
    release_mouse(self);
    return self;
}

void web_browser_destruct(struct WebBrowser *self)
{
    if (!self) return;
    OutputDebugStringA("~CWebBrowser() \n");
    if (self->mouse_released) {
        self->mouse_released = 0;
        restore_mouse();
    }
    if (self->cursor_shown) hide_cursor(self);
    if (self->site) {
        struct BrowserSite *site = self->site;
        self->site = NULL;
        browser_site_close(site);
    }
    release_browser(self);
    free(self->auth_url);
    self->auth_url = NULL;
}

static void show_creation_error(HRESULT error)
{
    char *text = NULL;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                   (DWORD)error, 0, (LPSTR)&text, 0, NULL);
    if (text) {
        OutputDebugStringA(text);
        LocalFree(text);
    }
}

static int create_control(struct WebBrowser *self)
{
    HRESULT hr = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                                   &IID_IWebBrowser2, (void **)&self->browser);
    if (FAILED(hr)) {
        OutputDebugStringA("error : MakeWebBrowserInterface_InProc FAILED \n");
        show_creation_error(hr);
        return 0;
    }
    if (self->browser)
        self->browser->lpVtbl->get_HWND(self->browser, (SHANDLE_PTR *)&self->browser_hwnd);
    return 1;
}

static void register_class(HINSTANCE instance)
{
    WNDCLASSEXA wc;

    wc.cbSize = sizeof wc;
    wc.style = 0;
    wc.lpfnWndProc = web_browser_wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(0x77));
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WEB_CLASS_NAME;
    wc.hIconSm = NULL;
    RegisterClassExA(&wc);
}

static void adopt_browser_window(struct WebBrowser *self)
{
    UpdateWindow(self->hwnd);
    SetParent(self->browser_hwnd, self->hwnd);
    SetWindowLongA(self->browser_hwnd, GWL_STYLE, WS_CHILD);
}

static int create_browser_window(struct WebBrowser *self, HWND parent, HINSTANCE instance,
                                 int width, int height, int child)
{
    RECT rect;
    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    int x = 0, y = 0;
    if (!self || !create_control(self)) return 0;
    self->owner = parent;
    register_class(instance);
    if (child) {
        style = (style & ~WS_POPUP) | WS_CHILD;
        if (!GetClientRect(g_hMainWnd, &rect)) return 0;
        x = rect.right / 2 - width / 2;
        y = rect.bottom / 2 - height / 2;
    }
    self->hwnd = CreateWindowExA(0, WEB_CLASS_NAME, g_windowClassName, style, x, y, width, height,
                                 parent, NULL, instance, NULL);
    if (!self->hwnd) return 0;
    adopt_browser_window(self);
    if (FAILED(browser_site_open(self->hwnd, self->browser, &self->site))) return 0;
    if (!child) {
        if (!GetClientRect(self->hwnd, &rect)) return 0;
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }
    web_browser_resize(self, width, height);
    return web_browser_add_address_bar(self, width, 22);
}

int web_browser_create(struct WebBrowser *self, HWND parent, HINSTANCE instance, int width, int height)
{
    return create_browser_window(self, parent, instance, width, height, 0);
}

int web_browser_create_child(struct WebBrowser *self, HWND parent, HINSTANCE instance, int width, int height)
{
    return create_browser_window(self, parent, instance, width, height, 1);
}

int web_browser_rebar_height(struct WebBrowser *self)
{
    RECT rc;

    if (!self->address_bar || !self->rebar)
        return 0;
    GetClientRect(self->rebar, &rc);
    return rc.bottom - rc.top + 4;
}

char web_browser_add_address_bar(struct WebBrowser *self, int width, int height)
{
    REBARINFO bar = {0};
    REBARBANDINFOA band = {0};
    HWND combo;
    RECT rc;

    self->rebar = CreateWindowExA(WS_EX_TOOLWINDOW, "ReBarWindow32", "rebar", 0x56809648, 0, 0, width, 0,
                                  self->hwnd, NULL, GetModuleHandleA(NULL), NULL);
    if (!self->rebar)
        return 0;
    bar.cbSize = sizeof bar;
    bar.fMask = 0;
    bar.himl = NULL;
    if (!SendMessageA(self->rebar, RB_SETBARINFO, 0, (LPARAM)&bar))
        return 0;
    combo = CreateWindowExA(WS_EX_CLIENTEDGE, "COMBOBOX", "addressbar", 0x50000002, 10, 10, 0xa0, 0x16, self->rebar,
                            NULL, GetModuleHandleA(NULL), NULL);
    if (!combo) return 0;
    band.hwndChild = combo;
    band.cbSize = sizeof band;
    band.fMask = RBBIM_STYLE | RBBIM_TEXT | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_BACKGROUND;
    band.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
    band.cyMinChild = height;
    band.lpText = "";
    band.cxMinChild = 0;
    band.cx = 0xa0;
    if (!SendMessageA(self->rebar, RB_INSERTBANDA, (WPARAM)-1, (LPARAM)&band))
        return 0;
    self->address_combo = combo;
    g_addressEditOldProc = (WNDPROC)SetWindowLongA(GetWindow(combo, GW_CHILD), GWL_WNDPROC, (LONG)web_address_bar_proc);
    GetClientRect(self->hwnd, &rc);
    SetWindowPos(self->rebar, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE);
    ShowWindow(self->rebar, SW_HIDE);
    self->rebar_height = web_browser_rebar_height(self);
    return 1;
}

LRESULT CALLBACK web_address_bar_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_KEYDOWN) {
        if (wparam == VK_RETURN && g_windowMgr.active_web_browser) {
            SendMessageA(g_windowMgr.active_web_browser->hwnd, WM_COMMAND, WEB_ADDRESS_COMMAND, 0);
            return 0;
        }
    } else if (msg - WM_KEYUP <= 1 && wparam == VK_RETURN) {
        return 0;
    }
    return CallWindowProcA(g_addressEditOldProc, hwnd, msg, wparam, lparam);
}

void web_browser_resize(struct WebBrowser *self, int width, int height)
{
    IWebBrowser2 *browser = self->browser;
    int control_width, control_height, bar;
    RECT rc;

    if (!browser)
        return;
    control_width = self->screen_width == width ? width - 1 : width;
    control_height = self->screen_height == height ? height - 1 : height;
    self->width = control_width;
    if (!self->address_bar || !self->rebar) {
        bar = 0;
    } else {
        GetClientRect(self->rebar, &rc);
        bar = rc.bottom - rc.top + 4;
        browser = self->browser;
        control_width = self->width;
    }
    self->height = control_height - bar;
    browser->lpVtbl->put_Width(browser, control_width);
    self->browser->lpVtbl->put_Height(self->browser, self->height);
    if (self->rebar)
        SetWindowPos(self->rebar, NULL, 0, 0, width, height, SWP_NOMOVE);
}

int web_browser_navigate(struct WebBrowser *self, const char *url)
{
    char *prefix = NULL;
    BSTR address;
    VARIANT empty;
    HRESULT hr;

    if (!self || !self->browser || !url) return 0;
    address = browser_address_to_wide(url);
    if (!address) return 0;
    if (!browser_address_prefix(url, &prefix)) {
        SysFreeString(address);
        return 0;
    }
    free(self->auth_url);
    self->auth_url = prefix;
    VariantInit(&empty);
    hr = self->browser->lpVtbl->Navigate(self->browser, address, &empty, &empty, &empty, &empty);
    SysFreeString(address);
    ShowWindow(self->hwnd, SW_SHOW);
    self->browser->lpVtbl->put_Visible(self->browser, VARIANT_TRUE);
    if (self->address_combo) SetWindowTextA(self->address_combo, url);
    if (FAILED(hr)) OutputDebugStringA("failed : Navigate() \n");
    return SUCCEEDED(hr);
}

void web_browser_update(struct WebBrowser *self)
{
    long width = 0, height = 0;
    BSTR location = NULL;
    char *text;
    enum BrowserAuthStatus status;
    struct UIWindow *sns;
    HRESULT hr;

    if (!self || !self->browser || !IsWindowVisible(self->hwnd)) return;
    self->browser->lpVtbl->get_Width(self->browser, &width);
    self->browser->lpVtbl->get_Height(self->browser, &height);
    if (self->width != width || self->height != height)
        web_browser_resize(self, self->width, self->height);
    update_cursor(self);
    if (!self->auth_url) return;
    hr = self->browser->lpVtbl->get_LocationURL(self->browser, &location);
    if (FAILED(hr) || !location) {
        SysFreeString(location);
        return;
    }
    text = browser_address_to_text(location, CP_THREAD_ACP);
    SysFreeString(location);
    if (!text) return;
    status = browser_address_auth_status(text, self->auth_url);
    free(text);
    if (status == BROWSER_AUTH_PENDING) return;
    sns = ui_window_find(0x11a);
    if (status == BROWSER_AUTH_SUCCESS) {
        ui_window_mgr_queue_delete_web_browser(&g_windowMgr, self->hwnd);
        g_session1.snsAuthorised = 1;
        if (sns) ui_connect_twitter_wnd_set_authorised((struct UIConnectTwitterWnd *)sns, 1);
    } else if (status == BROWSER_AUTH_FAILED) {
        ui_window_mgr_modeless_message_box(&g_windowMgr, "sns auth failed", 0, 1, 0, "message", 0x118, 0x78, 0);
        ui_window_mgr_queue_delete_web_browser(&g_windowMgr, self->hwnd);
        g_session1.snsAuthorised = 0;
        if (sns) ui_connect_twitter_wnd_set_authorised((struct UIConnectTwitterWnd *)sns, 0);
    } else {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0xca1), 0xff, 0, 0);
        ui_window_mgr_queue_delete_web_browser(&g_windowMgr, self->hwnd);
    }
}

void web_browser_show(struct WebBrowser *self, BYTE show)
{
    if (show == 1) {
        ShowWindow(self->hwnd, SW_SHOW);
        self->browser->lpVtbl->put_Visible(self->browser, VARIANT_TRUE);
    } else {
        ShowWindow(self->hwnd, SW_HIDE);
        self->browser->lpVtbl->put_Visible(self->browser, VARIANT_FALSE);
    }
}

int web_browser_owns_window(struct WebBrowser *self, HWND hwnd)
{
    return self && (self->hwnd == hwnd || self->hwnd == GetParent(hwnd));
}

void web_browser_show_address_bar(struct WebBrowser *self, BYTE show)
{
    RECT bar_rc, client_rc;
    int top;

    if (!self->browser)
        return;
    self->address_bar = show;
    if (!self->rebar)
        return;
    self->browser->lpVtbl->put_Visible(self->browser, VARIANT_FALSE);
    ShowWindow(self->rebar, self->address_bar ? SW_SHOW : SW_HIDE);
    if (!self->address_bar || !self->rebar) {
        top = 0;
    } else {
        GetClientRect(self->rebar, &bar_rc);
        top = bar_rc.bottom - bar_rc.top + 4;
    }
    self->browser->lpVtbl->put_Top(self->browser, top);
    GetClientRect(self->hwnd, &client_rc);
    web_browser_resize(self, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top);
    self->browser->lpVtbl->put_Visible(self->browser, VARIANT_TRUE);
}

LRESULT CALLBACK web_browser_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct WebBrowser *wb = ui_window_mgr_find_web_browser_by_hwnd(&g_windowMgr, hwnd);
    char text[0x200];

    switch (msg) {
    case WM_CREATE:
    case WM_NCHITTEST:
    case WM_NCLBUTTONDOWN:
    case WM_PARENTNOTIFY:
        g_windowMgr.active_web_browser = ui_window_mgr_find_web_browser_by_hwnd(&g_windowMgr, hwnd);
        break;
    case WM_DESTROY:
        SetFocus(g_hMainWnd);
        break;
    case WM_SIZE:
        if (wb)
            web_browser_resize(wb, LOWORD(lparam), (int)((DWORD)lparam >> 16));
        break;
    case WM_CLOSE:
        ui_window_mgr_delete_web_browser(&g_windowMgr, hwnd);
        break;
    case WM_HELP:
        return 1;
    case WM_KEYDOWN:
        if (wparam == VK_F1 || wparam == VK_F11)
            return 1;
        break;
    case WM_COMMAND:
        if (LOWORD(wparam) != WEB_ADDRESS_COMMAND || !wb)
            break;
        GetWindowTextA(wb->address_combo, text, sizeof text);
        web_browser_navigate(wb, text);
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}
