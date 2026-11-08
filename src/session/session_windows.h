#ifndef SESSION_SESSION_WINDOWS_H
#define SESSION_SESSION_WINDOWS_H

#include "session/csession.h"

void session_refresh_basic_info(struct Session *self);
void session_invalidate_homun_info_wnd(struct Session *self);
void session_invalidate_mer_info_wnd(struct Session *self);
void session_refresh_skill_tree(struct Session *self);
void session_invalidate_windows(struct Session *self);
void session_notify_messenger_group_wnd(struct Session *self);
void session_refresh_party_ui(struct Session *self);
void session_refresh_friend_ui(struct Session *self);
void session_notify_store_candidate_window(struct Session *self, int which);
void session_notify_store_setup_window(struct Session *self, BYTE alt, int value);
void session_notify_messenger_group_and_plates(struct Session *self);
void session_refresh_mouse_snap_state(struct Session *self, int skill);
void session_refresh_equip_wnd(struct Session *self);
/* Called by session_refresh_skill_tree with the basic info window and its height, to lay
   the window out again. client_install_hooks() (core/client_init.c) installs
   ui_basic_info_wnd_set_height (ui/character/basic_info_wnd.c). NULL: the window keeps its
   layout. */
struct UIBasicInfoWnd;
extern void (*g_sessionBasicInfoHeight)(struct UIBasicInfoWnd *wnd, int height);

void session_refresh_homun_skill_item_wnd(struct Session *self);
void session_refresh_mer_skill_item_wnd(struct Session *self);
void session_refresh_skill_wnds(struct Session *self);
void session_take_back_mail_item(struct Session *self, int index, int used);
void session_set_hide_other_players(struct Session *self, BYTE flag);
void session_set_quick_slot(struct Session *self, BYTE kind, int id, int value, int slot);

#endif
