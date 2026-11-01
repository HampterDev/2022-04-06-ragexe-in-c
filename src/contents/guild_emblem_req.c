#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdint.h>
#include <string.h>
#include "contents/guild_emblem_transport.h"
#include "contents/guild_emblem_req.h"
#include "core/fatal.h"
#include "core/memory.h"

#define EMBLEM_RESEND_AFTER_MS 60000

struct EmblemRequest {
    int guild;
    int version;
};

struct SentEmblemRequest {
    struct EmblemRequest request;
    DWORD sent_at;
};

struct EmblemQueue {
    unsigned char *items;
    size_t capacity, first, count;
};

struct GuildEmblemReqBundle {
    struct EmblemQueue waiting, sent;
};

static void *queue_at(const struct EmblemQueue *queue, size_t index, size_t item_size)
{
    return queue->items + ((queue->first + index) % queue->capacity) * item_size;
}

static void queue_append(struct EmblemQueue *queue, const void *item, size_t item_size)
{
    if (queue->count == queue->capacity) {
        size_t capacity, i;
        unsigned char *items;

        if (queue->capacity > SIZE_MAX / item_size / 2)
            fatal_error("emblem request queue too long");
        capacity = queue->capacity ? queue->capacity * 2 : 8;
        items = mem_alloc(capacity * item_size);
        for (i = 0; i < queue->count; ++i)
            memcpy(items + i * item_size, queue_at(queue, i, item_size), item_size);
        mem_free(queue->items);
        queue->items = items;
        queue->capacity = capacity;
        queue->first = 0;
    }
    memcpy(queue_at(queue, queue->count, item_size), item, item_size);
    ++queue->count;
}

static void queue_pop(struct EmblemQueue *queue)
{
    if (--queue->count)
        queue->first = (queue->first + 1) % queue->capacity;
    else
        queue->first = 0;
}

struct GuildEmblemReqBundle *emblem_bundle_create(void)
{
    struct GuildEmblemReqBundle *bundle = mem_try_alloc(sizeof *bundle);

    if (bundle)
        memset(bundle, 0, sizeof *bundle);
    return bundle;
}

void emblem_bundle_destroy(struct GuildEmblemReqBundle *bundle)
{
    if (!bundle)
        return;
    mem_free(bundle->sent.items);
    mem_free(bundle->waiting.items);
    mem_free(bundle);
}

static void expire_sent(struct GuildEmblemReqBundle *bundle)
{
    while (bundle->sent.count) {
        const struct SentEmblemRequest *sent = queue_at(&bundle->sent, 0, sizeof *sent);

        if (timeGetTime() < sent->sent_at + EMBLEM_RESEND_AFTER_MS)
            break;
        queue_pop(&bundle->sent);
    }
}

static int was_sent(struct GuildEmblemReqBundle *bundle, struct EmblemRequest request)
{
    size_t i;

    for (i = 0; i < bundle->sent.count; ++i) {
        const struct SentEmblemRequest *sent = queue_at(&bundle->sent, i, sizeof *sent);

        if (sent->request.guild == request.guild && sent->request.version == request.version)
            return 1;
    }
    return 0;
}

void emblem_bundle_queue(struct GuildEmblemReqBundle *bundle, int guild, int version)
{
    size_t i;
    struct EmblemRequest request = {guild, version};

    for (i = 0; i < bundle->waiting.count; ++i) {
        const struct EmblemRequest *waiting = queue_at(&bundle->waiting, i, sizeof *waiting);

        if (waiting->guild == guild && waiting->version == version)
            return;
    }
    queue_append(&bundle->waiting, &request, sizeof request);
}

void emblem_bundle_process(struct GuildEmblemReqBundle *bundle)
{
    expire_sent(bundle);
    while (bundle->waiting.count) {
        struct EmblemRequest request = *(struct EmblemRequest *)queue_at(&bundle->waiting, 0, sizeof request);
        struct SentEmblemRequest sent;

        queue_pop(&bundle->waiting);
        if (was_sent(bundle, request))
            continue;
        if (!request.guild)
            return;
        if (!guild_emblem_request(request.guild, request.version)) {
            emblem_bundle_queue(bundle, request.guild, request.version);
            return;
        }
        sent.request = request;
        sent.sent_at = timeGetTime();
        queue_append(&bundle->sent, &sent, sizeof sent);
        return;
    }
}
