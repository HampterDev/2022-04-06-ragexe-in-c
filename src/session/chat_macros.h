#ifndef SESSION_CHAT_MACROS_H
#define SESSION_CHAT_MACROS_H

#include "core/text_array.h"
struct Session;
struct JsonTree;

enum { CHAT_MACRO_COUNT = 10 };
extern const char *g_shortcutListSubkey;
void session_init_short_cut_texts(struct Session *session);
struct TextView session_chat_macro(const struct Session *session, int index);
int session_set_chat_macro(struct Session *session, int index, struct TextView text);
int session_chat_macros_changed(const struct Session *session);
int session_read_chat_macros(struct Session *session, const struct JsonTree *list);
void session_write_chat_macros(const struct Session *session, struct JsonTree *data);

#endif
