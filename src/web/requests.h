#ifndef ASYNCWORK_REQUESTS_H
#define ASYNCWORK_REQUESTS_H

#include "core/text_view.h"
#include "web/work_manager.h"
struct AdvenPartyInfo;

int async_work_mgr_account_data_load(struct AsyncWorkMgr *self, int aid, struct TextView world,
    struct TextView url);
int async_work_mgr_account_data_save(struct AsyncWorkMgr *self, int aid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data);
int async_work_mgr_char_data_load(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token);
int async_work_mgr_char_data_save(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, struct TextView data);
int async_work_mgr_emblem_download(struct AsyncWorkMgr *self, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, int version);
int async_work_mgr_emblem_upload(struct AsyncWorkMgr *self, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, struct TextView path);
int async_work_mgr_my_party_load(struct AsyncWorkMgr *self, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token);
int async_work_mgr_party_info_load(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, int query_aid);
int async_work_mgr_party_list_load(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, int mode,
    const struct AdvenPartyInfo *info, int page);
int async_work_mgr_mcstore_load(struct AsyncWorkMgr *self, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, int type);
int async_work_mgr_mcstore_save(struct AsyncWorkMgr *self, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data, int type);
int async_work_mgr_party_register(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token,
    const struct AdvenPartyInfo *info);
int async_work_mgr_party_stop_recruiting(struct AsyncWorkMgr *self, int aid, struct TextView world,
    struct TextView url, struct TextView token, int master_aid);
int async_work_mgr_twitter_send(struct AsyncWorkMgr *self, int aid, struct TextView token,
    struct TextView url, struct TextView tweet, struct TextView image);

#endif
