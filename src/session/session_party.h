#ifndef SESSION_SESSION_PARTY_H
#define SESSION_SESSION_PARTY_H

#include "session/party_types.h"

struct Session;

DWORD session_get_party_color_by_seq(struct Session *self, unsigned seq);
void session_add_friend(struct Session *self, struct PartyMember *m);
void session_add_party_member(struct Session *self, struct PartyMember *m);
void session_set_party_member_state(struct Session *self, DWORD aid, DWORD role);
void session_set_party_member_job_level(struct Session *self, DWORD aid, WORD a, WORD b);
void session_set_party_member_role(struct Session *self, DWORD aid, DWORD value);
void session_clear_friends(struct Session *self);
void session_clear_party_members(struct Session *self);
int session_remove_friend(struct Session *self, DWORD value);
void session_remove_party_member(struct Session *self, const char *name);
DWORD session_get_party_member_aid(struct Session *self, const char *name);
int session_count_online_party_members(struct Session *self);
struct PartyMember *session_get_party_member(struct Session *self, struct PartyMember *out,
                                                      DWORD aid);
struct PartyMember *session_get_party_member_by_name(struct Session *self, struct PartyMember *out,
                                                                 const char *name);
void session_hi_friends(struct Session *self, const char *text);
void session_rename_friend(struct Session *self, int id, const char *name);
int session_set_friend_state(struct Session *self, int aid, int char_id, BYTE value);

int session_not_lookalike(struct Session *self, const char *name, const char *other);
int session_looks_like_a_friend(struct Session *self, const char *name);
int session_looks_like_a_party_member(struct Session *self, const char *name);
BYTE np_dect(const char *name);
int session_has_friend_named(struct Session *self, const char *name);
int session_is_guild_member_by_name(struct Session *self, const char *name);
int session_is_guild_member_aid(struct Session *self, int id);
int session_is_guild_member_char_id(struct Session *self, int id);
BYTE session_named_body_palettes_enabled(struct Session *self);
BYTE session_is_account_char_name(struct Session *self, struct String name);
BYTE session_is_known_name(struct Session *self, struct String name);
BYTE session_is_ascii_name(struct Session *self, const char *name);

void session_on_pending_name(struct Session *self, int aid, const char *text);
void session_add_pending_name(struct Session *self, int aid);

#endif
