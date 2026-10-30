#include "web/party_booking_json.h"
#include "json/service_json.h"
#include <stdlib.h>
#include <string.h>

static const struct JsonTree *field(const struct JsonTree *value, const char *name)
{
    return value ? json_tree_find(value, name, strlen(name)) : NULL;
}

static int integer(const struct JsonTree *value, const char *name, int fallback)
{
    const struct JsonTree *member = field(value, name);
    return member ? json_value_as_int(member) : fallback;
}

int party_booking_read(struct AdvenPartyInfo *booking, const struct JsonTree *value)
{
    struct AdvenPartyInfo replacement;
    party_booking_init(&replacement);
    replacement.aid = (int)(unsigned)json_value_as_double(field(value, "AID"));
    replacement.gid = (int)(unsigned)json_value_as_double(field(value, "GID"));
    replacement.roles[0] = integer(value, "Tanker", 1) != 0;
    replacement.roles[1] = integer(value, "Healer", 1) != 0;
    replacement.roles[2] = integer(value, "Dealer", 1) != 0;
    replacement.roles[3] = integer(value, "Assist", 1) != 0;
    replacement.min_level = integer(value, "MinLV", 0);
    replacement.max_level = integer(value, "MaxLV", 999);
    replacement.purpose = integer(value, "Type", 1);
    if (!owned_text_assign(&replacement.name, json_value_text(field(value, "CharName"))) ||
        !owned_text_assign(&replacement.memo, json_value_text(field(value, "Memo")))) {
        party_booking_clear(&replacement);
        return 0;
    }
    party_booking_clear(booking);
    *booking = replacement;
    return 1;
}

int party_booking_list_read(struct PartyBookingList *list, struct JsonTree *value)
{
    struct PartyBookingList replacement = {0};
    size_t i, count = json_value_size(value);
    if (count > (size_t)-1 / sizeof *replacement.items)
        return 0;
    if (count) {
        replacement.items = calloc(count, sizeof *replacement.items);
        if (!replacement.items)
            return 0;
    }
    for (i = 0; i < count; ++i) {
        party_booking_init(&replacement.items[i]);
        ++replacement.count;
        if (!party_booking_read(&replacement.items[i], json_value_child(value, i))) {
            party_booking_list_clear(&replacement);
            return 0;
        }
    }
    party_booking_list_clear(list);
    *list = replacement;
    return 1;
}
