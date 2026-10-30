#include "core/owned_text.h"
#include "session/state.h"
#ifndef ASYNCWORK_ACCOUNT_DATA_H
#define ASYNCWORK_ACCOUNT_DATA_H

#include "web/asyncwork.h"
#include "web/service_managers.h"

struct JsonTree;

struct AccountDataLoadWork {
    struct AsyncWork base;
    int aid;
    struct OwnedText world;
    struct OwnedText url;
    struct OwnedText response;
    int succeeded;
};

struct AccountDataSaveWork {
    struct AsyncWork base;
    int aid;
    struct OwnedText world;
    struct OwnedText url;
    struct OwnedText data;
    struct OwnedText token;
    int succeeded;
};

struct AccountDataLoadWork *account_data_load_work_construct(struct AccountDataLoadWork *self);
void account_data_load_work_destruct(struct AccountDataLoadWork *self);
char account_data_load_work_run(struct AccountDataLoadWork *self);
void account_data_load_work_done(struct AccountDataLoadWork *self);

struct AccountDataSaveWork *account_data_save_work_construct(struct AccountDataSaveWork *self);
void account_data_save_work_destruct(struct AccountDataSaveWork *self);
char account_data_save_work_run(struct AccountDataSaveWork *self);
void account_data_save_work_done(struct AccountDataSaveWork *self);

int account_user_data_read_block_list(const struct JsonTree *data);
int account_user_data_read_chat_macros(struct AccountUserDataMgr *self, const struct JsonTree *data);
void account_user_data_read_hotkey_tab(struct AccountUserDataMgr *self, const struct JsonTree *tab,
                                                  int group);
void account_user_data_read_hotkeys(struct AccountUserDataMgr *self, struct JsonTree *data);
int account_user_data_write_hotkeys(struct AccountUserDataMgr *self, struct JsonTree *data);

#endif
