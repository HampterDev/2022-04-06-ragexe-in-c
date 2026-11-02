#include "contents/party_booking.h"
#include "contents/party_booking_regions.h"
#include "contents/client_res.h"
#include "contents/sound.h"
#include "contents/ui_window_mgr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core/msg_str.h"

#define PARTY_BOOKING_WINDOW 0xa9
#define MSG_PARTY_BOOKING_NEW 0x6d9
#define MSG_JOB_NAME_LOW 0x6af
#define MSG_JOB_NAME 0x658
#define LEVEL_SPAN 15
#define PENDING_BATCH 11

static int format_jobs(struct OwnedText *out, const short *jobs)
{
    struct OwnedText text = {0};
    int i;
    for (i = 0; i < PARTY_BOOKING_JOBS; ++i) {
        const char *name;
        if (jobs[i] < 0)
            continue;
        name = msg_str((jobs[i] <= 4 ? MSG_JOB_NAME_LOW : MSG_JOB_NAME) + jobs[i]);
        if (!owned_text_concat(&text, owned_text_view(&text), text_view_cstr(name)) ||
            !owned_text_concat(&text, owned_text_view(&text), text_view(",", 1))) {
            owned_text_clear(&text);
            return 0;
        }
    }
    if (text.size > 2)
        text.data[--text.size] = 0;
    owned_text_clear(out);
    *out = text;
    return 1;
}

struct PartyBooking *party_booking_construct(struct PartyBooking *self)
{
    memset(self, 0, sizeof *self);
    self->queue_new = 1;
    self->last_id = -1;
    self->filter_level = self->filter_map = self->filter_job = -1;
    return self;
}

void party_booking_destruct(struct PartyBooking *self)
{
    party_search_rows_clear(&self->pending);
    party_search_rows_clear(&self->rows);
    party_search_regions_clear(&self->region_maps);
    text_array_clear(&self->region_names);
}

void party_booking_clear_rows(struct PartyBooking *self)
{
    party_search_rows_clear(&self->rows);
    party_search_rows_clear(&self->pending);
}

void party_booking_reset(struct PartyBooking *self)
{
    self->queue_new = 1;
    self->last_id = -1;
    self->window_open = 0;
    party_booking_clear_rows(self);
}

void party_booking_set_filter(struct PartyBooking *self, int level, int map, int job)
{
    self->filter_level = level;
    self->filter_map = map;
    self->filter_job = job;
}

char party_booking_filter_matches(struct PartyBooking *self, struct PartyBookingPost post)
{
    int i;
    if (self->filter_level > 0 &&
        (self->filter_level > post.level || post.level - self->filter_level > LEVEL_SPAN))
        return 0;
    if (self->filter_map > 0 && self->filter_map != post.map)
        return 0;
    if (self->filter_job <= 0)
        return 1;
    for (i = 0; i < PARTY_BOOKING_JOBS; ++i)
        if (self->filter_job == post.jobs[i])
            return 1;
    return 0;
}

int party_booking_make_row(struct PartyBooking *self, struct PartySearchRow *out,
                           struct PartyBookingPost post)
{
    struct PartySearchRow row = {0};
    const struct PartySearchMaps *maps;
    const struct PartySearchMap *map;
    const struct tm *posted;
    __time32_t timestamp = post.posted_at;
    int region = post.map >> 8, level_top = post.level + LEVEL_SPAN;
    size_t name_index = region ? (size_t)(region - 1) : 0;
    size_t leader_size = 0;
    char text[64];

    if (name_index >= self->region_names.size)
        return 0;
    maps = party_search_region_maps(&self->region_maps, region + 1);
    map = party_search_map_at(maps, post.map & 0xff);
    if (!map)
        return 0;
    posted = _localtime32(&timestamp);
    if (!posted)
        return 0;
    sprintf(text, " %02d : %02d ", posted->tm_hour, posted->tm_min);
    while (leader_size < sizeof post.leader && post.leader[leader_size])
        ++leader_size;
    if (!owned_text_assign(&row.time, text_view_cstr(text)) ||
        !owned_text_assign(&row.leader, text_view(post.leader, leader_size)))
        goto fail;
    if (level_top > g_session1.baseLevel3rd)
        level_top = g_session1.baseLevel3rd;
    sprintf(text, "%02d~%02d", post.level, level_top);
    if (!owned_text_assign(&row.levels, text_view_cstr(text)) ||
        !owned_text_concat(&row.place, text_array_at(&self->region_names, name_index), text_view(" ", 1)) ||
        !owned_text_concat(&row.place, owned_text_view(&row.place), owned_text_view(&map->name)) ||
        !format_jobs(&row.jobs, post.jobs))
        goto fail;
    party_search_row_clear(out);
    *out = row;
    return 1;
fail:
    party_search_row_clear(&row);
    return 0;
}

static void add_post(struct PartyBooking *self, const struct PartyBookingPost *post, char *queue_new)
{
    struct PartySearchRow row = {0};
    if (party_booking_make_row(self, &row, *post) &&
        party_search_rows_insert(&self->rows, post->id, &row)) {
        self->last_id = post->id;
        if (!*queue_new)
            *queue_new = party_booking_take_pending(self);
    }
    party_search_row_clear(&row);
}

static void add_more(struct PartyBooking *self, char queue_new)
{
    if (queue_new == 1) {
        struct PartySearchRow more = {0};
        int key = self->last_id == INT_MAX ? INT_MIN : self->last_id + 1;
        more.is_more_row = 1;
        party_search_rows_insert(&self->rows, key, &more);
    }
}

void party_booking_add_rows(struct PartyBooking *self, const struct PartyBookingPost *posts,
                            int count, char queue_new)
{
    self->queue_new = queue_new;
    for (; count > 0; --count, ++posts)
        add_post(self, posts, &queue_new);
    add_more(self, queue_new);
}

#pragma pack(push, 1)
struct PACKET_ZC_PARTY_BOOKING_ACK_SEARCH {
    unsigned short packet_type;
    unsigned short packet_length;
    char more;
    unsigned char posts[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PARTY_BOOKING_ACK_SEARCH, 5);

int party_booking_decode_search(struct PartyBookingSearch *out, const unsigned char *packet, size_t size)
{
    const struct PACKET_ZC_PARTY_BOOKING_ACK_SEARCH *head = (const struct PACKET_ZC_PARTY_BOOKING_ACK_SEARCH *)packet;

    if (!packet || size < sizeof *head)
        return 0;
    if (head->packet_length != size || (size - sizeof *head) % sizeof(struct PartyBookingPost))
        return 0;
    out->posts = head->posts;
    out->count = (size - sizeof *head) / sizeof(struct PartyBookingPost);
    out->more = head->more;
    return 1;
}

void party_booking_add_search(struct PartyBooking *self, const struct PartyBookingSearch *search)
{
    char queue_new = search->more;
    size_t i;
    self->queue_new = queue_new;
    for (i = 0; i < search->count; ++i) {
        struct PartyBookingPost post;
        memcpy(&post, search->posts + i * sizeof post, sizeof post);
        add_post(self, &post, &queue_new);
    }
    add_more(self, queue_new);
}

char party_booking_take_pending(struct PartyBooking *self)
{
    unsigned moved;
    for (moved = 0; moved < PENDING_BATCH && self->pending.size; ++moved) {
        const struct PartySearchEntry *first = party_search_rows_at(&self->pending, 0);
        int key = first->key;
        if (!party_search_rows_insert(&self->rows, key, &first->row))
            break;
        party_search_rows_remove(&self->pending, key);
    }
    return self->pending.size != 0;
}

void party_booking_on_new(struct PartyBooking *self, struct PartyBookingPost post)
{
    struct UISeekPartyListWnd *window = (struct UISeekPartyListWnd *)ui_window_find(PARTY_BOOKING_WINDOW);
    if (!window) {
        if (self->alarm_filtered && !party_booking_filter_matches(self, post))
            return;
        if (self->alarm) {
            sound_play_wave("party_alarm.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
            ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)msg_str(MSG_PARTY_BOOKING_NEW), 0xffff00, 0xe, 0);
        }
        return;
    }
    if (!party_booking_wnd_matches(window, post))
        return;
    if (self->queue_new) {
        struct PartySearchRow row = {0};
        if (party_booking_make_row(self, &row, post))
            party_search_rows_insert(&self->pending, post.id, &row);
        party_search_row_clear(&row);
    } else {
        party_booking_add_rows(self, &post, 1, 0);
    }
    party_booking_wnd_alarm(window);
}

void party_booking_remove(struct PartyBooking *self, int id)
{
    party_search_rows_remove(&self->rows, id);
    party_search_rows_remove(&self->pending, id);
}

void party_booking_remove_row(struct PartyBooking *self, int id)
{
    party_search_rows_remove(&self->rows, id);
}

void party_booking_set_jobs(struct PartyBooking *self, int id, const short *jobs)
{
    struct PartySearchEntry *entry = party_search_rows_find(&self->rows, id);
    if (!entry)
        entry = party_search_rows_find(&self->pending, id);
    if (entry)
        format_jobs(&entry->row.jobs, jobs);
}

void party_booking_load_regions(struct PartyBooking *self)
{
    party_search_read_regions(g_session1.luaState, 2, &self->region_names, &self->region_maps);
}
