#include "core/owned_text.h"
#ifndef ASYNCWORK_EMBLEM_H
#define ASYNCWORK_EMBLEM_H

#include "web/asyncwork.h"
#include "web/service_managers.h"

struct EmblemDownloadWork {
    struct AsyncWork base;
    struct OwnedText url;
    int aid;
    struct OwnedText token;
    struct OwnedText world;
    int gdid;
    int version;
    struct OwnedText data;
    int status;
    int succeeded;
};

struct EmblemUploadWork {
    struct AsyncWork base;
    struct OwnedText url;
    int aid;
    struct OwnedText token;
    struct OwnedText world;
    int gdid;
    struct OwnedText path;
    struct OwnedText response;
    int succeeded;
};

struct EmblemDownloadWork *emblem_download_work_construct(struct EmblemDownloadWork *self);
void emblem_download_work_destruct(struct EmblemDownloadWork *self);
void emblem_download_work_done(struct EmblemDownloadWork *self);
char emblem_download_work_run(struct EmblemDownloadWork *self);
struct EmblemUploadWork *emblem_upload_work_construct(struct EmblemUploadWork *self);
void emblem_upload_work_destruct(struct EmblemUploadWork *self);
void emblem_upload_work_done(struct EmblemUploadWork *self);
char emblem_upload_work_run(struct EmblemUploadWork *self);

#endif
