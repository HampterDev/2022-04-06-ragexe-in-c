#include "session/character_name.h"
#include "web/service_manager_state.h"
#include "web/requests.h"
#include "web/agency.h"
#include "contents/mcstore_requests.h"
#include "session/state.h"
#include "session/service_address.h"

static int adventurer_agency_url(struct AdventurerAgencyMgr *self, struct OwnedText *out, int kind)
{
    const char *path;
    if (!self) return 0;
    switch (kind) {
    case 0: case 2: path = "/party/add"; break;
    case 1: path = "/party/list"; break;
    case 3: path = "/party/search"; break;
    case 4: path = "/party/del"; break;
    case 5: path = "/party/get"; break;
    case 6: path = "/party/change"; break;
    case 7: path = "/party/info"; break;
    default: return 0;
    }
    return session_service_url(&g_session1, out, path);
}

void adventurer_agency_load_my_party(struct AdventurerAgencyMgr *self)
{
    struct OwnedText url = {0};

    if (adventurer_agency_url(self, &url, AGENCY_URL_GET)
        && async_work_mgr_my_party_load(g_asyncWorkMgr, g_session1.ownAid, g_session1.ownGid,
            text_view_cstr(string_cstr(&g_session1.worldName)),
            owned_text_view(&url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken)))
        self->pending++;
    owned_text_clear(&url);
}

int adventurer_agency_load_list(struct AdventurerAgencyMgr *self, int page)
{
    struct AdvenPartyInfo none;
    struct OwnedText url = {0};
    int sent;

    party_booking_init(&none);
    sent = adventurer_agency_url(self, &url, AGENCY_URL_LIST)
        && async_work_mgr_party_list_load(g_asyncWorkMgr, g_session1.ownAid, g_session1.ownGid,
            text_view_cstr(string_cstr(&g_session1.worldName)),
            owned_text_view(&url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), 0, &none, page);
    if (sent) self->pending++;
    owned_text_clear(&url);
    party_booking_clear(&none);
    return sent;
}

void adventurer_agency_load_party_info(struct AdventurerAgencyMgr *self, int query_aid)
{
    struct OwnedText url = {0};

    if (adventurer_agency_url(self, &url, AGENCY_URL_INFO)
        && async_work_mgr_party_info_load(g_asyncWorkMgr, g_session1.ownAid, g_session1.ownGid,
            text_view_cstr(string_cstr(&g_session1.worldName)),
            owned_text_view(&url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), query_aid))
        self->pending++;
    owned_text_clear(&url);
}

int adventurer_agency_register(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *info)
{
    struct OwnedText url = {0};
    int sent;

    sent = adventurer_agency_url(self, &url, AGENCY_URL_ADD)
        && async_work_mgr_party_register(g_asyncWorkMgr, g_session1.ownAid, g_session1.ownGid,
            text_view_cstr(string_cstr(&g_session1.worldName)),
            owned_text_view(&url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), info);
    if (sent) self->pending++;
    owned_text_clear(&url);
    return sent;
}

int adventurer_agency_search(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *filter,
                                         int page)
{
    struct OwnedText url = {0};
    int sent;

    sent = adventurer_agency_url(self, &url, AGENCY_URL_SEARCH)
        && async_work_mgr_party_list_load(g_asyncWorkMgr, g_session1.ownAid, g_session1.ownGid,
            text_view_cstr(string_cstr(&g_session1.worldName)),
            owned_text_view(&url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), 1, filter, page);
    if (sent) self->pending++;
    owned_text_clear(&url);
    return sent;
}

void adventurer_agency_stop_recruiting(struct AdventurerAgencyMgr *self, int master_aid)
{
    struct OwnedText url = {0};

    self->stop_master_aid = master_aid;
    if (adventurer_agency_url(self, &url, AGENCY_URL_DEL)
        && async_work_mgr_party_stop_recruiting(g_asyncWorkMgr, g_session1.ownAid,
            text_view_cstr(string_cstr(&g_session1.worldName)),
            owned_text_view(&url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), master_aid))
        self->pending++;
    owned_text_clear(&url);
}

void adventurer_agency_cancel_own_booking(struct AdventurerAgencyMgr *self)
{
    struct OwnedText url = {0};
    int master_aid;

    if (!self->recruiting)
        return;
    self->stop_master_aid = g_session1.ownAid;
    master_aid = g_session1.ownAid;
    if (adventurer_agency_url(self, &url, AGENCY_URL_DEL)
        && async_work_mgr_party_stop_recruiting(g_asyncWorkMgr, g_session1.ownAid,
            text_view_cstr(string_cstr(&g_session1.worldName)),
            owned_text_view(&url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), master_aid))
        self->pending++;
    owned_text_clear(&url);
}

int twitter_send(struct TwitterDataMgr *self, struct TextView tweet, struct TextView image)
{
    return twitter_has_service(self)
        && async_work_mgr_twitter_send(g_asyncWorkMgr, g_session1.ownAid,
            text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), owned_text_view(&self->upload_url),
            tweet, image);
}

int mcstore_request_load(struct McStoreBackup *self, int aid, int gid, struct TextView world, int type)
{
    return self && aid && gid && self->load_url.size
        && async_work_mgr_mcstore_load(g_asyncWorkMgr, aid, gid, world,
            owned_text_view(&self->load_url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), type);
}

void emblem_mgr_download(struct EmblemMgr *self, int gdid, int version)
{
    if (!self || !self->download_url.size) return;
    async_work_mgr_emblem_download(g_asyncWorkMgr, owned_text_view(&self->download_url),
        g_session1.ownAid, text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken),
        string_view(&g_session1.worldName), gdid, version);
}

int emblem_mgr_upload(struct EmblemMgr *self, int gdid, struct TextView path)
{
    int sent = 0;
    if (self && self->upload_url.size && self->upload_blocked != 1)
        sent = async_work_mgr_emblem_upload(g_asyncWorkMgr, owned_text_view(&self->upload_url),
            g_session1.ownAid, text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken),
            string_view(&g_session1.worldName), gdid, path);
    return sent;
}
