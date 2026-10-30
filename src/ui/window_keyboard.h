#ifndef UI_WINDOW_KEYBOARD_H
#define UI_WINDOW_KEYBOARD_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
struct UIWindowMgr;

BYTE ui_window_mgr_on_key_down(struct UIWindowMgr *manager, int vk, int lparam, int first_press);
BYTE ui_window_mgr_is_chat_edit_focused(struct UIWindowMgr *manager);
BYTE ui_window_mgr_excute_msg_in_battle_mode(struct UIWindowMgr *manager, int lparam);
void ui_window_mgr_set_text_at_battle_mode(struct UIWindowMgr *manager);

#endif
