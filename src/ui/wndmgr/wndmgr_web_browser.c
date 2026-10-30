#include "ui/wndmgr/wndmgr.h"
#include <stdint.h>
#include <stdlib.h>
#include "core/app.h"
#include "core/msg_str.h"

static struct WmBrowser *find_browser(struct UIWindowMgr *self, HWND hwnd)
{
    unsigned key = (unsigned)(uintptr_t)hwnd;

    return tree_find(&self->browsers, &key, tree_less_uint);
}

static void add_browser(struct UIWindowMgr *self, HWND hwnd, struct WebBrowser *browser)
{
    struct WmBrowser entry;

    entry.hwnd = (unsigned)(uintptr_t)hwnd;
    entry.browser = browser;
    tree_add(&self->browsers, &entry, sizeof entry, tree_less_uint, NULL);
}

void wm_destroy_browser(struct UIWindowMgr *self, struct WebBrowser *browser)
{
    if (!browser) return;
    if (self->browser == browser) self->browser = NULL;
    if (self->active_web_browser == browser) self->active_web_browser = NULL;
    web_browser_destruct(browser);
    free(browser);
}

static struct WebBrowser *alloc_browser(void)
{
    struct WebBrowser *browser = malloc(sizeof *browser);

    return browser ? web_browser_construct(browser) : NULL;
}

struct WebBrowser *ui_window_mgr_new_web_browser(struct UIWindowMgr *self, int width, int height)
{
    struct WebBrowser *browser;

    if (self->browsers.size >= WM_WEB_BROWSER_MAX) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(0x62a), 0xff, 0, 0);
        return NULL;
    }
    browser = alloc_browser();
    if (browser == NULL) return NULL;
    if (!web_browser_create(browser, g_hMainWnd, g_hInstance, width, height)) {
        wm_destroy_browser(self, browser);
        return NULL;
    }
    add_browser(self, browser->hwnd, browser);
    return browser;
}

struct WebBrowser *ui_window_mgr_get_login_browser(struct UIWindowMgr *self, int width, int height)
{
    if (self->browser == NULL) {
        self->browser = alloc_browser();
        if (self->browser == NULL)
            return NULL;
        if (!web_browser_create(self->browser, g_hMainWnd, g_hInstance, width, height)) {
            wm_destroy_browser(self, self->browser);
            return NULL;
        }
        add_browser(self, self->browser->hwnd, self->browser);
    }
    return self->browser;
}

void ui_window_mgr_delete_web_browser(struct UIWindowMgr *self, HWND hwnd)
{
    struct WmBrowser *entry = find_browser(self, hwnd);
    struct WebBrowser *browser;

    if (entry) {
        browser = entry->browser;
        if (browser) {
            if (browser == self->browser)
                self->browser = NULL;
            wm_destroy_browser(self, browser);
        }
        tree_erase(&self->browsers, TREE_NODE_OF(entry), NULL);
        return;
    }
    browser = self->browser;
    if (browser && hwnd == browser->hwnd) {
        wm_destroy_browser(self, browser);
        self->browser = NULL;
    }
}

BYTE ui_window_mgr_add_web_browser(struct UIWindowMgr *self, HWND hwnd, struct WebBrowser *browser)
{
    struct WmBrowser *entry = find_browser(self, hwnd);

    if (entry && entry->browser)
        return 0;
    if (browser == NULL)
        return 0;
    if (entry) {
        entry->browser = browser;
        return 1;
    }
    add_browser(self, hwnd, browser);
    return 1;
}

struct WebBrowser *ui_window_mgr_find_web_browser_by_hwnd(struct UIWindowMgr *self, HWND hwnd)
{
    struct WmBrowser *entry = find_browser(self, hwnd);

    return entry ? entry->browser : NULL;
}

struct WebBrowser *ui_window_mgr_find_web_browser(struct UIWindowMgr *self, HWND owner)
{
    struct TreeNode *node;

    for (node = tree_first(&self->browsers); node != self->browsers.head; node = tree_next(node)) {
        struct WebBrowser *browser = TREE_VALUE(node, struct WmBrowser)->browser;

        if (browser && browser->owner == owner)
            return browser;
    }
    return NULL;
}

void ui_window_mgr_queue_delete_web_browser(struct UIWindowMgr *self, HWND hwnd)
{
    int id = (int)(intptr_t)hwnd;

    list_push_back(&self->browsers_to_delete, &id, sizeof id);
}
