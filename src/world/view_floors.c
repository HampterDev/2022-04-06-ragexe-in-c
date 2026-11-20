#include "world/view_floors.h"

static uint32_t get16(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static uint32_t get32(const unsigned char *p)
{
    return get16(p) | get16(p + 2) << 16;
}

int view_stamp_read(const unsigned char *packet, unsigned size, struct ViewStamp *stamp)
{
    if (size < VIEW_STAMP_SIZE || (packet[10] != VIEW_COMES && packet[10] != VIEW_GOES))
        return 0;
    stamp->gid = get32(packet + 2);
    stamp->tick = get32(packet + 6);
    stamp->kind = packet[10];
    stamp->rev_move = (uint16_t)get16(packet + 11);
    stamp->rev_act = (uint16_t)get16(packet + 13);
    stamp->rev_vitals = (uint16_t)get16(packet + 15);
    return 1;
}

int view_stamp_stamps(const struct ViewStamp *stamp, int packet_id, const unsigned char *packet, unsigned size)
{
    if (packet_id == 0x0080 && size >= 6)
        return stamp->kind == VIEW_GOES && get32(packet + 2) == stamp->gid;
    if ((packet_id == 0x09fd || packet_id == 0x09fe || packet_id == 0x09ff) && size >= 9)
        return stamp->kind == VIEW_COMES && get32(packet + 5) == stamp->gid;
    return 0;
}

int record_rev_newer(uint16_t a, uint16_t b)
{
    return (int16_t)(uint16_t)(a - b) > 0;
}

int record_against(uint32_t held_tick, uint16_t held_rev, uint32_t tick, uint16_t rev)
{
    if (held_rev == rev || (record_rev_newer(held_rev, rev) && (int32_t)(held_tick - tick) <= 0))
        return RECORD_REFUSED;
    if (record_rev_newer(rev, held_rev) && (int32_t)(held_tick - tick) >= 0)
        return RECORD_REPLACES;
    return RECORD_BESIDE;
}

void view_floors_clear(struct ViewFloors *view)
{
    view->count = 0;
    view->clock = 0;
}

static int index_of(const struct ViewFloors *view, uint32_t gid)
{
    int i;

    for (i = 0; i < view->count; i++)
        if (view->units[i].gid == gid)
            return i;
    return -1;
}

const struct ViewUnit *view_floors_find(const struct ViewFloors *view, uint32_t gid)
{
    int at = gid ? index_of(view, gid) : -1;

    return at >= 0 ? &view->units[at] : 0;
}

struct ViewUnit *view_floors_unit(struct ViewFloors *view, uint32_t gid)
{
    struct ViewUnit *unit;
    unsigned char *byte;
    unsigned i;
    int at, k;

    if (gid == 0)
        return 0;
    at = index_of(view, gid);
    if (at < 0) {
        if (view->count < VIEW_FLOORS_UNITS) {
            at = view->count++;
        } else {
            for (k = 0; k < view->count; k++)
                if (!view->units[k].in_view && !view->units[k].pinned
                    && (at < 0 || (int32_t)(view->units[k].used - view->units[at].used) < 0))
                    at = k;
            if (at < 0)
                return 0;
        }
        unit = &view->units[at];
        byte = (unsigned char *)unit;
        for (i = 0; i < sizeof *unit; i++)
            byte[i] = 0;
        unit->gid = gid;
    }
    unit = &view->units[at];
    unit->used = ++view->clock;
    return unit;
}

int view_floors_in_view(const struct ViewFloors *view, uint32_t gid)
{
    const struct ViewUnit *unit = view_floors_find(view, gid);

    return unit && (unit->in_view || unit->pinned);
}

void view_floors_pin(struct ViewFloors *view, uint32_t gid)
{
    struct ViewUnit *unit = view_floors_unit(view, gid);

    if (unit)
        unit->pinned = 1;
}

/* The floor becomes `rev` unless it is newer already. */
static void raise_floor(uint16_t *floor, unsigned char *has, uint16_t rev)
{
    if (!*has || record_rev_newer(rev, *floor)) {
        *floor = rev;
        *has = 1;
    }
}

const struct ViewUnit *view_floors_stamp(struct ViewFloors *view, const struct ViewStamp *stamp)
{
    struct ViewUnit *unit = view_floors_unit(view, stamp->gid);
    uint16_t less = stamp->kind == VIEW_COMES ? 1 : 0;

    if (!unit || unit->pinned)
        return 0;
    raise_floor(&unit->floors.move, &unit->floors.has_move, (uint16_t)(stamp->rev_move - less));
    raise_floor(&unit->floors.act, &unit->floors.has_act, (uint16_t)(stamp->rev_act - less));
    raise_floor(&unit->floors.vitals, &unit->floors.has_vitals, (uint16_t)(stamp->rev_vitals - less));
    if (stamp->kind == VIEW_COMES) {
        unit->in_view = 1;
        unit->came = stamp->tick;
        unit->has_came = 1;
    } else {
        unit->in_view = 0;
        unit->went = stamp->tick;
        unit->has_went = 1;
    }
    return unit;
}

int view_floors_past_notice(const struct ViewFloors *view, uint32_t gid, uint32_t tick)
{
    const struct ViewUnit *unit = view_floors_find(view, gid);

    if (!unit || unit->pinned)
        return 0;
    if (unit->has_came && (int32_t)(tick - unit->came) < 0)
        return 1;
    return !unit->in_view && unit->has_went && (int32_t)(tick - unit->went) <= 0;
}
