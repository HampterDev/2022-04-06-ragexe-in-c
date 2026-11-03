#ifndef SESSION_HOTKEY_BINDINGS_H
#define SESSION_HOTKEY_BINDINGS_H

#include "std/string.h"

struct Session;

struct HotKey {
    int code;
    int modifier;
    struct String label;
    struct String action;
};

struct HotKey *session_get_hot_key(struct Session *session, struct HotKey *out,
                                int category, int index);

int session_get_hot_key_list_size(struct Session *session, int group);
/* The slot of the skill hotkey in the first hotkey list bound to key `code`, or -1
   (g_windowMgrHandlers.skill_hot_key_slot). */
int session_skill_hot_key_slot(struct Session *session, int code);
int session_get_original_hot_key_index(struct Session *session, int group, int index);

#endif
