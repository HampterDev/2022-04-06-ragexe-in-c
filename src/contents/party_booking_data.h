#ifndef PARTY_BOOKING_DATA_H
#define PARTY_BOOKING_DATA_H

#include "core/owned_text.h"

struct AdvenPartyInfo {
    unsigned char roles[4];
    int min_level, max_level, purpose;
    struct OwnedText memo;
    int aid, gid;
    struct OwnedText name;
};

void party_booking_init(struct AdvenPartyInfo *booking);
void party_booking_clear(struct AdvenPartyInfo *booking);
int party_booking_assign(struct AdvenPartyInfo *destination, const struct AdvenPartyInfo *source);

struct PartyBookingList {
    struct AdvenPartyInfo *items;
    size_t count;
};
void party_booking_list_clear(struct PartyBookingList *list);

#endif
