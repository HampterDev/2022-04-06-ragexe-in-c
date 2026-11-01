#ifndef CONTENTS_CHARACTER_SETTINGS_H
#define CONTENTS_CHARACTER_SETTINGS_H

struct CharUserDataMgr;
extern struct CharUserDataMgr *g_charUserDataMgr;

int char_user_data_create(void);
void char_user_data_destroy(void);
void char_user_data_reset(struct CharUserDataMgr *self);
void char_user_data_load(struct CharUserDataMgr *self, int aid, int gid, const char *world);
int char_user_data_save(struct CharUserDataMgr *self);

#endif
