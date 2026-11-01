#ifndef CONTENTS_ACCOUNT_SETTINGS_H
#define CONTENTS_ACCOUNT_SETTINGS_H

struct AccountUserDataMgr;
extern struct AccountUserDataMgr *g_accountUserDataMgr;
int account_user_data_create(void);
void account_user_data_destroy(void);
int account_user_data_is_loaded(const struct AccountUserDataMgr *self);
void account_user_data_reset(struct AccountUserDataMgr *self);
void account_user_data_load(struct AccountUserDataMgr *self, int aid, const char *world);
int account_user_data_save(struct AccountUserDataMgr *self);

#endif
