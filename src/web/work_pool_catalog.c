#include "contents/mcstore_work.h"
#include "web/work_pool_internal.h"
#include "web/work_names.h"
#include "web/account_data.h"
#include "web/char_user_data.h"
#include "web/emblem.h"
#include "web/party_works.h"
#include "contents/mcstore_backup.h"
#include "contents/twitter.h"

OFFSET_ASSERT(AccountDataLoadWork, base, 0);
static void account_data_load_initialize(void *work) { account_data_load_work_construct(work); }
static void account_data_load_dispose(void *work) { account_data_load_work_destruct(work); }
static char account_data_load_run(void *work) { return account_data_load_work_run(work); }
static void account_data_load_done(void *work) { account_data_load_work_done(work); }

OFFSET_ASSERT(AccountDataSaveWork, base, 0);
static void account_data_save_initialize(void *work) { account_data_save_work_construct(work); }
static void account_data_save_dispose(void *work) { account_data_save_work_destruct(work); }
static char account_data_save_run(void *work) { return account_data_save_work_run(work); }
static void account_data_save_done(void *work) { account_data_save_work_done(work); }

OFFSET_ASSERT(CharDataLoadWork, base, 0);
static void char_data_load_initialize(void *work) { char_data_load_work_construct(work); }
static void char_data_load_dispose(void *work) { char_data_load_work_destruct(work); }
static char char_data_load_run(void *work) { return char_data_load_work_run(work); }
static void char_data_load_done(void *work) { char_data_load_work_done(work); }

OFFSET_ASSERT(CharDataSaveWork, base, 0);
static void char_data_save_initialize(void *work) { char_data_save_work_construct(work); }
static void char_data_save_dispose(void *work) { char_data_save_work_destruct(work); }
static char char_data_save_run(void *work) { return char_data_save_work_run(work); }
static void char_data_save_done(void *work) { char_data_save_work_done(); }

OFFSET_ASSERT(TwitterSendWork, base, 0);
static void twitter_send_initialize(void *work) { twitter_send_work_construct(work); }
static void twitter_send_dispose(void *work) { twitter_send_work_destruct(work); }
static char twitter_send_run(void *work) { return twitter_send_work_run(work); }
static void twitter_send_done(void *work) { twitter_send_work_done(work); }

OFFSET_ASSERT(EmblemUploadWork, base, 0);
static void emblem_upload_initialize(void *work) { emblem_upload_work_construct(work); }
static void emblem_upload_dispose(void *work) { emblem_upload_work_destruct(work); }
static char emblem_upload_run(void *work) { return emblem_upload_work_run(work); }
static void emblem_upload_done(void *work) { emblem_upload_work_done(work); }

OFFSET_ASSERT(EmblemDownloadWork, base, 0);
static void emblem_download_initialize(void *work) { emblem_download_work_construct(work); }
static void emblem_download_dispose(void *work) { emblem_download_work_destruct(work); }
static char emblem_download_run(void *work) { return emblem_download_work_run(work); }
static void emblem_download_done(void *work) { emblem_download_work_done(work); }

OFFSET_ASSERT(McStoreLoadWork, base, 0);
static void mcstore_load_initialize(void *work) { mcstore_load_work_construct(work); }
static void mcstore_load_dispose(void *work) { mcstore_load_work_destruct(work); }
static char mcstore_load_run(void *work) { return mcstore_load_work_run(work); }
static void mcstore_load_done(void *work) { mcstore_load_work_done(work); }

OFFSET_ASSERT(McStoreSaveWork, base, 0);
static void mcstore_save_initialize(void *work) { mcstore_save_work_construct(work); }
static void mcstore_save_dispose(void *work) { mcstore_save_work_destruct(work); }
static char mcstore_save_run(void *work) { return mcstore_save_work_run(work); }
static void mcstore_save_done(void *work) { mcstore_save_work_done(work); }

OFFSET_ASSERT(PartyListLoadWork, base, 0);
static void party_list_load_initialize(void *work) { party_list_load_work_construct(work); }
static void party_list_load_dispose(void *work) { party_list_load_work_destruct(work); }
static char party_list_load_run(void *work) { return party_list_load_work_run(work); }
static void party_list_load_done(void *work) { party_list_load_work_done(work); }

OFFSET_ASSERT(MyPartyLoadWork, base, 0);
static void my_party_load_initialize(void *work) { my_party_load_work_construct(work); }
static void my_party_load_dispose(void *work) { my_party_load_work_destruct(work); }
static char my_party_load_run(void *work) { return my_party_load_work_run(work); }
static void my_party_load_done(void *work) { my_party_load_work_done(work); }

OFFSET_ASSERT(PartyRegisterWork, base, 0);
static void party_register_initialize(void *work) { party_register_work_construct(work); }
static void party_register_dispose(void *work) { party_register_work_destruct(work); }
static char party_register_run(void *work) { return party_register_work_run(work); }
static void party_register_done(void *work) { party_register_work_done(work); }

OFFSET_ASSERT(PartyStopRecruitingWork, base, 0);
static void party_stop_recruiting_initialize(void *work) { party_stop_recruiting_work_construct(work); }
static void party_stop_recruiting_dispose(void *work) { party_stop_recruiting_work_destruct(work); }
static char party_stop_recruiting_run(void *work) { return party_stop_recruiting_work_run(work); }
static void party_stop_recruiting_done(void *work) { party_stop_recruiting_work_done(work); }

OFFSET_ASSERT(PartyChangeMasterWork, base, 0);
static void party_change_master_initialize(void *work) { party_change_master_work_construct(work); }
static void party_change_master_dispose(void *work) { party_change_master_work_destruct(work); }
static char party_change_master_run(void *work) { return party_change_master_work_run(work); }
static void party_change_master_done(void *work) { party_change_master_work_done(work); }

OFFSET_ASSERT(PartyInfoLoadWork, base, 0);
static void party_info_load_initialize(void *work) { party_info_load_work_construct(work); }
static void party_info_load_dispose(void *work) { party_info_load_work_destruct(work); }
static char party_info_load_run(void *work) { return party_info_load_work_run(work); }
static void party_info_load_done(void *work) { party_info_load_work_done(work); }

struct WorkPoolMgr *work_pool_mgr_create(void)
{
    const struct WorkPoolType types[] = {
        {g_accountDataLoadWorkName, sizeof(struct AccountDataLoadWork), account_data_load_initialize, account_data_load_dispose, {account_data_load_run, account_data_load_done}},
        {g_accountDataSaveWorkName, sizeof(struct AccountDataSaveWork), account_data_save_initialize, account_data_save_dispose, {account_data_save_run, account_data_save_done}},
        {g_charDataLoadWorkName, sizeof(struct CharDataLoadWork), char_data_load_initialize, char_data_load_dispose, {char_data_load_run, char_data_load_done}},
        {g_charDataSaveWorkName, sizeof(struct CharDataSaveWork), char_data_save_initialize, char_data_save_dispose, {char_data_save_run, char_data_save_done}},
        {g_twitterSendWorkName, sizeof(struct TwitterSendWork), twitter_send_initialize, twitter_send_dispose, {twitter_send_run, twitter_send_done}},
        {g_emblemUploadWorkName, sizeof(struct EmblemUploadWork), emblem_upload_initialize, emblem_upload_dispose, {emblem_upload_run, emblem_upload_done}},
        {g_emblemDownloadWorkName, sizeof(struct EmblemDownloadWork), emblem_download_initialize, emblem_download_dispose, {emblem_download_run, emblem_download_done}},
        {g_mcStoreLoadWorkName, sizeof(struct McStoreLoadWork), mcstore_load_initialize, mcstore_load_dispose, {mcstore_load_run, mcstore_load_done}},
        {g_mcStoreSaveWorkName, sizeof(struct McStoreSaveWork), mcstore_save_initialize, mcstore_save_dispose, {mcstore_save_run, mcstore_save_done}},
        {g_partyListLoadWorkName, sizeof(struct PartyListLoadWork), party_list_load_initialize, party_list_load_dispose, {party_list_load_run, party_list_load_done}},
        {g_myPartyLoadWorkName, sizeof(struct MyPartyLoadWork), my_party_load_initialize, my_party_load_dispose, {my_party_load_run, my_party_load_done}},
        {g_partyRegisterWorkName, sizeof(struct PartyRegisterWork), party_register_initialize, party_register_dispose, {party_register_run, party_register_done}},
        {g_partyStopRecruitingWorkName, sizeof(struct PartyStopRecruitingWork), party_stop_recruiting_initialize, party_stop_recruiting_dispose, {party_stop_recruiting_run, party_stop_recruiting_done}},
        {g_partyChangeMasterWorkName, sizeof(struct PartyChangeMasterWork), party_change_master_initialize, party_change_master_dispose, {party_change_master_run, party_change_master_done}},
        {g_partyInfoLoadWorkName, sizeof(struct PartyInfoLoadWork), party_info_load_initialize, party_info_load_dispose, {party_info_load_run, party_info_load_done}},
    };
    return work_pool_create_with_types(types, sizeof types / sizeof types[0]);
}
