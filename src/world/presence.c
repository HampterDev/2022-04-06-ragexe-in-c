#include "world/presence.h"

/* Whether the unit is in view and has an actor to file on. */
static int present(const struct Presence *presence, uint32_t gid)
{
    return view_floors_in_view(presence->view, gid) && presence->track_of(presence->context, gid) != 0;
}

static int present_of(void *context, uint32_t gid)
{
    return present((const struct Presence *)context, gid);
}

/* Whether a record of revision `rev` is not newer than the floor of its track. */
static int is_past(int has, uint16_t floor, uint16_t rev)
{
    return has && !record_rev_newer(rev, floor);
}

static void put_waiting(const struct Presence *presence, uint32_t now, uint32_t gid, const struct MoveRecord *move,
                        const struct TimedItem *timed)
{
    struct RecordWaiting waiting;

    waiting.gid = gid;
    if (move) {
        waiting.kind = RECORD_WAIT_MOVE;
        waiting.u.move = *move;
    } else {
        waiting.kind = RECORD_WAIT_TIMED;
        waiting.u.timed = *timed;
    }
    record_wait_put(presence->wait, now, &waiting);
}

int presence_file_move(const struct Presence *presence, uint32_t now, uint32_t gid, const struct MoveRecord *record)
{
    const struct ViewUnit *known = view_floors_find(presence->view, gid);
    struct ViewUnit *unit;

    if (known && is_past(known->floors.has_move, known->floors.move, record->rev))
        return 0;
    if (!present(presence, gid)) {
        put_waiting(presence, now, gid, record, 0);
        return 0;
    }
    unit = view_floors_unit(presence->view, gid);
    return move_track_file(presence->track_of(presence->context, gid), &unit->floors, record);
}

int presence_file_timed(const struct Presence *presence, uint32_t now, const struct TimedItem *item,
                        struct TimedItem *early)
{
    const struct ViewUnit *known = view_floors_find(presence->view, item->actor);
    struct ViewUnit *unit;

    if (item->kind == TIMED_HIT) {
        if (view_floors_past_notice(presence->view, item->actor, item->tick))
            return TIMED_DROPPED;
        return timed_queue_file(presence->queue, item, 0, presence->own, early);
    }
    if (known && (item->kind == TIMED_ACT ? is_past(known->floors.has_act, known->floors.act, item->u.act.rev)
                                          : is_past(known->floors.has_vitals, known->floors.vitals, item->u.vitals.rev)))
        return TIMED_DROPPED;
    if (!present(presence, item->actor)) {
        put_waiting(presence, now, item->actor, 0, item);
        return TIMED_DROPPED;
    }
    unit = view_floors_unit(presence->view, item->actor);
    return timed_queue_file(presence->queue, item, &unit->floors, presence->own, early);
}

int presence_stamp(const struct Presence *presence, uint32_t now, const struct ViewStamp *stamp)
{
    struct MoveTrack *track = presence->track_of(presence->context, stamp->gid);
    struct MoveRecord held[MOVE_TRACK_HELD];
    struct TimedItem items[TIMED_QUEUE_ITEMS], early;
    int moves = 0, count = 0, i;

    if (stamp->gid == presence->own || view_floors_unit(presence->view, stamp->gid) == 0)
        return 0;
    /* what the unit holds leaves it, and is filed again against the floors the stamp sets */
    if (stamp->kind == VIEW_GOES && track) {
        for (i = 0; i < track->count; i++)
            held[moves++] = track->held[i];
        track->count = 0;
    }
    while (count < TIMED_QUEUE_ITEMS && timed_queue_take_actor(presence->queue, stamp->gid, &items[count]))
        count++;
    view_floors_stamp(presence->view, stamp);
    for (i = 0; i < moves; i++)
        presence_file_move(presence, now, stamp->gid, &held[i]);
    for (i = 0; i < count; i++)
        presence_file_timed(presence, now, &items[i], &early);
    return 1;
}

int presence_file_waiting(const struct Presence *presence, uint32_t now, struct TimedItem *early, int *filed)
{
    struct RecordWaiting waiting;

    *filed = TIMED_DROPPED;
    if (!record_wait_take(presence->wait, now, present_of, (void *)presence, &waiting))
        return 0;
    if (waiting.kind == RECORD_WAIT_MOVE)
        presence_file_move(presence, now, waiting.gid, &waiting.u.move);
    else
        *filed = presence_file_timed(presence, now, &waiting.u.timed, early);
    return 1;
}
