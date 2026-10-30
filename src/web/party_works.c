#include "web/service_managers.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "web/party_works.h"
#include "contents/game_session.h"

static void format_lu(char buf[0x40], unsigned value)
{
    memset(buf, 0, 0x40);
    _snprintf_s(buf, 0x40, (size_t)-1, "%lu", value);
}

static void format_d(char buf[10], int value)
{
    memset(buf, 0, 10);
    _snprintf_s(buf, 10, (size_t)-1, "%d", value);
}

static int take_response(struct HttpPost *post, struct OwnedText *response)
{
    int ok;
    char *body = http_text_from_utf8(http_post_body(post, NULL));
    if (!body) return 0;
    ok = owned_text_assign(response, text_view_cstr(body));
    free(body);
    return ok;
}

char party_change_master_work_run(struct PartyChangeMasterWork *self)
{
    char aid[0x40], gid[0x40], to_aid[0x40], to_gid[0x40];
    char *world, *name, *to_name;
    struct HttpPost *post;

    format_lu(aid, self->aid);
    format_lu(gid, self->gid);
    format_lu(to_aid, self->to_aid);
    format_lu(to_gid, self->to_gid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    name = http_text_to_utf8(owned_text_cstr(&self->name));
    to_name = http_text_to_utf8(owned_text_cstr(&self->to_name));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "CharName", name);
    http_post_add_text(post, "To_AID", to_aid);
    http_post_add_text(post, "To_GID", to_gid);
    http_post_add_text(post, "To_CharName", to_name);
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));

    if (http_post_perform(post) != 0 || !take_response(post, &self->response)) {
        self->succeeded = 0;
    } else {
        self->succeeded = 1;
    }
    http_post_free(post);
    free(to_name);
    free(name);
    free(world);
    return 0;
}

void party_change_master_work_done(struct PartyChangeMasterWork *self)
{
    adventurer_agency_change_master_answered(g_adventurerAgency, self->succeeded, owned_text_cstr(&self->response));
}

char my_party_load_work_run(struct MyPartyLoadWork *self)
{
    char aid[0x40], gid[0x40];
    char *world;
    struct HttpPost *post;

    format_lu(aid, self->aid);
    format_lu(gid, self->gid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));

    if (http_post_perform(post) != 0 || !take_response(post, &self->response)) {
        self->succeeded = 0;
    } else {
        self->succeeded = 1;
    }
    http_post_free(post);
    free(world);
    return 0;
}

void my_party_load_work_done(struct MyPartyLoadWork *self)
{
    adventurer_agency_my_party_loaded(g_adventurerAgency, self->succeeded, owned_text_cstr(&self->response));
}

char party_info_load_work_run(struct PartyInfoLoadWork *self)
{
    char aid[0x40], gid[0x40], query_aid[0x40];
    char *world;
    struct HttpPost *post;

    format_lu(aid, self->aid);
    format_lu(gid, self->gid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    format_lu(query_aid, self->query_aid);
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "QueryAID", query_aid);

    if (http_post_perform(post) == 0 && take_response(post, &self->response)) {
        self->succeeded = 1;
    }
    http_post_free(post);
    free(world);
    return 0;
}

void party_info_load_work_done(struct PartyInfoLoadWork *self)
{
    adventurer_agency_party_info_loaded(g_adventurerAgency, self->succeeded, owned_text_cstr(&self->response));
}

char party_list_load_work_run(struct PartyListLoadWork *self)
{
    char aid[0x40], gid[0x40], page[10], min_level[10], max_level[10], purpose[10];
    char *world, *memo;
    struct HttpPost *post;

    format_lu(aid, self->aid);
    format_lu(gid, self->gid);
    format_d(page, self->page);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));

    switch (self->mode) {
    case 0:
        http_post_add_text(post, "page", page);
        break;
    case 1:
        format_d(min_level, self->info.min_level);
        http_post_add_text(post, "MinLV", min_level);
        format_d(max_level, self->info.max_level);
        http_post_add_text(post, "MaxLV", max_level);
        if (self->info.roles[0])
            http_post_add_text(post, "Tanker", "1");
        if (self->info.roles[2])
            http_post_add_text(post, "Dealer", "1");
        if (self->info.roles[1])
            http_post_add_text(post, "Healer", "1");
        if (self->info.roles[3])
            http_post_add_text(post, "Assist", "1");
        if (self->info.purpose != 0) {
            format_d(purpose, self->info.purpose);
            http_post_add_text(post, "Type", purpose);
        }
        if (self->info.memo.size != 0) {
            memo = http_text_to_utf8(owned_text_cstr(&self->info.memo));
            http_post_add_text(post, "Memo", memo);
            free(memo);
        }
        http_post_add_text(post, "page", page);
        break;
    default:
        break;
    }

    if (http_post_perform(post) == 0 && take_response(post, &self->response)) {
        self->succeeded = 1;
    }
    http_post_free(post);
    free(world);
    return 0;
}

void party_list_load_work_done(struct PartyListLoadWork *self)
{
    adventurer_agency_party_list_loaded(g_adventurerAgency, self->succeeded, owned_text_cstr(&self->response));
}

char party_register_work_run(struct PartyRegisterWork *self)
{
    char aid[0x40], gid[0x40], min_level[10], max_level[10], purpose[10];
    char *world, *char_name, *memo;
    struct HttpPost *post;

    format_lu(aid, self->aid);
    format_lu(gid, self->gid);
    format_d(min_level, self->info.min_level);
    format_d(max_level, self->info.max_level);
    format_d(purpose, self->info.purpose);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    char_name = http_text_to_utf8(owned_text_cstr(&self->info.name));
    memo = http_text_to_utf8(owned_text_cstr(&self->info.memo));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "CharName", char_name);
    http_post_add_text(post, "MinLV", min_level);
    http_post_add_text(post, "MaxLV", max_level);
    http_post_add_text(post, "Tanker", self->info.roles[0] ? "1" : "0");
    http_post_add_text(post, "Healer", self->info.roles[1] ? "1" : "0");
    http_post_add_text(post, "Dealer", self->info.roles[2] ? "1" : "0");
    http_post_add_text(post, "Assist", self->info.roles[3] ? "1" : "0");
    http_post_add_text(post, "Type", purpose);
    http_post_add_text(post, "Memo", memo);

    if (http_post_perform(post) != 0 || !take_response(post, &self->response)) {
        self->succeeded = 0;
    } else {
        self->succeeded = 1;
    }
    http_post_free(post);
    free(memo);
    free(char_name);
    free(world);
    return 0;
}

void party_register_work_done(struct PartyRegisterWork *self)
{
    adventurer_agency_register_answered(g_adventurerAgency, self->succeeded, owned_text_cstr(&self->response));
}

char party_stop_recruiting_work_run(struct PartyStopRecruitingWork *self)
{
    char aid[0x40], master_aid[0x40];
    char *world;
    struct HttpPost *post;

    format_lu(aid, self->aid);
    format_lu(master_aid, self->master_aid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "MasterAID", master_aid);

    if (http_post_perform(post) != 0 || !take_response(post, &self->response)) {
        self->succeeded = 0;
    } else {
        self->succeeded = 1;
    }
    http_post_free(post);
    free(world);
    return 0;
}

void party_stop_recruiting_work_done(struct PartyStopRecruitingWork *self)
{
    adventurer_agency_stop_recruiting_answered(g_adventurerAgency, self->succeeded, owned_text_cstr(&self->response));
}
