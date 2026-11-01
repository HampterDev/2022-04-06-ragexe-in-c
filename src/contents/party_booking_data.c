#include "contents/party_booking_data.h"
#include <stdlib.h>
#include <string.h>

void party_booking_init(struct AdvenPartyInfo *booking)
{
    memset(booking, 0, sizeof *booking);
    booking->max_level = 999;
}

void party_booking_clear(struct AdvenPartyInfo *booking)
{
    owned_text_clear(&booking->memo);
    owned_text_clear(&booking->name);
    party_booking_init(booking);
}

int party_booking_assign(struct AdvenPartyInfo *destination, const struct AdvenPartyInfo *source)
{
    struct AdvenPartyInfo copy = *source;
    if (destination == source)
        return 1;
    owned_text_init(&copy.memo);
    owned_text_init(&copy.name);
    if (!owned_text_assign(&copy.memo, owned_text_view(&source->memo)) ||
        !owned_text_assign(&copy.name, owned_text_view(&source->name))) {
        party_booking_clear(&copy);
        return 0;
    }
    party_booking_clear(destination);
    *destination = copy;
    return 1;
}

void party_booking_list_clear(struct PartyBookingList *list)
{
    size_t i;
    for (i = 0; i < list->count; ++i)
        party_booking_clear(&list->items[i]);
    free(list->items);
    list->items = NULL;
    list->count = 0;
}
