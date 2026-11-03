#ifndef UI_CHAT_FRIENDS_H
#define UI_CHAT_FRIENDS_H

struct UIWindowMgr;

/* Friend helpers the original client kept on UIWindowMgr, under their original names.
   They live in ui/chat/friend_request.c because they concern the friend list, not
   windows; `self` is unused. */

/* Nonzero when `name` is on the player's friend list. */
int ui_window_mgr_is_friend(struct UIWindowMgr *self, const char *name);
/* Asks the server to add `name` as a friend (CZ_ADD_FRIENDS). */
void ui_window_mgr_request_add_friend(struct UIWindowMgr *self, const char *name);

#endif
