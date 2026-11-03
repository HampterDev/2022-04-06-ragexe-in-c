/* The window rows of the login and character-selection windows, the change-name window and
   the Lua-described windows (hotkeys 0x9c, quick slot 0x9f, store search 0xb5), with their
   factories and closes; window_table.h describes the table. */
#include "ui/character/select_char_new.h"
#include "render/renderer.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/wndmgr/make_window_internal.h"
#include "ui/wndmgr/window_registry.h"
#include "session/session_state.h"
#include "core/object_type_tables.h"
#include "session/session_options.h"
#include "ui/character/character.h"
#include "ui/character/sel_char_userver.h"
#include "ui/character/select_server.h"
#include "ui/chat/chat.h"
#include "ui/hotkeys/hotkey_bars.h"
#include "ui/quests/quests.h"
#include "ui/shops/shops.h"

static void wm_move_login(struct UIWindow *wnd)
{
    int y = scale_y_from480(0x12c);

    wm_move(wnd, ui_center_x640(0xb9), y);
}

static struct UIWindow *make_select_char(struct UIWindowMgr *self, int window_id)
{
    static const char bg[] = UI_SKIN "select_character_ver3\\bg_back1.tga";
    struct UIWindow *wnd, *backdrop;
    struct BitmapRes *res;

    wnd = ui_new_select_char_wnd_create();
    self->new_select_char_wnd = wnd;
    backdrop = (struct UIWindow *)uirp_image_wnd_new(bg, 5);
    res = ui_skin_bitmap(bg);
    if (backdrop && res) {
        ui_window_create(backdrop, res->width, res->height);
        wm_move(backdrop, wm_center_x(backdrop->width), wm_center_y(backdrop->height));
        ui_window_mgr_add_window(self, backdrop);
        ui_window_create(wnd, backdrop->width, backdrop->height);
        wm_move(wnd, wm_center_x(backdrop->width), wm_center_y(backdrop->height));
        ui_window_mgr_add_window(self, wnd);
    }
    own_states_clear();
    skill_cooldowns_clear(&g_session1.skillDelays);
    return wnd;
}

static struct UIWindow *make_make_char(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd, *backdrop;

    wnd = ui_new_make_char_wnd_new();
    backdrop = (struct UIWindow *)uirp_image_wnd_new(UI_SKIN "make_character_ver2\\bg_back2.tga", 1);
    ui_window_create(backdrop, 0x404, 0x300);
    wm_move(backdrop, wm_center_x(0x404) - 2, wm_center_y(0x300));
    ui_window_mgr_add_window(self, backdrop);
    ui_window_create(wnd, 0x31a, 0x1a6);
    wm_move(wnd, wm_center_x(0x31a), wm_center_y(0x1a6) - 0x46);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

static struct UIWindow *make_lua_window(struct UIWindowMgr *self, int window_id)
{
    int width = 0, height = 0, x = 0, y = 0;
    int at_x, at_y;
    struct WindowInfo *closed;
    struct String name_x, name_y;
    struct UIWindow *wnd;

    if (!client_lua_call(g_session1.luaState, "GetWindowInfo", "d>dddd", window_id, &width, &height, &x, &y))
        return NULL;
    x = g_renderer->width * x / 1024;
    y = g_renderer->height * y / 768;
    wnd = ui_window_mgr_get_window(self, window_id);
    string_init(&name_y);
    string_init(&name_x);
    if (wnd)
        goto done;
    switch (window_id) {
    case 0x9c:
        wnd = ui_hot_key_wnd_new();
        self->hot_key_wnd = wnd;
        break;
    case 0x9f:
        wnd = ui_quik_slot_wnd_new();
        break;
    case 0xb5:
        wnd = ui_serach_store_info_wnd_new();
        g_session1.storeSearchOpenOnEnter = 1;
        break;
    }
    closed = ui_window_mgr_get_closed_rect(self, window_id);
    if (closed) {
        at_x = closed->x;
        at_y = closed->y;
    } else {
        at_x = session_get_option_int(&g_session1, string_cstr(&name_x), x);
        at_y = session_get_option_int(&g_session1, string_cstr(&name_y), y);
    }
    wnd->id = window_id;
    ui_window_create(wnd, width, height);
    if (at_x >= 0 && at_x <= g_renderer->width - wnd->width && at_y >= 0 &&
        at_y <= g_renderer->height - wnd->height) {
        x = at_x;
        y = at_y;
    }
    wm_move(wnd, x, y);
    ui_window_mgr_add_window(self, wnd);
    if (window_id == 0x9f)
        wm_set_info(wnd, &self->quick_slot_info);
    else if (window_id == 0xd4)
        wm_set_info(wnd, &self->guild_helper_info);
done:
    string_free(&name_x);
    string_free(&name_y);
    return wnd;
}

/* 0x2 */
static struct UIWindow *make_select_server_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_select_server_wnd_new();
    ui_window_create(wnd, 0x118, 0xc8);
    {
        int y = scale_y_from480(0x12c) - 0x50;

        wm_move(wnd, ui_center_x640(0xb9), y);
    }
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x3 */
static struct UIWindow *make_login_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_login_wnd_new();
    self->login_wnd = wnd;
    ui_window_create(wnd, 0x12d, 0x84);
    wm_move_login(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x6 */
static struct UIWindow *make_wait_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_wait_wnd_new();
    ui_window_create(wnd, 0x118, 0x78);
    wm_move_login(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x7 */
static struct UIWindow *make_loading_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->loading_wnd;
    if (wnd == NULL) {
        wnd = ui_loading_wnd_new();
        self->loading_wnd = wnd;
        ui_window_create(wnd, 0xf0, 0xf);
        wm_move(wnd, g_renderer->width / 2 - 0x64, g_renderer->height / 2 + 0xa0);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x7f */
static struct UIWindow *make_sel_char_for_userver_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;
    int width = g_session1.charCount > 0xc ? 0x30f : 0x21c;

    wnd = ui_sel_char_for_u_server_wnd_new();
    self->sel_char_for_userver_wnd = wnd;
    ui_window_create(wnd, width, 0x190);
    wm_move(wnd, wm_center_x(width), wm_center_y(0x190));
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x9c, 0x9f, 0xb5 */
static struct UIWindow *make_shown_lua_window(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = make_lua_window(self, window_id);
    if (wnd == NULL)
        return NULL;
    wm_show(wnd);
    return wnd;
}

/* 0x9c, 0x9f, 0xab, 0xb5: post_quit1 */
static int close_lua_window(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd;
    int closed = 0;

    if (ui_window_mgr_get_window(self, window_id) == NULL)
        return closed;
    wnd = ui_window_mgr_get_window(self, window_id);
    closed = 1;
    if (wnd == NULL)
        return closed;
    if (window_id == 0x9c) {
        ui_window_mgr_post_quit2(self, self->hot_key_wnd);
        self->hot_key_wnd = NULL;
    } else if (window_id == 0xb5) {
        ui_window_mgr_post_quit2(self, wnd);
        session_reset_store_search(&g_session1);
    } else {
        ui_window_mgr_post_quit2(self, wnd);
    }
    return closed;
}

/* 0x145 */
static struct UIWindow *make_rpimage_wnd2(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (ui_window_mgr_get_window(self, 0x145)) {
        wnd = ui_window_mgr_get_window(self, 0x145);
        return wnd;
    }
    wnd = uirp_image_wnd2_new();
    self->rpimage_wnd2 = wnd;
    ui_window_create(wnd, 0x12d, 0x84);
    wm_move_login(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x14c */
static struct UIWindow *make_disconnected_server_msg_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_disconnected_server_msg_wnd_new();
    ui_window_create(wnd, 0x118, 0x78);
    wm_move_login(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* The rows of the login, character-selection and Lua-described windows.
   window_table.h describes the columns. In ID order. */
static const struct WmWindowRow k_login_rows[] = {
    WM_ROW(0x2, WM_NO_SLOT, WM_UNTRACKED, make_select_server_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x3, WM_SLOT(login_wnd), WM_UNTRACKED | WM_GET_SLOT, make_login_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x6, WM_NO_SLOT, WM_UNTRACKED, make_wait_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x7, WM_SLOT(loading_wnd), WM_UNTRACKED, make_loading_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x7f, WM_SLOT(sel_char_for_userver_wnd), WM_UNTRACKED | WM_FORGOTTEN_BY_QUIT, make_sel_char_for_userver_wnd, WM_CLOSE_WINDOW),
    WM_ROW_OPEN(0x80, change_name_wnd, WM_UNTRACKED | WM_KEPT_BY_CLEAR | WM_FORGOTTEN_BY_QUIT, WM_CLOSE_SLOT, ui_change_name_wnd_new, 0xb8, 0x69, WM_AT_CENTER, 0x104, 0xb4),
    WM_ROW(0x9c, WM_SLOT(hot_key_wnd), WM_UNTRACKED | WM_KEPT_BY_FORGET, make_shown_lua_window, close_lua_window),
    WM_ROW(0x9f, WM_NO_SLOT, WM_UNTRACKED, make_shown_lua_window, close_lua_window),
    /* No window has this ID any more; post_quit1 still handles it with the Lua windows. */
    WM_ROW(0xab, WM_NO_SLOT, 0, NULL, close_lua_window),
    WM_ROW(0xb5, WM_NO_SLOT, 0, make_shown_lua_window, close_lua_window),
    WM_ROW(0x115, WM_SLOT(new_select_char_wnd), WM_UNTRACKED | WM_GET_SLOT | WM_FORGOTTEN_BY_QUIT, make_select_char, WM_CLOSE_WINDOW),
    WM_ROW(0x116, WM_NO_SLOT, WM_UNTRACKED, make_make_char, WM_CLOSE_WINDOW),
    WM_ROW(0x145, WM_SLOT(rpimage_wnd2), WM_UNTRACKED | WM_GET_SLOT, make_rpimage_wnd2, WM_CLOSE_WINDOW),
    WM_ROW(0x14c, WM_NO_SLOT, WM_UNTRACKED, make_disconnected_server_msg_wnd, WM_CLOSE_WINDOW),
};

const struct WmWindowRows wm_login_windows = WM_WINDOW_ROWS(k_login_rows);
