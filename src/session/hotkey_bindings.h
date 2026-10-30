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
int session_get_original_hot_key_index(struct Session *session, int group, int index);

#endif
