#include "core/owned_text.h"
#ifndef CONTENTS_TWITTER_H
#define CONTENTS_TWITTER_H

#include "base.h"
#include "web/work.h"
#include "web/http_post.h"
#include "web/requests.h"
#include "web/service_managers.h"

struct TwitterSendWork {
    struct AsyncWork base;
    unsigned aid;
    struct OwnedText url;
    struct OwnedText tweet;
    struct OwnedText image;
    struct OwnedText token;
    int succeeded;
};

struct TwitterSendWork *twitter_send_work_construct(struct TwitterSendWork *self);
void twitter_send_work_destruct(struct TwitterSendWork *self);
int twitter_send_work_init(struct TwitterSendWork *self, unsigned aid, struct TextView token,
    struct TextView url, struct TextView tweet, struct TextView image);
char twitter_send_work_run(struct TwitterSendWork *self);
void twitter_send_work_done(struct TwitterSendWork *self);

#endif
