#include <stdlib.h>
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "web/service_manager_state.h"
#include <string.h>
#include <windows.h>
#include "base.h"
#include "web/work.h"
#include "web/http_post.h"
#include "web/requests.h"
#include "contents/client_res.h"
#include "contents/game_session.h"
#include "std/std.h"
#include "contents/twitter.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"

#define MSG_TWEET_SENT 0xca8
#define MSG_TWEET_FAILED 0xd96
#define CHAT_RED 0xff

void twitter_on_sent(struct TwitterDataMgr *self, int succeeded)
{
    (void)self;

    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_LINE,
                           (int)msg_str(succeeded == 1 ? MSG_TWEET_SENT : MSG_TWEET_FAILED), CHAT_RED, 0, 0);
}

char twitter_send_work_run(struct TwitterSendWork *self)
{
    char aid[0x40];
    char *tweet, *image;
    struct HttpPost *post;
    int rc;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);

    tweet = http_text_to_utf8(owned_text_cstr(&self->tweet));
    image = http_text_to_utf8(owned_text_cstr(&self->image));

    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_file(post, "image", image);
    http_post_add_text(post, "tweet", tweet);
    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "submit", "submit");

    rc = http_post_perform(post);
    if (rc != 0)
        self->succeeded = 0;
    else
        self->succeeded = 1;

    http_post_free(post);
    free(image);
    free(tweet);
    return 0;
}

void twitter_send_work_done(struct TwitterSendWork *self)
{
    twitter_on_sent(g_twitterDataMgr, self->succeeded);
}
