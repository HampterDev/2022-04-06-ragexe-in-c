#include <assert.h>
#include <string.h>
#include "service_manager_test_support.h"
#define calloc service_test_calloc
#define free service_test_free
#include "../src/web/service_manager_state.c"
#undef calloc
#undef free
#define malloc service_text_malloc
#define free service_text_free
#include "../src/core/owned_text.c"
#undef malloc
#undef free

void service_test_account_state(void)
{
    struct AccountUserDataMgr *manager = g_accountUserDataMgr;
    char world[6001];
    assert(manager && !manager->aid && !manager->loaded && !manager->pending_saves);
    assert(!strcmp(owned_text_cstr(&manager->world), "unknown"));
    assert(!manager->save_url.size && !manager->load_url.size);
    memset(world, 'w', sizeof world - 1); world[sizeof world - 1] = 0;
    assert(owned_text_assign(&manager->world, text_view_cstr(world)));
    assert(owned_text_assign(&manager->load_url, text_view_cstr(world)));
    assert(owned_text_assign(&manager->save_url, text_view_cstr(world)));
    world[0] = 'x';
    assert(owned_text_cstr(&manager->world)[0] == 'w');
    manager->loaded = 1; manager->pending_saves = 3;
    assert(account_user_data_is_loaded(manager));
    assert(account_user_data_pending_saves(manager) == 3);
    account_user_data_save_answered(manager);
    assert(account_user_data_pending_saves(manager) == 2);
}
void service_test_character_state(void)
{
    struct CharUserDataMgr *manager = g_charUserDataMgr;
    char text[6001];
    assert(manager && !manager->aid && !manager->gid && !manager->loaded && !manager->pending_saves);
    assert(!strcmp(owned_text_cstr(&manager->world), "unknown"));
    assert(!manager->save_url.size && !manager->load_url.size);
    memset(text, 'c', sizeof text - 1); text[sizeof text - 1] = 0;
    assert(owned_text_assign(&manager->world, text_view_cstr(text)));
    assert(owned_text_assign(&manager->load_url, text_view_cstr(text)));
    assert(owned_text_assign(&manager->save_url, text_view_cstr(text)));
    text[0] = 'x';
    assert(owned_text_cstr(&manager->world)[0] == 'c');
    manager->pending_saves = 7;
    char_user_data_save_answered(manager);
    assert(manager->pending_saves == 6);
}
void service_test_agency_state(void)
{
    struct AdventurerAgencyMgr *manager = g_adventurerAgency;
    struct AdvenPartyInfo *bookings;
    char text[513];
    int i;
    assert(manager && !manager->recruiting && !manager->stop_master_aid && !manager->pending);
    assert(!manager->parties.items && !manager->parties.count);
    assert(adventurer_agency_parties(manager) == &manager->parties);
    bookings = calloc(65, sizeof *bookings);
    assert(bookings);
    memset(text, 'b', sizeof text - 1); text[sizeof text - 1] = 0;
    for (i = 0; i < 65; ++i) {
        party_booking_init(&bookings[i]);
        assert(!bookings[i].min_level && bookings[i].max_level == 999);
        assert(!bookings[i].roles[0] && !bookings[i].roles[3]);
        assert(owned_text_assign(&bookings[i].memo, text_view_cstr(text)));
        assert(owned_text_assign(&bookings[i].name, text_view_cstr(text)));
    }
    text[0] = 'x';
    assert(owned_text_cstr(&bookings[0].memo)[0] == 'b');
    manager->parties.items = bookings;
    manager->parties.count = 65;
    manager->pending = 4; manager->recruiting = 1;
    assert(adventurer_agency_pending_requests(manager) == 4);
    assert(adventurer_agency_is_recruiting(manager));
}

struct Session g_session1;
int service_urls_missing;

int session_service_urls(struct Session *session,
    struct OwnedText *first, const char *first_path,
    struct OwnedText *second, const char *second_path)
{
    (void)session;
    if (service_urls_missing) return 0;
    return owned_text_concat(first, text_view_cstr("https://service"), text_view_cstr(first_path))
        && owned_text_concat(second, text_view_cstr("https://service"), text_view_cstr(second_path));
}

void service_test_media_state(void)
{
    assert(!g_emblemMgr->upload_blocked);
    assert(!g_emblemMgr->upload_url.size && !g_emblemMgr->download_url.size);
    assert(emblem_mgr_build_urls(g_emblemMgr));
    assert(!strcmp(owned_text_cstr(&g_emblemMgr->upload_url), "https://service/emblem/upload"));
    assert(!strcmp(owned_text_cstr(&g_emblemMgr->download_url), "https://service/emblem/download"));
    assert(twitter_has_service(g_twitterDataMgr));
    assert(!strcmp(twitter_auth_url(g_twitterDataMgr), "https://service/twitter/user-auth"));
}
