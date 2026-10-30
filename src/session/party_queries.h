#ifndef SESSION_PARTY_QUERIES_H
#define SESSION_PARTY_QUERIES_H

struct Session;

int session_is_party_member_aid(struct Session *self, int aid, unsigned char only_if_clear);

#endif
