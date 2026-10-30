#include "session/character_name.h"
#include "web/service_manager_state.h"
#include "web/requests.h"
#include "contents/mcstore_requests.h"
#include "session/state.h"
#include "session/service_address.h"
#include "session/session_names.h"
#include "world_state_memory.h"
#include <stdio.h>

struct Session g_session1;
struct AsyncWorkMgr *g_asyncWorkMgr;
int service_accepts, service_configured = 1, service_calls, service_chat_lines;
static int expected_mode, expected_page;
static const struct AdvenPartyInfo *expected_booking;

int session_service_url(struct Session *session, struct OwnedText *out, const char *path)
{
    assert(session == &g_session1);
    return service_configured && owned_text_concat(out, text_view_cstr("https://service"), text_view_cstr(path));
}

int async_work_mgr_party_register(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, const struct AdvenPartyInfo *info)
{
    (void)manager;
    assert(aid == 101 && gid == 202 && world.size == 5 && token.size == 16);
    assert(!strcmp(url.data, "https://service/party/add") && info == expected_booking);
    ++service_calls;
    return service_accepts;
}

int async_work_mgr_party_list_load(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, int mode,
    const struct AdvenPartyInfo *info, int page)
{
    (void)manager;
    assert(aid == 101 && gid == 202 && world.size == 5 && token.size == 16);
    assert(mode == expected_mode && page == expected_page);
    assert(!strcmp(url.data, mode ? "https://service/party/search" : "https://service/party/list"));
    if (mode && expected_booking) assert(info == expected_booking);
    ++service_calls;
    return service_accepts;
}

int async_work_mgr_my_party_load(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token)
{
    (void)manager; (void)aid; (void)gid; (void)world; (void)url; (void)token;
    assert(0); return 0;
}

int async_work_mgr_mcstore_load(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, int type)
{
    (void)manager;
    assert(aid == 101 && gid == 202 && world.size == 5 && token.size == 16 && type == 1);
    assert(!strcmp(url.data, "load"));
    ++service_calls;
    return service_accepts;
}

int async_work_mgr_twitter_send(struct AsyncWorkMgr *manager, int aid, struct TextView token,
    struct TextView url, struct TextView tweet, struct TextView image)
{
    (void)manager;
    assert(aid == 101 && token.size == sizeof g_session1.webAuthToken && !memcmp(token.data, g_session1.webAuthToken, token.size) && !strcmp(url.data, "upload"));
    assert(tweet.size == 4 && !memcmp(tweet.data, "text", 4));
    assert(image.size == 9 && !memcmp(image.data, "image.jpg", 9));
    ++service_calls;
    return service_accepts;
}

int session_service_urls(struct Session *session, struct OwnedText *first, const char *a,
    struct OwnedText *second, const char *b)
{
    (void)session; (void)first; (void)second; (void)a; (void)b;
    assert(0); return 0;
}

int async_work_mgr_party_change_master(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView name, int to_aid, int to_gid, struct TextView to_name, struct TextView world,
    struct TextView url, struct TextView token)
{
    (void)manager; (void)aid; (void)gid; (void)name; (void)to_aid; (void)to_gid;
    (void)to_name; (void)world; (void)url; (void)token; assert(0); return 0;
}
int async_work_mgr_party_info_load(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, int query_aid)
{
    (void)manager; (void)aid; (void)gid; (void)world; (void)url; (void)token;
    (void)query_aid; assert(0); return 0;
}
int async_work_mgr_party_stop_recruiting(struct AsyncWorkMgr *manager, int aid, struct TextView world,
    struct TextView url, struct TextView token, int master_aid)
{
    (void)manager; (void)aid; (void)world; (void)url; (void)token; (void)master_aid;
    assert(0); return 0;
}
int async_work_mgr_emblem_download(struct AsyncWorkMgr *manager, struct TextView url, int aid,
    struct TextView token, struct TextView world, int guild, int version)
{
    (void)manager; (void)url; (void)aid; (void)token; (void)world; (void)guild; (void)version;
    assert(0); return 0;
}
int async_work_mgr_emblem_upload(struct AsyncWorkMgr *manager, struct TextView url, int aid,
    struct TextView token, struct TextView world, int guild, struct TextView path)
{
    (void)manager; (void)url; (void)aid; (void)token; (void)world; (void)guild; (void)path;
    ++service_calls; return service_accepts;
}

int main(void)
{
    struct AdventurerAgencyMgr agency = {0};
    struct TwitterDataMgr twitter = {0};
    struct EmblemMgr emblem = {0};
    struct McStoreBackup shop = {0};
    struct AdvenPartyInfo booking;
    int calls, accepted;
    g_adventurerAgency = &agency;
    g_twitterDataMgr = &twitter;
    g_session1.ownAid = 101; g_session1.ownGid = 202;
    memcpy(g_session1.webAuthToken, "0123456789abcdef", sizeof g_session1.webAuthToken);
    g_session1.saleListSentAt = 0x58585858u;
    string_init(&g_session1.worldName);
    string_assign_cstr(&g_session1.worldName, "world");
    owned_text_init(&shop.load_url);
    assert(owned_text_assign(&shop.load_url, text_view("load", 4)));
    assert(owned_text_assign(&twitter.upload_url, text_view_cstr("upload")));
    assert(owned_text_assign(&emblem.upload_url, text_view_cstr("emblem")));
    party_booking_init(&booking);
    expected_booking = &booking;
    for (accepted = 0; accepted < 2; ++accepted) {
        service_accepts = accepted;
        agency.pending = 0;
        expected_mode = 1; expected_page = 3;
        assert(adventurer_agency_register(&agency, &booking) == accepted);
        assert(adventurer_agency_search(&agency, &booking, 3) == accepted);
        expected_mode = 0; expected_page = 4;
        assert(adventurer_agency_load_list(&agency, 4) == accepted);
        assert(agency.pending == 3 * accepted);
        assert(emblem_mgr_upload(&emblem, 7, text_view_cstr("image.bmp")) == accepted);
        assert(mcstore_request_load(&shop, 101, 202, text_view_cstr("world"), 1) == accepted);
        assert(twitter_send(&twitter, text_view_cstr("text"), text_view_cstr("image.jpg")) == accepted);
    }
    calls = service_calls;
    service_configured = 0;
    assert(!adventurer_agency_register(&agency, &booking));
    assert(!adventurer_agency_search(&agency, &booking, 3));
    assert(!adventurer_agency_load_list(&agency, 4));
    assert(!twitter_send(NULL, text_view_cstr("text"), text_view_cstr("image.jpg")));
    assert(!mcstore_request_load(&shop, 0, 202, text_view_cstr("world"), 1));
    assert(!mcstore_request_load(&shop, 101, 0, text_view_cstr("world"), 1));
    emblem.upload_blocked = 1;
    assert(!emblem_mgr_upload(&emblem, 7, text_view_cstr("image.bmp")));
    assert(service_calls == calls && agency.pending == 3);
    service_configured = 1;
    party_booking_clear(&booking);
    owned_text_clear(&emblem.upload_url);
    owned_text_clear(&twitter.upload_url);
    owned_text_clear(&shop.load_url);
    string_free(&g_session1.worldName);
    g_adventurerAgency = NULL; g_twitterDataMgr = NULL;
    assert(!live_blocks);
    puts("Service submissions: real producers return queue status; missing configuration and invalid IDs do not submit; failed submissions preserve counters and borrowed input");
    return 0;
}
