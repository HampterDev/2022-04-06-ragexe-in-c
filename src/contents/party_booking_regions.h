#ifndef CONTENTS_PARTY_BOOKING_REGIONS_H
#define CONTENTS_PARTY_BOOKING_REGIONS_H

#include "core/text_array.h"
#include "contents/party_booking_storage.h"

struct ClientLua;
int party_search_read_regions(struct ClientLua *lua, int first,
                               struct TextArray *names, struct PartySearchRegions *regions);

#endif
