#ifndef ASYNCWORK_SERVICE_MANAGERS_H
#define ASYNCWORK_SERVICE_MANAGERS_H

#include "core/text_view.h"

struct AccountUserDataMgr;
struct CharUserDataMgr;
struct AdventurerAgencyMgr;
struct AdvenPartyInfo;
struct PartyBookingList;

extern struct AccountUserDataMgr *g_accountUserDataMgr;
extern struct CharUserDataMgr *g_charUserDataMgr;
extern struct AdventurerAgencyMgr *g_adventurerAgency;

int account_user_data_create(void);
void account_user_data_destroy(void);
int account_user_data_is_loaded(const struct AccountUserDataMgr *self);
int account_user_data_pending_saves(const struct AccountUserDataMgr *self);
void account_user_data_reset(struct AccountUserDataMgr *self);
void account_user_data_load(struct AccountUserDataMgr *self, int aid, const char *world);
void account_user_data_save(struct AccountUserDataMgr *self);
void account_user_data_on_loaded(struct AccountUserDataMgr *self, int ok, int aid,
    const char *world, const char *response);
void account_user_data_save_answered(struct AccountUserDataMgr *self);

int char_user_data_create(void);
void char_user_data_destroy(void);
void char_user_data_reset(struct CharUserDataMgr *self);
void char_user_data_load(struct CharUserDataMgr *self, int aid, int gid, const char *world);
void char_user_data_save(struct CharUserDataMgr *self);
void char_user_data_on_loaded(struct CharUserDataMgr *self, int ok, int aid, int gid,
    const char *world, const char *response);
void char_user_data_save_answered(struct CharUserDataMgr *self);

int adventurer_agency_create(void);
void adventurer_agency_destroy(void);
int adventurer_agency_is_recruiting(const struct AdventurerAgencyMgr *self);
int adventurer_agency_pending_requests(const struct AdventurerAgencyMgr *self);
const struct PartyBookingList *adventurer_agency_parties(const struct AdventurerAgencyMgr *self);
void adventurer_agency_load_my_party(struct AdventurerAgencyMgr *self);
int adventurer_agency_load_list(struct AdventurerAgencyMgr *self, int page);
void adventurer_agency_load_party_info(struct AdventurerAgencyMgr *self, int aid);
int adventurer_agency_register(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *info);
int adventurer_agency_search(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *filter, int page);
void adventurer_agency_stop_recruiting(struct AdventurerAgencyMgr *self, int aid);
void adventurer_agency_cancel_own_booking(struct AdventurerAgencyMgr *self);
void adventurer_agency_change_master_answered(struct AdventurerAgencyMgr *self, int ok, const char *response);
void adventurer_agency_my_party_loaded(struct AdventurerAgencyMgr *self, int ok, const char *response);
void adventurer_agency_party_list_loaded(struct AdventurerAgencyMgr *self, int ok, const char *response);
void adventurer_agency_party_info_loaded(struct AdventurerAgencyMgr *self, int ok, const char *response);
void adventurer_agency_register_answered(struct AdventurerAgencyMgr *self, int ok, const char *response);
void adventurer_agency_stop_recruiting_answered(struct AdventurerAgencyMgr *self, int ok, const char *response);

struct EmblemMgr;
extern struct EmblemMgr *g_emblemMgr;
int emblem_mgr_create(void);
void emblem_mgr_destroy(void);
int emblem_mgr_build_urls(struct EmblemMgr *self);
void emblem_mgr_download(struct EmblemMgr *self, int gdid, int version);
int emblem_mgr_upload(struct EmblemMgr *self, int gdid, struct TextView path);
void emblem_mgr_on_downloaded(struct EmblemMgr *self, int succeeded, int status,
    struct TextView data, int gdid, int version);
void emblem_mgr_on_uploaded(struct EmblemMgr *self, int succeeded,
    struct TextView response, int gdid);

struct TwitterDataMgr;
extern struct TwitterDataMgr *g_twitterDataMgr;
int twitter_create(void);
void twitter_destroy(void);
const char *twitter_auth_url(const struct TwitterDataMgr *self);
int twitter_has_service(const struct TwitterDataMgr *self);
int twitter_send(struct TwitterDataMgr *self, struct TextView tweet, struct TextView image);
void twitter_on_sent(struct TwitterDataMgr *self, int succeeded);

#endif
