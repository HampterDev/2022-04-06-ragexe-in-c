#ifndef UI_CONTROLS_UI_HOOKS_H
#define UI_CONTROLS_UI_HOOKS_H

#include "base.h"

struct ItemInfo;

/* The calls the generic widgets in ui/controls make into feature code above them. There
   is one instance, g_uiHooks (ui/controls/ui_hooks.c); client_install_hooks() in
   core/client_init.c fills it in once at startup, before any window exists. Each member
   says who provides it and what the widgets do while it is NULL. */
struct UIHooks {
    /* Shows the help balloon with `text` near screen position (x, y), as
       mode_show_tooltip does. mode_show_current_tooltip (world/mode_ui_hooks.c), for
       whichever mode runs. NULL: no balloon. */
    void (*show_tooltip)(const char *text, int x, int y, DWORD colour, BYTE flags);
    /* Switches the mouse cursor to animation `action` (CURSOR_ARROW, CURSOR_HAND, ...).
       mode_set_current_cursor_action (world/mode_ui_hooks.c). NULL: the cursor stays. */
    void (*set_cursor_action)(int action);
    /* A click on an item link in a text button, an edit line or a list: shows `item`
       in the item collection window and, unless `no_compare`, the worn item it would
       replace beside it. item_collection_show_link (ui/items/item_collection.c).
       NULL: the click opens nothing. */
    void (*show_item_link)(struct ItemInfo *item, int no_compare);
    /* Navigation links in texts and edit lines: start navigating to `map`, open the
       navigation search for `text`, or show `map` with a mark at (x, y), all through
       the client's navigation manager g_navi. navi_link_navigate, navi_link_search and
       navi_link_show_map (navi/navi_ui_hooks.c). NULL: the link does nothing. */
    void (*navi_navigate)(const char *map, int kind, int flags, unsigned char silent, int x, int y,
                          int id);
    void (*navi_search)(const char *text, size_t size, int filter, unsigned char silent);
    void (*navi_show_map)(const char *map, int x, int y);
    /* A quest link in a text: opens the quest window at quest `id`.
       game_open_quest_link (game/game_ui_hooks.c). NULL: nothing opens. */
    void (*open_quest)(int id);
    /* Whether an edit line may move the IME caret out of a navigation link it shows.
       ui_rodex_compose_allows_caret_move (ui/mail/rodex_ui_hooks.c) says no while the
       mail compose window is open and its body is not the focused edit. NULL: it may. */
    int (*may_move_ime_caret)(void);
};

extern struct UIHooks g_uiHooks;

static __inline void ui_show_tooltip(const char *text, int x, int y, DWORD colour, BYTE flags)
{
    if (g_uiHooks.show_tooltip)
        g_uiHooks.show_tooltip(text, x, y, colour, flags);
}

static __inline void ui_set_cursor_action(int action)
{
    if (g_uiHooks.set_cursor_action)
        g_uiHooks.set_cursor_action(action);
}

#endif
