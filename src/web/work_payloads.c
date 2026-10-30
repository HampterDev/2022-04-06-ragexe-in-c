#include "contents/mcstore_work.h"
#include "web/work_names.h"
#include "web/account_data.h"
#include "web/char_user_data.h"
#include "web/emblem.h"
#include "web/party_works.h"
#include "contents/mcstore_backup.h"
#include "contents/twitter.h"
#include <mmsystem.h>

const char *g_accountDataLoadWorkName = "AccountLinkedUserData_Load";
const char *g_accountDataSaveWorkName = "AccountLinkedUserData_Save";
const char *g_charDataLoadWorkName = "CharacterLinkedUserData_Load";
const char *g_charDataSaveWorkName = "CharacterLinkedUserData_Save";
const char *g_emblemDownloadWorkName = "Emblem_Download";
const char *g_emblemUploadWorkName = "Emblem_Upload";
const char *g_partyChangeMasterWorkName = "Change_PartyMaster";
const char *g_myPartyLoadWorkName = "MyParty_Load";
const char *g_partyInfoLoadWorkName = "PartyInfo_Load";
const char *g_partyListLoadWorkName = "PartyList_Load";
const char *g_partyRegisterWorkName = "Register_Party";
const char *g_partyStopRecruitingWorkName = "Stop_Recruiting";
const char *g_mcStoreLoadWorkName = "MerchantStoreInformation_Load";
const char *g_mcStoreSaveWorkName = "MerchantStoreInformation_Save";
const char *g_twitterSendWorkName = "TwitterData_Send";

struct AccountDataLoadWork *account_data_load_work_construct(struct AccountDataLoadWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    owned_text_init(&self->response);
    self->succeeded = 0;
    return self;
}

void account_data_load_work_destruct(struct AccountDataLoadWork *self)
{
    owned_text_clear(&self->response);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int account_data_load_work_setup(struct AsyncWork *work, int aid, struct TextView world,
    struct TextView url)
{
    struct AccountDataLoadWork *self = (struct AccountDataLoadWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    owned_text_clear(&self->response);
    self->succeeded = 0;
    return 1;

failed:
    account_data_load_work_destruct(self);
    account_data_load_work_construct(self);
    return 0;
}

struct AccountDataSaveWork *account_data_save_work_construct(struct AccountDataSaveWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    owned_text_init(&self->data);
    owned_text_init(&self->token);
    self->succeeded = 0;
    return self;
}

void account_data_save_work_destruct(struct AccountDataSaveWork *self)
{
    owned_text_clear(&self->token);
    owned_text_clear(&self->data);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int account_data_save_work_setup(struct AsyncWork *work, int aid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data)
{
    struct AccountDataSaveWork *self = (struct AccountDataSaveWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    if (!owned_text_assign(&self->data, data)) goto failed;
    self->succeeded = 0;
    return 1;

failed:
    account_data_save_work_destruct(self);
    account_data_save_work_construct(self);
    return 0;
}

struct CharDataLoadWork *char_data_load_work_construct(struct CharDataLoadWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    owned_text_init(&self->token);
    owned_text_init(&self->response);
    self->succeeded = 0;
    return self;
}

void char_data_load_work_destruct(struct CharDataLoadWork *self)
{
    owned_text_clear(&self->response);
    owned_text_clear(&self->token);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int char_data_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token)
{
    struct CharDataLoadWork *self = (struct CharDataLoadWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    owned_text_clear(&self->response);
    self->succeeded = 0;
    return 1;

failed:
    char_data_load_work_destruct(self);
    char_data_load_work_construct(self);
    return 0;
}

struct CharDataSaveWork *char_data_save_work_construct(struct CharDataSaveWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    owned_text_init(&self->data);
    owned_text_init(&self->token);
    self->succeeded = 0;
    return self;
}

void char_data_save_work_destruct(struct CharDataSaveWork *self)
{
    owned_text_clear(&self->token);
    owned_text_clear(&self->data);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int char_data_save_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, struct TextView data)
{
    struct CharDataSaveWork *self = (struct CharDataSaveWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    if (!owned_text_assign(&self->data, data)) goto failed;
    self->succeeded = 0;
    return 1;

failed:
    char_data_save_work_destruct(self);
    char_data_save_work_construct(self);
    return 0;
}

struct EmblemDownloadWork *emblem_download_work_construct(struct EmblemDownloadWork *self)
{
    self->base.started = 0;
    owned_text_init(&self->url);
    self->aid = 0;
    owned_text_init(&self->token);
    owned_text_init(&self->world);
    self->gdid = 0;
    self->version = 0;
    owned_text_init(&self->data);
    self->status = 500;
    self->succeeded = 0;
    return self;
}

void emblem_download_work_destruct(struct EmblemDownloadWork *self)
{
    owned_text_clear(&self->data);
    owned_text_clear(&self->world);
    owned_text_clear(&self->token);
    owned_text_clear(&self->url);
}

int emblem_download_work_setup(struct AsyncWork *work, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, int version)
{
    struct EmblemDownloadWork *self = (struct EmblemDownloadWork *)work;

    self->base.started = timeGetTime();
    if (!owned_text_assign(&self->url, url)) goto failed;
    self->aid = aid;
    if (!owned_text_assign(&self->token, token)) goto failed;
    if (!owned_text_assign(&self->world, world)) goto failed;
    self->gdid = gdid;
    self->version = version;
    return 1;

failed:
    emblem_download_work_destruct(self);
    emblem_download_work_construct(self);
    return 0;
}

struct EmblemUploadWork *emblem_upload_work_construct(struct EmblemUploadWork *self)
{
    self->base.started = 0;
    owned_text_init(&self->url);
    self->aid = 0;
    owned_text_init(&self->token);
    owned_text_init(&self->world);
    self->gdid = 0;
    owned_text_init(&self->path);
    owned_text_init(&self->response);
    self->succeeded = 0;
    return self;
}

void emblem_upload_work_destruct(struct EmblemUploadWork *self)
{
    owned_text_clear(&self->response);
    owned_text_clear(&self->path);
    owned_text_clear(&self->world);
    owned_text_clear(&self->token);
    owned_text_clear(&self->url);
}

int emblem_upload_work_setup(struct AsyncWork *work, struct TextView url, int aid,
    struct TextView token, struct TextView world, int gdid, struct TextView path)
{
    struct EmblemUploadWork *self = (struct EmblemUploadWork *)work;

    self->base.started = timeGetTime();
    if (!owned_text_assign(&self->url, url)) goto failed;
    self->aid = aid;
    if (!owned_text_assign(&self->token, token)) goto failed;
    if (!owned_text_assign(&self->world, world)) goto failed;
    self->gdid = gdid;
    if (!owned_text_assign(&self->path, path)) goto failed;
    return 1;

failed:
    emblem_upload_work_destruct(self);
    emblem_upload_work_construct(self);
    return 0;
}

struct PartyChangeMasterWork *party_change_master_work_construct(struct PartyChangeMasterWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->name);
    owned_text_init(&self->to_name);
    owned_text_init(&self->token);
    owned_text_init(&self->world);
    owned_text_init(&self->response);
    owned_text_init(&self->url);
    self->succeeded = 0;
    return self;
}

void party_change_master_work_destruct(struct PartyChangeMasterWork *self)
{
    owned_text_clear(&self->url);
    owned_text_clear(&self->response);
    owned_text_clear(&self->world);
    owned_text_clear(&self->token);
    owned_text_clear(&self->to_name);
    owned_text_clear(&self->name);
}

struct MyPartyLoadWork *my_party_load_work_construct(struct MyPartyLoadWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->token);
    owned_text_init(&self->world);
    owned_text_init(&self->response);
    owned_text_init(&self->url);
    self->succeeded = 0;
    return self;
}

void my_party_load_work_destruct(struct MyPartyLoadWork *self)
{
    owned_text_clear(&self->url);
    owned_text_clear(&self->response);
    owned_text_clear(&self->world);
    owned_text_clear(&self->token);
}

int my_party_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token)
{
    struct MyPartyLoadWork *self = (struct MyPartyLoadWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    self->succeeded = 0;
    return 1;

failed:
    my_party_load_work_destruct(self);
    my_party_load_work_construct(self);
    return 0;
}

struct PartyInfoLoadWork *party_info_load_work_construct(struct PartyInfoLoadWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    owned_text_init(&self->response);
    owned_text_init(&self->token);
    self->succeeded = 0;
    return self;
}

void party_info_load_work_destruct(struct PartyInfoLoadWork *self)
{
    owned_text_clear(&self->token);
    owned_text_clear(&self->response);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int party_info_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, int query_aid)
{
    struct PartyInfoLoadWork *self = (struct PartyInfoLoadWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    self->query_aid = query_aid;
    self->succeeded = 0;
    return 1;

failed:
    party_info_load_work_destruct(self);
    party_info_load_work_construct(self);
    return 0;
}

struct PartyListLoadWork *party_list_load_work_construct(struct PartyListLoadWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    self->page = 0;
    owned_text_init(&self->response);
    owned_text_init(&self->token);
    party_booking_init(&self->info);
    self->succeeded = 0;
    return self;
}

void party_list_load_work_destruct(struct PartyListLoadWork *self)
{
    party_booking_clear(&self->info);
    owned_text_clear(&self->token);
    owned_text_clear(&self->response);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int party_list_load_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, const struct AdvenPartyInfo *info, int mode,
    int page)
{
    struct PartyListLoadWork *self = (struct PartyListLoadWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    if (!party_booking_assign(&self->info, info)) goto failed;
    self->page = page;
    self->mode = mode;
    self->succeeded = 0;
    return 1;

failed:
    party_list_load_work_destruct(self);
    party_list_load_work_construct(self);
    return 0;
}

struct PartyRegisterWork *party_register_work_construct(struct PartyRegisterWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->token);
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    party_booking_init(&self->info);
    self->succeeded = 0;
    owned_text_init(&self->response);
    return self;
}

void party_register_work_destruct(struct PartyRegisterWork *self)
{
    owned_text_clear(&self->response);
    party_booking_clear(&self->info);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
    owned_text_clear(&self->token);
}

int party_register_work_setup(struct AsyncWork *work, int aid, int gid, struct TextView world,
    struct TextView url, struct TextView token, const struct AdvenPartyInfo *info)
{
    struct PartyRegisterWork *self = (struct PartyRegisterWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    if (!party_booking_assign(&self->info, info)) goto failed;
    self->succeeded = 0;
    return 1;

failed:
    party_register_work_destruct(self);
    party_register_work_construct(self);
    return 0;
}

struct PartyStopRecruitingWork *party_stop_recruiting_work_construct(struct PartyStopRecruitingWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->master_aid = 0;
    owned_text_init(&self->token);
    owned_text_init(&self->world);
    owned_text_init(&self->response);
    owned_text_init(&self->url);
    self->succeeded = 0;
    return self;
}

void party_stop_recruiting_work_destruct(struct PartyStopRecruitingWork *self)
{
    owned_text_clear(&self->url);
    owned_text_clear(&self->response);
    owned_text_clear(&self->world);
    owned_text_clear(&self->token);
}

int party_stop_recruiting_work_setup(struct AsyncWork *work, int aid, struct TextView world,
    struct TextView url, struct TextView token, int master_aid)
{
    struct PartyStopRecruitingWork *self = (struct PartyStopRecruitingWork *)work;

    self->base.started = timeGetTime();
    self->aid = aid;
    self->master_aid = master_aid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    self->succeeded = 0;
    return 1;

failed:
    party_stop_recruiting_work_destruct(self);
    party_stop_recruiting_work_construct(self);
    return 0;
}

struct McStoreLoadWork *mcstore_load_work_construct(struct McStoreLoadWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    owned_text_init(&self->token);
    owned_text_init(&self->response);
    self->succeeded = 0;
    self->type = 0;
    return self;
}

void mcstore_load_work_destruct(struct McStoreLoadWork *self)
{
    owned_text_clear(&self->response);
    owned_text_clear(&self->token);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int mcstore_load_work_init(struct McStoreLoadWork *self, struct TextView url, unsigned aid,
    unsigned gid, struct TextView world, struct TextView token, int type)
{
    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    owned_text_clear(&self->response);
    self->succeeded = 0;
    self->type = type;
    return 1;

failed:
    mcstore_load_work_destruct(self);
    mcstore_load_work_construct(self);
    return 0;
}

struct McStoreSaveWork *mcstore_save_work_construct(struct McStoreSaveWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    self->gid = 0;
    owned_text_init(&self->world);
    owned_text_init(&self->url);
    owned_text_init(&self->data);
    owned_text_init(&self->token);
    self->succeeded = 0;
    owned_text_init(&self->response);
    self->type = 0;
    return self;
}

void mcstore_save_work_destruct(struct McStoreSaveWork *self)
{
    owned_text_clear(&self->response);
    owned_text_clear(&self->token);
    owned_text_clear(&self->data);
    owned_text_clear(&self->url);
    owned_text_clear(&self->world);
}

int mcstore_save_work_init(struct McStoreSaveWork *self, struct TextView url, unsigned aid,
    unsigned gid, struct TextView world, struct TextView token, struct TextView data, int type)
{
    self->base.started = timeGetTime();
    self->aid = aid;
    self->gid = gid;
    if (!owned_text_assign(&self->world, world)) goto failed;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    if (!owned_text_assign(&self->data, data)) goto failed;
    self->succeeded = 0;
    self->type = type;
    return 1;

failed:
    mcstore_save_work_destruct(self);
    mcstore_save_work_construct(self);
    return 0;
}

struct TwitterSendWork *twitter_send_work_construct(struct TwitterSendWork *self)
{
    self->base.started = 0;
    self->aid = 0;
    owned_text_init(&self->url);
    owned_text_init(&self->tweet);
    owned_text_init(&self->image);
    owned_text_init(&self->token);
    self->succeeded = 0;
    return self;
}

void twitter_send_work_destruct(struct TwitterSendWork *self)
{
    owned_text_clear(&self->token);
    owned_text_clear(&self->image);
    owned_text_clear(&self->tweet);
    owned_text_clear(&self->url);
}

int twitter_send_work_init(struct TwitterSendWork *self, unsigned aid, struct TextView token,
    struct TextView url, struct TextView tweet, struct TextView image)
{
    self->base.started = timeGetTime();
    self->aid = aid;
    if (!owned_text_assign(&self->url, url)) goto failed;
    if (!owned_text_assign(&self->tweet, tweet)) goto failed;
    if (!owned_text_assign(&self->image, image)) goto failed;
    if (!owned_text_assign(&self->token, token)) goto failed;
    self->succeeded = 0;
    return 1;

failed:
    twitter_send_work_destruct(self);
    twitter_send_work_construct(self);
    return 0;
}
