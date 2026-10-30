#ifndef ASYNCWORK_ASYNCWORK_H
#define ASYNCWORK_ASYNCWORK_H

#include <windows.h>
#include "base.h"
#include "std/std.h"
#include "web/work.h"
#include "web/http_post.h"
#include "web/work_manager.h"
#include "web/work_pool.h"
#include "web/party_booking_data.h"

#include "web/requests.h"

int account_data_load_work_setup(struct AsyncWork *work, int aid, struct TextView world,
    struct TextView url);
int account_data_save_work_setup(struct AsyncWork *work, int aid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data);
int char_data_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token);
int char_data_save_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data);
int emblem_download_work_setup(struct AsyncWork *work, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, int version);
int emblem_upload_work_setup(struct AsyncWork *work, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, struct TextView path);
int my_party_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token);
int party_info_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, int query_aid);
int party_list_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, const struct AdvenPartyInfo *info, int mode,
    int page);
int party_register_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, const struct AdvenPartyInfo *info);
int party_stop_recruiting_work_setup(struct AsyncWork *work, int aid, struct TextView world,
    struct TextView url, struct TextView token, int master_aid);

#endif
