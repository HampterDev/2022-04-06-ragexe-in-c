#ifndef UI_WNDMGR_WINDOW_TABLE_H
#define UI_WNDMGR_WINDOW_TABLE_H

#include <stddef.h>

#include "ui/window_manager_state.h"

/* The window table: one row per window ID, saying everything the window manager does with
   that ID. Before it, the IDs were listed four times: the factory switches (make_window*.c),
   the switches of get_window, is_untracked_id and post_quit1 (window_slots.c), the forget
   list (forget_window.c) and clear_slots (teardown.c).

   Who owns the rows: the window clusters, each next to the factory and close functions its
   rows name: wm_login_windows (make_window_login.c), wm_character_windows, wm_hud_windows,
   wm_trade_windows and wm_social_windows (make_window_<cluster>.c). A row goes with the code
   that creates the ID's window or sets its slot. The rows nobody owns (IDs of windows this
   client no longer has, two slots nothing sets, UI_ID_NONE) are in window_registry.c.

   Composition: g_windowTable (window_registry.c) lists those tables. client_install_hooks()
   installs it with ui_window_mgr_set_window_table() before any window is opened. The window
   manager reaches the feature windows only through the installed rows.

   Order: an ID has at most one row; a second row for an ID, an ID outside 0..WM_WINDOW_IDS-1,
   or two rows that give one slot different slot flags stop the client when the table is
   installed (fatal_error). Apart from that the order of the tables and of their rows decides
   nothing: lookups go by ID, and forget_window, clear_slots and post_quit2 only compare and
   clear slots.

   A missing row, or no installed table: make_window creates nothing and returns NULL,
   get_window looks the ID up only in windows_by_id, is_untracked_id says ESC tracks it,
   post_quit1 queues whatever get_window returns, and no slot is forgotten or cleared for it.

   Columns:
   id     the window ID; WM_NO_ID for a slot that no window ID opens (such a row only says
          whether forget_window and clear_slots clear the slot).
   slot   WM_SLOT(field): the UIWindowMgr field that holds the ID's window; WM_NO_SLOT if the
          window is found only through windows_by_id. Unless a flag says otherwise,
          forget_window clears the slot when its window is destroyed and clear_slots clears
          it at teardown. Two rows may share a slot (0x35 and 0xcf: party_setting_wnd).
   flags  WM_UNTRACKED: is_untracked_id says ESC does not track the ID.
          WM_UNTRACKED_UNLESS_SHOWN: the same, unless a visible window with the ID exists
          (0x91).
          WM_ESC_LAST: the ESC list keeps the ID at its end, so ESC closes that window
          last (0xcb, the navigation window).
          WM_GET_SLOT: get_window returns the slot instead of the windows_by_id entry.
          WM_FORGOTTEN_BY_QUIT: post_quit2 clears the slot when it queues the slot's window.
          WM_KEPT_BY_FORGET (quirk): forget_window leaves the slot when its window is
          destroyed.
          WM_KEPT_BY_CLEAR (quirk): clear_slots leaves the slot; the window's destructor
          forgets it, so after close_all_windows it is NULL anyway.
          The quirks are recorded as found; docs/custom-review.md lists them.
   make   the factory. make_window registers what it returns under the ID (wm_register);
          NULL: make_window creates nothing. The WM_ROW_OPEN rows use wm_open_row, which
          opens the slot's window from the row's `open` column (wm_open: created once,
          placed, given its info, shown).
   close  what post_quit1 does with the ID. It returns whether something was closed; only
          then does the ID leave the ESC list.
          WM_CLOSE_WINDOW (NULL): queue whatever get_window returns; the slot is not cleared.
          WM_CLOSE_SLOT: queue the slot's window and clear the slot.
          WM_CLOSE_STORED: the same, after storing the window's info (ui_window_store_info).
          WM_CLOSE_UNCOUNTED (quirk): WM_CLOSE_SLOT, but post_quit1 returns 0, so the ID
          stays in the ESC list.
          A function: the close does more (mode messages, packets, other windows).
   get    a function that get_window calls instead (0x12d: the first party plate).
   open   for WM_ROW_OPEN rows: the constructor (or the one-argument constructor and its
          argument), the size, the placement (WM_AT_*) and position, and the UIWindowMgr
          field sent to the new window with UI_MSG_SET_INFO (0: none).

   close_all_windows and close_all_but_chat need no column: they destroy the windows of the
   window list and then call clear_slots; what close_all_but_chat keeps (the chat window,
   whisper windows and chat tabs) is chat code in teardown.c. */

#define WM_WINDOW_IDS 0x240 /* window IDs 0..0x23f; UI_ID_NONE is 0x239 */
#define WM_NO_ID (-1)
#define WM_FIELD(field) ((unsigned)offsetof(struct UIWindowMgr, field))
#define WM_SLOT(field) WM_FIELD(field)
#define WM_NO_SLOT 0u /* offset 0 is windows_by_id, never a window (window_table.c checks) */

enum {
    WM_UNTRACKED = 1 << 0,
    WM_UNTRACKED_UNLESS_SHOWN = 1 << 1,
    WM_GET_SLOT = 1 << 2,
    WM_FORGOTTEN_BY_QUIT = 1 << 3,
    WM_KEPT_BY_FORGET = 1 << 4,
    WM_KEPT_BY_CLEAR = 1 << 5,
    WM_ESC_LAST = 1 << 6,
};
/* The flags about the slot; rows that share a slot must agree on them. */
#define WM_SLOT_FLAGS (WM_FORGOTTEN_BY_QUIT | WM_KEPT_BY_FORGET | WM_KEPT_BY_CLEAR)

typedef struct UIWindow *(*WmNew)(void);
typedef struct UIWindow *(*WmNew1)(int arg);
typedef struct UIWindow *(*WmMake)(struct UIWindowMgr *self, int window_id);
typedef int (*WmClose)(struct UIWindowMgr *self, int window_id);
typedef struct UIWindow *(*WmGet)(struct UIWindowMgr *self, int window_id);

struct WmOpen {
    WmNew new_window;
    WmNew1 new_window1;
    int arg;
    int width, height;
    int at, x, y;
    unsigned info;
};

struct WmWindowRow {
    int id;
    unsigned slot;
    unsigned flags;
    WmMake make;
    WmClose close;
    WmGet get;
    struct WmOpen open;
};

/* One owner's rows. */
struct WmWindowRows {
    const struct WmWindowRow *rows;
    size_t count;
};
#define WM_WINDOW_ROWS(rows) { (rows), sizeof(rows) / sizeof((rows)[0]) }

/* The tables the window manager uses, installed at startup. */
struct WmWindowTable {
    const struct WmWindowRows *const *parts;
    size_t count;
};

/* A row whose factory is a function. */
#define WM_ROW(id, slot, flags, make, close) { (id), (slot), (flags), (make), (close), NULL }
/* Rows whose factory is wm_open: constructor, size, placement and position, and info. */
#define WM_ROW_OPEN(id, field, flags, close, new_window, w, h, at, x, y) \
    { (id), WM_SLOT(field), (flags), wm_open_row, (close), NULL, \
      { (new_window), NULL, 0, (w), (h), (at), (x), (y), 0 } }
#define WM_ROW_OPEN_INFO(id, field, flags, close, new_window, w, h, at, x, y, info) \
    { (id), WM_SLOT(field), (flags), wm_open_row, (close), NULL, \
      { (new_window), NULL, 0, (w), (h), (at), (x), (y), WM_FIELD(info) } }
#define WM_ROW_OPEN_INFO1(id, field, flags, close, new_window1, arg, w, h, at, x, y, info) \
    { (id), WM_SLOT(field), (flags), wm_open_row, (close), NULL, \
      { NULL, (new_window1), (arg), (w), (h), (at), (x), (y), WM_FIELD(info) } }
/* A row whose get_window answer is a function. */
#define WM_ROW_GET(id, get) { (id), WM_NO_SLOT, 0, NULL, NULL, (get) }
/* A slot that no window ID opens. */
#define WM_ROW_SLOT(field, flags) { WM_NO_ID, WM_SLOT(field), (flags) }

#define WM_CLOSE_WINDOW NULL
#define WM_CLOSE_SLOT wm_close_row_slot
#define WM_CLOSE_STORED wm_close_row_stored
#define WM_CLOSE_UNCOUNTED wm_close_row_uncounted

/* window_table.c */
void ui_window_mgr_set_window_table(const struct WmWindowTable *table);
/* The installed row of `window_id`, or NULL. */
const struct WmWindowRow *wm_window_row(int window_id);
/* All installed rows, ID rows and slot rows, in table order. */
size_t wm_window_row_count(void);
const struct WmWindowRow *wm_window_row_at(size_t index);
struct UIWindow **wm_row_slot(struct UIWindowMgr *self, const struct WmWindowRow *row);

/* window_slots.c: the plain closes, and the slot helpers custom closes use. */
int wm_close_row_slot(struct UIWindowMgr *self, int window_id);
int wm_close_row_stored(struct UIWindowMgr *self, int window_id);
int wm_close_row_uncounted(struct UIWindowMgr *self, int window_id);
int wm_close_slot(struct UIWindowMgr *self, struct UIWindow **slot);
int wm_close_slot_stored(struct UIWindowMgr *self, struct UIWindow **slot);

/* make_window.c */
struct UIWindow *wm_open_row(struct UIWindowMgr *self, int window_id);

#endif
