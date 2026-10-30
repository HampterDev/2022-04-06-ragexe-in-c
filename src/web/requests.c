#include "contents/mcstore_work.h"
#include "web/work_names.h"
#include "web/work_manager_internal.h"
#include "web/asyncwork.h"
#include "contents/mcstore_backup.h"
#include "contents/twitter.h"

int async_work_mgr_account_data_load(struct AsyncWorkMgr *self, int aid, struct TextView world,
    struct TextView url)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_accountDataLoadWorkName);

    if (work) {
        if (account_data_load_work_setup(work, aid, world, url))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_account_data_save(struct AsyncWorkMgr *self, int aid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_accountDataSaveWorkName);

    if (work) {
        if (account_data_save_work_setup(work, aid, world, url, token, data))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_char_data_load(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_charDataLoadWorkName);

    if (work) {
        if (char_data_load_work_setup(work, aid, gid, world, url, token))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_char_data_save(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, struct TextView data)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_charDataSaveWorkName);

    if (work) {
        if (char_data_save_work_setup(work, aid, gid, world, url, token, data))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_emblem_download(struct AsyncWorkMgr *self, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, int version)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_emblemDownloadWorkName);

    if (work) {
        if (emblem_download_work_setup(work, url, aid, token, world, gdid, version))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_emblem_upload(struct AsyncWorkMgr *self, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, struct TextView path)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_emblemUploadWorkName);

    if (work) {
        if (emblem_upload_work_setup(work, url, aid, token, world, gdid, path))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_my_party_load(struct AsyncWorkMgr *self, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_myPartyLoadWorkName);

    if (work) {
        if (my_party_load_work_setup(work, aid, gid, world, url, token))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_party_info_load(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, int query_aid)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_partyInfoLoadWorkName);

    if (work) {
        if (party_info_load_work_setup(work, aid, gid, world, url, token, query_aid))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_party_list_load(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, int mode,
    const struct AdvenPartyInfo *info, int page)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_partyListLoadWorkName);

    if (work) {
        if (party_list_load_work_setup(work, aid, gid, world, url, token, info, mode, page))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_mcstore_load(struct AsyncWorkMgr *self, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, int type)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_mcStoreLoadWorkName);

    if (work) {
        if (mcstore_load_work_init((struct McStoreLoadWork *)work, url, aid, gid, world, token, type))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_mcstore_save(struct AsyncWorkMgr *self, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data, int type)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_mcStoreSaveWorkName);

    if (work) {
        if (mcstore_save_work_init((struct McStoreSaveWork *)work, url, aid, gid, world, token,
                               data, type))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_party_register(struct AsyncWorkMgr *self, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token,
    const struct AdvenPartyInfo *info)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_partyRegisterWorkName);

    if (work) {
        if (party_register_work_setup(work, aid, gid, world, url, token, info))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_party_stop_recruiting(struct AsyncWorkMgr *self, int aid, struct TextView world,
    struct TextView url, struct TextView token, int master_aid)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_partyStopRecruitingWorkName);

    if (work) {
        if (party_stop_recruiting_work_setup(work, aid, world, url, token, master_aid))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}

int async_work_mgr_twitter_send(struct AsyncWorkMgr *self, int aid, struct TextView token,
    struct TextView url, struct TextView tweet, struct TextView image)
{
    int submitted = 0;
    struct AsyncWork *work = async_work_mgr_acquire(self, g_twitterSendWorkName);

    if (work) {
        if (twitter_send_work_init((struct TwitterSendWork *)work, (unsigned)aid, token, url, tweet,
                               image))
            submitted = async_work_mgr_submit(self, work);
        else
            async_work_mgr_cancel(self, work);
    }
    return submitted;
}
