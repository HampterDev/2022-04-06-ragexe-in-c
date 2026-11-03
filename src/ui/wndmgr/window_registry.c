#include "ui/window_types.h"
#include "ui/wndmgr/window_registry.h"

/* The rows that no window cluster owns; window_table.h describes the columns. */
static const struct WmWindowRow k_unowned_rows[] = {
    /* IDs of windows this client no longer has. The original's lists named them, and ESC
       still does not track them; the dead-code sweep left them as found. 0x91 is tracked only
       while a visible window has its ID, which none has. */
    WM_ROW(0x32, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x73, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x7b, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x7c, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x88, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x90, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x91, WM_NO_SLOT, WM_UNTRACKED_UNLESS_SHOWN, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x9a, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0xb4, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0xba, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0xbd, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    WM_ROW(0x157, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    /* Windows without an ID of their own. */
    WM_ROW(UI_ID_NONE, WM_NO_SLOT, WM_UNTRACKED, NULL, WM_CLOSE_WINDOW),
    /* Never set; only clear_slots writes them. */
    WM_ROW_SLOT(orphan_char_wnd, WM_KEPT_BY_FORGET),
    WM_ROW_SLOT(orphan_guild_wnd, WM_KEPT_BY_FORGET),
};

static const struct WmWindowRows unowned_windows = WM_WINDOW_ROWS(k_unowned_rows);

/* The composition point: every table of window rows. Who owns the rows, what their order
   means and what a missing row does is in window_table.h; in short, an ID may have only one
   row in all of these, and the order of the list decides nothing. */
static const struct WmWindowRows *const k_window_tables[] = {
    &wm_login_windows,
    &wm_character_windows,
    &wm_hud_windows,
    &wm_trade_windows,
    &wm_social_windows,
    &unowned_windows,
};

const struct WmWindowTable g_windowTable = { k_window_tables, sizeof k_window_tables / sizeof k_window_tables[0] };
