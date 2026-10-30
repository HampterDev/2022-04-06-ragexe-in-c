#ifndef CONTENTS_MCSTORE_WORK_H
#define CONTENTS_MCSTORE_WORK_H
#include "base.h"
#include "web/work.h"
#include "web/http_post.h"
#include "web/requests.h"
#include "core/owned_text.h"

struct McStoreLoadWork {
    struct AsyncWork base;
    unsigned aid;
    unsigned gid;
    struct OwnedText world;
    struct OwnedText url;
    struct OwnedText token;
    struct OwnedText response;
    int succeeded;
    int type;
};

struct McStoreSaveWork {
    struct AsyncWork base;
    unsigned aid;
    unsigned gid;
    struct OwnedText world;
    struct OwnedText url;
    struct OwnedText data;
    struct OwnedText token;
    int succeeded;
    struct OwnedText response;
    int type;
};

struct McStoreLoadWork *mcstore_load_work_construct(struct McStoreLoadWork *self);
void mcstore_load_work_destruct(struct McStoreLoadWork *self);
int mcstore_load_work_init(struct McStoreLoadWork *self, struct TextView url, unsigned aid,
    unsigned gid, struct TextView world, struct TextView token, int type);
void mcstore_load_work_done(struct McStoreLoadWork *self);
char mcstore_load_work_run(struct McStoreLoadWork *self);

struct McStoreSaveWork *mcstore_save_work_construct(struct McStoreSaveWork *self);
void mcstore_save_work_destruct(struct McStoreSaveWork *self);
int mcstore_save_work_init(struct McStoreSaveWork *self, struct TextView url, unsigned aid,
    unsigned gid, struct TextView world, struct TextView token, struct TextView data, int type);
void mcstore_save_work_done(struct McStoreSaveWork *self);
char mcstore_save_work_run(struct McStoreSaveWork *self);

#endif
