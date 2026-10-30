#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <winsock2.h>
#include <windows.h>
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "contents/client_res.h"
#include "contents/schedule_storage.h"
#include "ui/window_messages.h"
#include "core/msg_str.h"

struct TimeScheduler *g_timeScheduler;
static const int shutdown_warning_seconds[6] = {1800, 900, 600, 300, 180, 60};
#define WARNING_STAGES 6
#define CHAT_YELLOW 0xffff
#define CHAT_RED 0xff
#define MSG_PARTIAL_SHUTDOWN_AT 0x92c
#define MSG_PARTIAL_SHUTDOWN_IN 0x92d

static void partial_job_run(struct Schedule *s, int now) { partial_shutdown_run((struct PartialShutDownSchedule *)s, now); }
static void captcha_job_run(struct Schedule *s, int now) { captcha_upload_run((struct CaptchaUploadSchedule *)s); }
static void captcha_job_destroy(struct Schedule *s) { captcha_upload_destroy((struct CaptchaUploadSchedule *)s); }
static const struct ScheduleOps plain_ops = {schedule_run, schedule_free};
static const struct ScheduleOps partial_ops = {partial_job_run, schedule_free};
static const struct ScheduleOps captcha_ops = {captcha_job_run, captcha_job_destroy};
static const struct ScheduleOps gdi_ops = {gdi_check_run, schedule_free};

static void chat_line(const char *text, int color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)text, color, 0, 0);
}

struct Schedule *schedule_construct(struct Schedule *self, int now, int interval, int kind, int id, int tag)
{
    *self = (struct Schedule){0};
    self->ops = &plain_ops;
    self->id = id;
    self->last_run = now;
    self->interval = interval;
    self->kind = kind;
    self->tag = tag;
    self->alive = 1;
    return self;
}

struct Schedule *schedule_create(int now, int interval, int kind, const struct ScheduleOps *ops)
{
    struct Schedule *self = malloc(sizeof *self);
    if (!self) return NULL;
    schedule_construct(self, now, interval, kind, schedule_next_id(), 0);
    if (ops) self->ops = ops;
    return self;
}

void schedule_free(struct Schedule *self) { free(self); }
void schedule_destroy(struct Schedule *self) { if (self) self->ops->destroy(self); }

char schedule_is_due(struct Schedule *self, int now)
{
    return self->interval > 0 && (int64_t)self->last_run + self->interval < now;
}

void schedule_run(struct Schedule *self, int now)
{
    if (self->kind == SCHEDULE_ONCE) { self->alive = 0; return; }
    if (self->kind == SCHEDULE_RANDOM)
        self->interval = (int)(300.0 * (rand() / (double)RAND_MAX) + 30.0);
    self->last_run = now;
}

struct TimeScheduler *time_scheduler_construct(struct TimeScheduler *self)
{
    *self = (struct TimeScheduler){0};
    return self;
}

void time_scheduler_clear(struct TimeScheduler *self)
{
    struct ScheduleNode *node, *next;
    if (!self || self->clearing) return;
    if (self->ticking) { self->clear_requested = 1; return; }
    self->clearing = 1;
    for (node = self->first; node; node = node->next) {
        schedule_destroy(node->job);
        node->job = NULL;
    }
    for (node = self->first; node; node = next) { next = node->next; free(node); }
    *self = (struct TimeScheduler){0};
}

void time_scheduler_destruct(struct TimeScheduler *self) { time_scheduler_clear(self); }

char time_scheduler_create(void)
{
    struct TimeScheduler *self;
    if (g_timeScheduler) return 0;
    self = malloc(sizeof *self);
    if (!self) return 0;
    g_timeScheduler = time_scheduler_construct(self);
    return 1;
}

void time_scheduler_destroy(void)
{
    if (!g_timeScheduler) return;
    time_scheduler_destruct(g_timeScheduler);
    free(g_timeScheduler);
    g_timeScheduler = NULL;
}

struct Schedule *time_scheduler_find(struct TimeScheduler *self, int id)
{
    struct ScheduleNode *node;
    if (!self) return NULL;
    for (node = self->first; node; node = node->next)
        if (node->job && node->job->id == id) return node->job;
    return NULL;
}

struct Schedule *time_scheduler_add(struct TimeScheduler *self, struct Schedule *job)
{
    struct ScheduleNode *node;
    __time32_t now;
    if (!self || !job || self->clearing || self->clear_requested || self->count == (size_t)-1) return NULL;
    for (node = self->first; node; node = node->next)
        if (node->job == job) return NULL;
    node = malloc(sizeof *node);
    if (!node) return NULL;
    _time32(&now);
    job->last_run = now;
    node->job = job;
    node->next = NULL;
    node->prev = self->last;
    if (self->last) self->last->next = node;
    else self->first = node;
    self->last = node;
    ++self->count;
    return job;
}

static int time32_local(long value, struct tm *out)
{
    __time32_t time = value;
    return _localtime32_s(out, &time) == 0;
}

struct PartialShutDownSchedule *partial_shutdown_construct(struct PartialShutDownSchedule *self, int now, int interval, int kind)
{
    *self = (struct PartialShutDownSchedule){0};
    schedule_construct(&self->base, now, interval, kind, schedule_next_id(), 0);
    self->base.ops = &partial_ops;
    return self;
}

void partial_shutdown_next_warning(struct PartialShutDownSchedule *self)
{
    int64_t left;

    for (; self->stage < WARNING_STAGES; self->stage++) {
        left = (int64_t)self->remaining - shutdown_warning_seconds[self->stage];
        if (left > 0) {
            self->base.interval = (int)left;
            return;
        }
    }
}

int partial_shutdown_start(struct PartialShutDownSchedule *self, long end, long now)
{
    struct tm local;
    if (!self || end <= now || !time32_local(end, &local) || !time32_local(now, &local)) return 0;
    self->now = now;
    self->end = end;
    self->stage = 0;
    self->remaining = (int)(end - now);
    if (self->remaining < 3600) partial_shutdown_next_warning(self);
    else if (self->remaining - 3600 < 3600) self->base.interval = self->remaining - 3000;
    return 1;
}

struct PartialShutDownSchedule *partial_shutdown_create(int now, int interval, int kind, long end)
{
    struct PartialShutDownSchedule *self = malloc(sizeof *self);
    if (!self) return NULL;
    partial_shutdown_construct(self, now, interval, kind);
    if (!partial_shutdown_start(self, end, now)) { free(self); return NULL; }
    return self;
}

void partial_shutdown_run(struct PartialShutDownSchedule *self, int now)
{
    char line[0x100];
    struct tm local;
    schedule_run(&self->base, now);
    self->remaining = (int)(_difftime32(self->end, now));
    if (self->remaining >= 3600) {
        snprintf(line, sizeof line, msg_str(MSG_PARTIAL_SHUTDOWN_IN), self->remaining / 3600, self->remaining % 3600 / 60);
        chat_line(line, CHAT_YELLOW);
        return;
    }
    if (self->stage >= WARNING_STAGES) { self->base.alive = 0; return; }
    if (!game_current_base_mode()) return;
    if (!time32_local(self->end, &local)) { self->base.alive = 0; return; }
    snprintf(line, sizeof line, msg_str(MSG_PARTIAL_SHUTDOWN_AT), local.tm_year + 1900,
             local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    chat_line(line, CHAT_YELLOW);
    self->stage++;
    partial_shutdown_next_warning(self);
}

#define CAPTCHA_CHUNK 0x400
#define PACKET_CAPTCHA_UPLOAD 0x0a54

struct CaptchaUploadSchedule *captcha_upload_create(int now, int key, int size, const void *data)
{
    struct CaptchaUploadSchedule *self;
    if (size < 0 || (size && !data)) return NULL;
    self = malloc(sizeof *self);
    if (!self) return NULL;
    self->data = size ? malloc((size_t)size) : NULL;
    if (size && !self->data) { free(self); return NULL; }
    if (size) memcpy(self->data, data, (size_t)size);
    schedule_construct(&self->base, now, 1, SCHEDULE_REPEAT, schedule_next_id(), 0);
    self->base.ops = &captcha_ops;
    self->size = size;
    self->key = key;
    self->sent = 0;
    return self;
}

void captcha_upload_destruct(struct CaptchaUploadSchedule *self)
{
    free(self->data);
    self->data = NULL;
    self->size = self->sent = 0;
}

void captcha_upload_destroy(struct CaptchaUploadSchedule *self)
{
    captcha_upload_destruct(self);
    free(self);
}

void captcha_upload_run(struct CaptchaUploadSchedule *self)
{
    struct {
        unsigned short id, len;
        int key;
        unsigned char data[CAPTCHA_CHUNK];
    } packet;
    int chunk = self->size - self->sent;
    if (chunk > 0) {
        if (chunk > CAPTCHA_CHUNK) chunk = CAPTCHA_CHUNK;
        packet.id = PACKET_CAPTCHA_UPLOAD;
        packet.len = (unsigned short)(8 + chunk);
        packet.key = self->key;
        memcpy(packet.data, self->data + self->sent, (size_t)chunk);
        rag_connection_send_packet(rag_connection_instance(), packet.len, &packet);
        self->sent += chunk;
    }
    if (self->sent == self->size) self->base.alive = 0;
}

void gdi_check_run(struct Schedule *self, int now)
{
     (void)now;
    self->alive = 0;
}

struct Schedule *gdi_check_create(int now, int interval, int kind)
{
    return schedule_create(now, interval, kind, &gdi_ops);
}
