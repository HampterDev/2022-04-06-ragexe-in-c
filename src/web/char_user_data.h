#include "core/owned_text.h"
#ifndef ASYNCWORK_CHAR_USER_DATA_H
#define ASYNCWORK_CHAR_USER_DATA_H

#include "web/asyncwork.h"
#include "web/service_managers.h"

struct JsonTree;

struct CharDataLoadWork {
    struct AsyncWork base;
    int aid;
    int gid;
    struct OwnedText world;
    struct OwnedText url;
    struct OwnedText token;
    struct OwnedText response;
    int succeeded;
};

struct CharDataSaveWork {
    struct AsyncWork base;
    int aid;
    int gid;
    struct OwnedText world;
    struct OwnedText url;
    struct OwnedText data;
    struct OwnedText token;
    int succeeded;
};

struct CharDataLoadWork *char_data_load_work_construct(struct CharDataLoadWork *self);
void char_data_load_work_destruct(struct CharDataLoadWork *self);
char char_data_load_work_run(struct CharDataLoadWork *self);
void char_data_load_work_done(struct CharDataLoadWork *self);
struct CharDataSaveWork *char_data_save_work_construct(struct CharDataSaveWork *self);
void char_data_save_work_destruct(struct CharDataSaveWork *self);
char char_data_save_work_run(struct CharDataSaveWork *self);
void char_data_save_work_done(void);

void char_user_data_read_homun_skills(struct JsonTree *data);
void char_user_data_read_skills(struct JsonTree *data);
void char_user_data_write_homun_skills(struct CharUserDataMgr *self, struct JsonTree *data);
void char_user_data_write_skills(struct CharUserDataMgr *self, struct JsonTree *data);

#endif
