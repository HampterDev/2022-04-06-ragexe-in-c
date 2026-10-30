#include "web/service_manager_state.h"

#include "session/state.h"
#include "session/service_address.h"
#include <stdlib.h>

struct AccountUserDataMgr *g_accountUserDataMgr;

int account_user_data_create(void)
{
    struct AccountUserDataMgr *manager;
    if (g_accountUserDataMgr) return 0;
    manager = calloc(1, sizeof *manager);
    if (!manager) return 0;
    if (!owned_text_assign(&manager->world, text_view_cstr("unknown"))) {
        free(manager);
        return 0;
    }
    g_accountUserDataMgr = manager;
    return 1;
}

void account_user_data_destroy(void)
{
    struct AccountUserDataMgr *manager = g_accountUserDataMgr;
    g_accountUserDataMgr = NULL;
    if (!manager) return;
    owned_text_clear(&manager->load_url);
    owned_text_clear(&manager->save_url);
    owned_text_clear(&manager->world);
    free(manager);
}

int account_user_data_is_loaded(const struct AccountUserDataMgr *self)
{
    return self && self->loaded;
}

int account_user_data_pending_saves(const struct AccountUserDataMgr *self)
{
    return self ? self->pending_saves : 0;
}

void account_user_data_save_answered(struct AccountUserDataMgr *self)
{
    self->pending_saves--;
}

struct CharUserDataMgr *g_charUserDataMgr;

int char_user_data_create(void)
{
    struct CharUserDataMgr *manager;
    if (g_charUserDataMgr) return 0;
    manager = calloc(1, sizeof *manager);
    if (!manager) return 0;
    if (!owned_text_assign(&manager->world, text_view_cstr("unknown"))) {
        free(manager);
        return 0;
    }
    g_charUserDataMgr = manager;
    return 1;
}

void char_user_data_destroy(void)
{
    struct CharUserDataMgr *manager = g_charUserDataMgr;
    g_charUserDataMgr = NULL;
    if (!manager) return;
    owned_text_clear(&manager->load_url);
    owned_text_clear(&manager->save_url);
    owned_text_clear(&manager->world);
    free(manager);
}

void char_user_data_save_answered(struct CharUserDataMgr *self)
{
    self->pending_saves--;
}

struct AdventurerAgencyMgr *g_adventurerAgency;

int adventurer_agency_create(void)
{
    struct AdventurerAgencyMgr *manager;
    if (g_adventurerAgency) return 0;
    manager = calloc(1, sizeof *manager);
    if (!manager) return 0;
    manager->recruiting = 0;
    manager->stop_master_aid = 0;
    manager->parties.items = NULL;
    manager->parties.count = 0;
    manager->pending = 0;
    g_adventurerAgency = manager;
    return 1;
}

void adventurer_agency_destroy(void)
{
    struct AdventurerAgencyMgr *manager = g_adventurerAgency;
    g_adventurerAgency = NULL;
    if (!manager) return;
    party_booking_list_clear(&manager->parties);
    free(manager);
}

int adventurer_agency_is_recruiting(const struct AdventurerAgencyMgr *self)
{
    return self && self->recruiting;
}

int adventurer_agency_pending_requests(const struct AdventurerAgencyMgr *self)
{
    return self ? self->pending : 0;
}

const struct PartyBookingList *adventurer_agency_parties(const struct AdventurerAgencyMgr *self)
{
    return &self->parties;
}

struct EmblemMgr *g_emblemMgr;

int emblem_mgr_create(void)
{
    if (g_emblemMgr) return 0;
    g_emblemMgr = calloc(1, sizeof *g_emblemMgr);
    return g_emblemMgr != NULL;
}

void emblem_mgr_destroy(void)
{
    struct EmblemMgr *manager = g_emblemMgr;
    g_emblemMgr = NULL;
    if (!manager) return;
    owned_text_clear(&manager->upload_url);
    owned_text_clear(&manager->download_url);
    free(manager);
}

int emblem_mgr_build_urls(struct EmblemMgr *self)
{
    return self && session_service_urls(&g_session1,
        &self->upload_url, "/emblem/upload", &self->download_url, "/emblem/download");
}

struct TwitterDataMgr *g_twitterDataMgr;

int twitter_create(void)
{
    struct TwitterDataMgr *manager;
    if (g_twitterDataMgr) return 0;
    manager = calloc(1, sizeof *manager);
    if (!manager) return 0;
    if (!session_service_urls(&g_session1,
            &manager->upload_url, "/twitter/upload", &manager->auth_url, "/twitter/user-auth")) {
        owned_text_clear(&manager->upload_url);
        owned_text_clear(&manager->auth_url);
        free(manager);
        return 0;
    }
    g_twitterDataMgr = manager;
    return 1;
}

void twitter_destroy(void)
{
    struct TwitterDataMgr *manager = g_twitterDataMgr;
    g_twitterDataMgr = NULL;
    if (!manager) return;
    owned_text_clear(&manager->upload_url);
    owned_text_clear(&manager->auth_url);
    free(manager);
}

const char *twitter_auth_url(const struct TwitterDataMgr *self)
{
    return self ? owned_text_cstr(&self->auth_url) : "";
}

int twitter_has_service(const struct TwitterDataMgr *self)
{
    return self && self->upload_url.size != 0;
}
