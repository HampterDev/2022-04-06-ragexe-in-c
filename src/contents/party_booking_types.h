#ifndef SRC_CONTENTS_PARTY_BOOKING_TYPES_H
#define SRC_CONTENTS_PARTY_BOOKING_TYPES_H

#include "base.h"
#include "core/text_array.h"
#include "contents/party_booking_storage.h"

struct PartyBooking {
    int window_open;
    char alarm;
    char alarm_filtered;
    struct TextArray region_names;
    struct PartySearchRegions region_maps;
    struct PartySearchRows rows;
    struct PartySearchRows pending;
    char queue_new;
    int last_id;
    int answer_tick;
    int filter_level;
    int filter_map;
    int filter_job;
};
OFFSET_ASSERT(PartyBooking, region_maps, 0x14);
OFFSET_ASSERT(PartyBooking, rows, 0x1c);
OFFSET_ASSERT(PartyBooking, pending, 0x24);
OFFSET_ASSERT(PartyBooking, queue_new, 0x2c);
OFFSET_ASSERT(PartyBooking, filter_level, 0x38);
OFFSET_ASSERT(PartyBooking, filter_job, 0x40);

#endif
