#include "ui/items/item_tags.h"
#include "contents/user_hotkeys.h"
#include "session/hotkey_bindings.h"
#include "ui/window_manager_state.h"
#ifndef UI_HOTKEYS_H
#define UI_HOTKEYS_H

#include "ui/character/character.h"
#include "ui/ui_std.h"

#define WND_HOTKEY 0x9c

#define HOTKEY_GROUPS 4

struct HotKeyEntry {
    int index;
    struct HotKey key;
};

struct HotKey *hotkey_move_assign(struct HotKey *self, struct HotKey *other);

struct Session;

BYTE ui_window_mgr_is_reserved_key(struct UIWindowMgr *self, int key);

struct UIItemTagCtrl;

struct UIHotKeyWnd;
BYTE ui_hot_key_wnd_on_key(struct UIHotKeyWnd *self, int state);

struct UIHotKeyWnd *ui_hot_key_wnd_construct(struct UIHotKeyWnd *self);
struct UIWindow *ui_hot_key_wnd_new(void);

struct HotkeySlot;
const struct HotkeySlot *ui_short_cut_wnd_slot(const struct UIWindow *window, int index);

#endif
