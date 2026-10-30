#ifndef SESSION_OPTION_PAIRS_H
#define SESSION_OPTION_PAIRS_H

struct Session;
int session_get_instant_var(struct Session *session, int key);
void session_set_pair(struct Session *session, int key, long long value);

#endif
