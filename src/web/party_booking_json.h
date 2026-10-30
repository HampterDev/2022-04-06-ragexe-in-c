#ifndef ASYNCWORK_PARTY_BOOKING_JSON_H
#define ASYNCWORK_PARTY_BOOKING_JSON_H

#include "web/party_booking_data.h"

struct JsonTree;

int party_booking_read(struct AdvenPartyInfo *booking, const struct JsonTree *value);
int party_booking_list_read(struct PartyBookingList *list, struct JsonTree *value);

#endif
