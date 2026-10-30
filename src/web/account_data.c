#include "session/chat_macros.h"
#include "web/service_manager_state.h"
#include "web/account_data.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "json/service_json.h"
#include "contents/game_session.h"
#include "contents/user_hotkeys.h"
#include "contents/whisper_block.h"

char account_data_load_work_run(struct AccountDataLoadWork *self)
{
    char aid[0x40];
    char *world, *body;
    struct HttpPost *post;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "WorldName", world);

    if (http_post_perform(post) == 0 && (body = http_text_from_utf8(http_post_body(post, NULL))) != NULL) {
        self->succeeded = owned_text_assign(&self->response, text_view_cstr(body));
        free(body);
    }
    http_post_free(post);
    free(world);
    return 0;
}

void account_data_load_work_done(struct AccountDataLoadWork *self)
{
    account_user_data_on_loaded(g_accountUserDataMgr, self->succeeded,
        self->aid, owned_text_cstr(&self->world), owned_text_cstr(&self->response));
}

char account_data_save_work_run(struct AccountDataSaveWork *self)
{
    char aid[0x40];
    char *world, *data;
    struct HttpPost *post;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    data = http_text_to_utf8(owned_text_cstr(&self->data));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "data", data);

    if (http_post_perform(post) != 0)
        self->succeeded = 0;
    else
        self->succeeded = 1;
    http_post_free(post);
    free(data);
    free(world);
    return 0;
}

void account_data_save_work_done(struct AccountDataSaveWork *self)
{
    (void)self;

    account_user_data_save_answered(g_accountUserDataMgr);
}

static void build_urls(struct AccountUserDataMgr *self)
{
    session_service_urls(&g_session1, &self->save_url, "/userconfig/save",
        &self->load_url, "/userconfig/load");
}

void account_user_data_load(struct AccountUserDataMgr *self, int aid, const char *world)
{
    size_t world_size = strlen(world);

    if (self->aid != aid || self->world.size != world_size
        || memcmp(owned_text_cstr(&self->world), world, world_size) != 0) {
        async_work_mgr_account_data_load(g_asyncWorkMgr, aid, text_view(world, world_size),
            owned_text_view(&self->load_url));
    }
}

void account_user_data_reset(struct AccountUserDataMgr *self)
{
    self->aid = 0;
    owned_text_clear(&self->world);
    owned_text_assign(&self->world, text_view_cstr("unknown"));
    self->loaded = 0;
    whisper_block_reset(g_whisperBlockMgr);
    text_array_clear(&g_session1.chatMacrosLoaded);
    user_hotkey_mgr_reset(g_userHotkeyMgr);
    build_urls(self);
}
