#ifndef CONTENTS_PARTY_BOOKING_H
#define CONTENTS_PARTY_BOOKING_H

#include "contents/party_booking_types.h"

#define PARTY_BOOKING_JOBS 6

struct PartyBookingPost {
    int id;
    char leader[24];
    int posted_at;
    short level;
    short map;
    short jobs[PARTY_BOOKING_JOBS];
};
OFFSET_ASSERT(PartyBookingPost, posted_at, 0x1c);
OFFSET_ASSERT(PartyBookingPost, jobs, 0x24);
SIZE_ASSERT(PartyBookingPost, 0x30);

struct PartyBooking *party_booking_construct(struct PartyBooking *self);
void party_booking_destruct(struct PartyBooking *self);
void party_booking_reset(struct PartyBooking *self);
void party_booking_clear_rows(struct PartyBooking *self);
void party_booking_load_regions(struct PartyBooking *self);
void party_booking_set_filter(struct PartyBooking *self, int level, int map, int job);
char party_booking_filter_matches(struct PartyBooking *self, struct PartyBookingPost post);
int party_booking_make_row(struct PartyBooking *self, struct PartySearchRow *out,
                           struct PartyBookingPost post);
void party_booking_add_rows(struct PartyBooking *self, const struct PartyBookingPost *posts,
                            int count, char queue_new);
struct PartyBookingSearch {
    const unsigned char *posts;
    size_t count;
    char more;
};
int party_booking_decode_search(struct PartyBookingSearch *out, const unsigned char *packet, size_t size);
void party_booking_add_search(struct PartyBooking *self, const struct PartyBookingSearch *search);
char party_booking_take_pending(struct PartyBooking *self);
void party_booking_on_new(struct PartyBooking *self, struct PartyBookingPost post);
void party_booking_remove(struct PartyBooking *self, int id);
void party_booking_remove_row(struct PartyBooking *self, int id);
void party_booking_set_jobs(struct PartyBooking *self, int id, const short *jobs);

struct UISeekPartyListWnd;
char party_booking_wnd_matches(struct UISeekPartyListWnd *window, struct PartyBookingPost post);
void party_booking_wnd_alarm(struct UISeekPartyListWnd *window);

#endif
