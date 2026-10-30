#include "contents/mcstore_work.h"
#include "contents/mcstore_backup.h"
#include "session/state.h"
#include "core/thread_text.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "render/math3d.h"
#include "base.h"
#include <stdlib.h>
#include <string.h>

static int take_response(struct OwnedText *response, struct HttpPost *post)
{
    int ok;
    char *body = http_text_from_utf8(http_post_body(post, NULL));
    if (!body) return 0;
    ok = owned_text_assign(response, text_view_cstr(body));
    free(body);
    return ok;
}

void mcstore_load_work_done(struct McStoreLoadWork *self)
{
    mcstore_on_loaded(&g_session1.mcStoreBackup, self->succeeded, owned_text_view(&self->response));
}

char mcstore_load_work_run(struct McStoreLoadWork *self)
{
    char aid[0x40], gid[0x40], type[0x40];
    char *world;
    struct HttpPost *post;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    memset(gid, 0, sizeof gid);
    _snprintf_s(gid, sizeof gid, (size_t)-1, "%lu", self->gid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    memset(type, 0, sizeof type);
    _snprintf_s(type, sizeof type, (size_t)-1, "%d", self->type);

    post = http_post_create(owned_text_cstr(&self->url));
    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "Type", type);
    http_post_add_text(post, "WorldName", world);
    if (http_post_perform(post) == 0 && take_response(&self->response, post)) {
        self->succeeded = 1;
    }
    http_post_free(post);
    free(world);
    return 0;
}

void mcstore_save_work_done(struct McStoreSaveWork *self)
{
    mcstore_on_saved(&g_session1.mcStoreBackup, self->succeeded, owned_text_view(&self->response));
}

char mcstore_save_work_run(struct McStoreSaveWork *self)
{
    char aid[0x40], gid[0x40], type[0x40];
    char *world, *data;
    struct HttpPost *post;
    int code;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    memset(gid, 0, sizeof gid);
    _snprintf_s(gid, sizeof gid, (size_t)-1, "%lu", self->gid);
    memset(type, 0, sizeof type);
    _snprintf_s(type, sizeof type, (size_t)-1, "%d", self->type);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    data = http_text_to_utf8(owned_text_cstr(&self->data));

    post = http_post_create(owned_text_cstr(&self->url));
    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "Type", type);
    http_post_add_text(post, "data", data);
    code = http_post_perform(post);
    if (!take_response(&self->response, post)) code = -1;
    if (code != 0)
        self->succeeded = 0;
    else
        self->succeeded = 1;
    http_post_free(post);
    free(data);
    free(world);
    return 0;
}
