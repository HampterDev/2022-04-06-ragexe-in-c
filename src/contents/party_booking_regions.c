#include "contents/party_booking_regions.h"
#include "contents/client_lua_calls.h"

int party_search_read_regions(struct ClientLua *lua, int first,
                               struct TextArray *names, struct PartySearchRegions *regions)
{
    struct TextArray new_names = {0};
    struct PartySearchRegions new_regions = {0};
    struct PartySearchMaps maps = {0};
    int region, end, map;

    if (first < 1 || first > 256)
        return 0;
    for (region = first; region <= 257; ++region) {
        int unused = 0;
        const char *name = NULL;
        if (!client_lua_call(lua, "queryRegionInfo", "d>ds", region, &unused, &name))
            continue;
        if (!name)
            break;
        if (region > 256 || !text_array_resize(&new_names, new_names.size + 1) ||
            !owned_text_assign(&new_names.items[new_names.size - 1], text_view_cstr(name)))
            goto fail;
    }
    if (region > 257)
        goto fail;
    end = region;
    for (region = first; region < end; ++region) {
        int unused_a = 0, unused_b = 0, c0 = 0, c1 = 0, c2 = 0;
        const char *name = NULL;
        for (map = 1; map <= 257; ++map) {
            unsigned color;
            if (!client_lua_call(lua, "queryMapInfo", "dd>ddsddd", region, map,
                                 &unused_a, &unused_b, &name, &c0, &c1, &c2))
                continue;
            if (!name)
                break;
            color = (unsigned char)c0 | (unsigned char)c1 << 8 | (unsigned char)c2 << 16;
            if (map > 256 || !party_search_maps_append(&maps, text_view_cstr(name), color))
                goto fail;
        }
        if (map > 257 || !party_search_regions_insert(&new_regions, region, &maps))
            goto fail;
        party_search_maps_clear(&maps);
    }
    text_array_clear(names);
    party_search_regions_clear(regions);
    *names = new_names;
    *regions = new_regions;
    return 1;

fail:
    party_search_maps_clear(&maps);
    party_search_regions_clear(&new_regions);
    text_array_clear(&new_names);
    return 0;
}
