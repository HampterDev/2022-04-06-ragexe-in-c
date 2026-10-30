#ifndef SESSION_SKILL_LEVELS_H
#define SESSION_SKILL_LEVELS_H

struct Session;
int session_get_homun_skill_use_level(struct Session *session, int skill_id);
int session_get_merc_skill_use_level(struct Session *session, int skill_id);
int session_get_skill_table_value(struct Session *session, int skill_id);
void session_set_homun_skill_use_level(struct Session *session, int skill_id, int value);
void session_set_merc_skill_use_level(struct Session *session, int skill_id, int value);

#endif
