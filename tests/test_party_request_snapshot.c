#include "web/party_works.h"
#include "web/http_post.h"
#include "session/state.h"
#include "session/character_name.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct AdventurerAgencyMgr *g_adventurerAgency;
struct HttpPost { int count; char names[16][32]; char values[16][128]; };
static struct HttpPost request;
static int perform_result, freed, answered;
static const char *const expected[][2] = {
    {"AID", "101"}, {"GID", "202"}, {"AuthToken", "0123456789abcdef"},
    {"WorldName", "world"}, {"CharName", "before"}, {"MinLV", "7"}, {"MaxLV", "90"},
    {"Tanker", "1"}, {"Healer", "0"}, {"Dealer", "1"}, {"Assist", "0"},
    {"Type", "4"}, {"Memo", "queued memo"}
};

struct HttpPost *http_post_create(const char *url)
{
    assert(!strcmp(url, "https://service/party/add"));
    memset(&request, 0, sizeof request); freed = 0;
    return &request;
}
int http_post_add_text(struct HttpPost *post, const char *name, const char *value)
{
    int index = post->count++;
    assert(post == &request && index < 16 && strlen(name) < 32 && strlen(value) < 128);
    strcpy(post->names[index], name); strcpy(post->values[index], value);
    return 0;
}
int http_post_perform(struct HttpPost *post)
{
    size_t i;
    assert(post == &request && post->count == sizeof expected / sizeof expected[0]);
    for (i = 0; i < sizeof expected / sizeof expected[0]; ++i) {
        assert(!strcmp(post->names[i], expected[i][0]));
        assert(!strcmp(post->values[i], expected[i][1]));
    }
    return perform_result;
}
const char *http_post_body(const struct HttpPost *post, size_t *size)
{
    assert(post == &request && !perform_result);
    if (size) *size = 2;
    return "{}";
}
void http_post_free(struct HttpPost *post) { assert(post == &request && !freed); freed = 1; }
char *http_text_to_utf8(const char *text)
{
    char *copy = malloc(strlen(text) + 1); assert(copy); strcpy(copy, text); return copy;
}
char *http_text_from_utf8(const char *text) { return http_text_to_utf8(text); }

void adventurer_agency_register_answered(struct AdventurerAgencyMgr *manager, int ok, const char *response)
{
    assert(manager == g_adventurerAgency && ok == !perform_result);
    if (ok) assert(!strcmp(response, "{}"));
    ++answered;
}
#define UNUSED_RESPONSE(name) \
    void name(struct AdventurerAgencyMgr *manager, int ok, const char *response) \
    { (void)manager; (void)ok; (void)response; assert(0); }
UNUSED_RESPONSE(adventurer_agency_change_master_answered)
UNUSED_RESPONSE(adventurer_agency_my_party_loaded)
UNUSED_RESPONSE(adventurer_agency_party_info_loaded)
UNUSED_RESPONSE(adventurer_agency_party_list_loaded)
UNUSED_RESPONSE(adventurer_agency_stop_recruiting_answered)
#undef UNUSED_RESPONSE

int main(void)
{
    static struct Session session;
    struct PartyRegisterWork work;
    struct AdvenPartyInfo booking;
    const char token[16] = "0123456789abcdef";
    int result;
    session_set_character_name(&session, text_view_cstr("before"));
    party_booking_init(&booking);
    booking.min_level = 7; booking.max_level = 90; booking.purpose = 4;
    booking.roles[0] = booking.roles[2] = 1;
    assert(owned_text_assign(&booking.name, text_view_cstr(session_character_name(&session))));
    assert(owned_text_assign(&booking.memo, text_view_cstr("queued memo")));
    party_register_work_construct(&work);
    assert(party_register_work_setup(&work.base, 101, 202, text_view_cstr("world"),
        text_view_cstr("https://service/party/add"), text_view(token, sizeof token), &booking));
    session_set_character_name(&session, text_view_cstr("after"));
    assert(owned_text_assign(&booking.name, text_view_cstr("different booking")));
    party_booking_clear(&booking);
    for (result = 0; result < 2; ++result) {
        perform_result = result;
        assert(!party_register_work_run(&work));
        assert(freed && work.succeeded == !result);
        assert(!strcmp(session_character_name(&session), "after"));
        party_register_work_done(&work);
    }
    assert(answered == 2);
    party_register_work_destruct(&work);
    puts("Party request snapshot: actual payload preparation and HTTP worker retain the queued name after session changes and caller cleanup; all 13 form fields and completion status passed");
    return 0;
}
