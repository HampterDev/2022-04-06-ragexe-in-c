#include <stdint.h>
#include "ui/window_manager_state.h"
#include "core/markup_tags.h"
#include <stdlib.h>
#include <string.h>
#include "contents/client_res.h"
#include "std/std.h"
#include "contents/player_names.h"
#include "contents/ranking.h"
#include "contents/ranking_style.h"
#include "contents/ranking_output.h"
#include "contents/schedule.h"
#include "ui/window_messages.h"
#include "core/msg_str.h"

#define CHAT_YELLOW 0xffff
#define MSG_RANK_NAMES_PENDING 0xd99
#define MSG_RANK_NO_NAME 0x245

static void ranking_job_run(struct Schedule *job, int now)
{
    ranking_run((struct RankingSchedule *)job, now);
}
static void ranking_job_destroy(struct Schedule *job)
{
    ranking_destroy((struct RankingSchedule *)job);
}
static const struct ScheduleOps ranking_ops = {ranking_job_run, ranking_job_destroy};

static void rank_entry_name(const struct RankingSchedule *self, int char_id, struct String *name)
{
    size_t i;
    struct TextView found;

    if (!char_id)
        return;
    for (i = 0; i < self->nameless_count; ++i) {
        if (self->nameless[i] == char_id) {
            string_assign(name, string_cstr(&text_tag_NAMELESS), text_tag_NAMELESS.size);
            break;
        }
    }
    if (text_view_find(string_view(name), string_view(&text_tag_NAMELESS), 0) != STRING_NPOS)
        return;
    found = player_name_lookup(char_id);
    string_assign(name, found.data, found.size);
}

struct RankingSchedule *ranking_create(int now, int interval, int kind)
{
    struct RankingSchedule *ranking = malloc(sizeof *ranking);
    return ranking ? ranking_construct(ranking, now, interval, kind) : NULL;
}

struct RankingSchedule *ranking_construct(struct RankingSchedule *self, int now, int interval,
                                                     int kind)
{
    schedule_construct(&self->base, now, interval, kind, schedule_next_id(), 0);
    self->base.ops = &ranking_ops;
    self->timeout = 0;
    self->nameless = NULL;
    self->nameless_count = self->nameless_capacity = 0;
    memset(&self->rank, 0, sizeof self->rank);
    return self;
}

void ranking_destruct(struct RankingSchedule *self)
{
    self->base.ops = &ranking_ops;
    free(self->nameless);
    self->nameless = NULL;
    self->nameless_count = self->nameless_capacity = 0;
}

void ranking_destroy(struct RankingSchedule *self)
{
    if (!self) return;
    ranking_destruct(self);
    free(self);
}

void ranking_run(struct RankingSchedule *self, int now)
{
    struct String name;
    char all_named = 1;
    int i;

    if (!self->rank.packet_id || self->timeout <= 0) {
        self->base.alive = 0;
        return;
    }
    if ((int64_t)self->base.last_run + self->timeout > now) {
        for (i = 0; i < RANK_ENTRIES; i++) {
            string_init(&name);
            if (self->rank.char_ids[i]) {
                rank_entry_name(self, self->rank.char_ids[i], &name);
                if (!(text_view_find(string_view(&name), string_view(&text_tag_NAMELESS), 0) != STRING_NPOS) && !name.size)
                    all_named = 0;
            }
            string_free(&name);
        }
        if (!all_named) {
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_RANK_NAMES_PENDING),
                                   CHAT_YELLOW, 0, 0);
            return;
        }
    }
    ranking_print(self);
    self->base.alive = 0;
}

void ranking_set(struct RankingSchedule *self, const struct RankPacket *packet)
{
    memcpy(&self->rank, packet, sizeof self->rank);
}

void ranking_print(struct RankingSchedule *self)
{
    const struct RankingStyle *style = ranking_style(self->rank.rank_type);
    int i;
    struct String name;
    const char *no_name;

    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)style->title, style->title_color, 0, 0);

    for (i = 0; i < RANK_ENTRIES; i++) {
        string_init(&name);
        rank_entry_name(self, self->rank.char_ids[i], &name);
        if (!name.size || text_view_find(string_view(&name), string_view(&text_tag_NAMELESS), 0) != STRING_NPOS) {
            no_name = msg_str(MSG_RANK_NO_NAME);
            string_assign_cstr(&name, no_name);
        }
        ranking_line(style->line_color, "[ %2d ] %s  :  %4d POINT", i + 1, string_cstr(&name), self->rank.points[i]);
        string_free(&name);
    }

    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)RANK_SEPARATOR, style->title_color, 0, 0);
    ranking_line(style->line_color, "MY POINT    :  %4d POINT ", self->rank.my_points);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)RANK_SEPARATOR, style->title_color, 0, 0);
}
