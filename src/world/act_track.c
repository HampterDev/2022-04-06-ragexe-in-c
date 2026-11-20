#include "world/act_track.h"

static unsigned get16(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static uint32_t get32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int act_record_read(const unsigned char *packet, struct ActRecord *record)
{
    if ((packet[10] & ~ACT_RECORD_OLD) > ACT_CAST)
        return 0;
    record->gid = get32(packet + 2);
    record->at_tick = get32(packet + 6);
    record->kind = (unsigned char)(packet[10] & ~ACT_RECORD_OLD);
    record->old = (unsigned char)(packet[10] >> 7);
    record->end_tick = get32(packet + 11);
    record->mark_tick = get32(packet + 15);
    record->target = get32(packet + 19);
    record->x = (int)get16(packet + 23);
    record->y = (int)get16(packet + 25);
    record->skill_id = get16(packet + 27);
    record->skill_lv = get16(packet + 29);
    record->motion_ms = get16(packet + 31);
    record->element = (signed char)packet[33];
    record->disposable = (unsigned char)(packet[34] != 0);
    record->rev = (uint16_t)get16(packet + 35);
    return 1;
}

void vitals_record_read(const unsigned char *packet, struct VitalsRecord *record)
{
    record->gid = get32(packet + 2);
    record->at_tick = get32(packet + 6);
    record->hp = (int32_t)get32(packet + 10);
    record->max_hp = (int32_t)get32(packet + 14);
    record->rev = (uint16_t)get16(packet + 18);
}

void hit_notice_read(const unsigned char *packet, struct HitNotice *notice)
{
    notice->source = get32(packet + 2);
    notice->target = get32(packet + 6);
    notice->at_tick = get32(packet + 10);
    notice->damage = (int32_t)get32(packet + 14);
    notice->left_damage = (int32_t)get32(packet + 18);
    notice->count = (int16_t)get16(packet + 22);
    notice->kind = packet[24];
    notice->sp_damage = (unsigned char)(packet[25] != 0);
    notice->flinch_ms = get16(packet + 26);
}

static void remove_at(struct TimedQueue *queue, int at)
{
    int i;

    for (i = at + 1; i < queue->count; i++)
        queue->items[i - 1] = queue->items[i];
    queue->count--;
}

static uint16_t rev_of(const struct TimedItem *item)
{
    return item->kind == TIMED_ACT ? item->u.act.rev : item->u.vitals.rev;
}

/* Whether a record of `item`'s track is played already that `item` is not newer than. */
static int is_past(const struct TimedItem *item, const struct RecordFloors *floors)
{
    if (item->kind == TIMED_ACT)
        return floors->has_act && !record_rev_newer(item->u.act.rev, floors->act);
    return floors->has_vitals && !record_rev_newer(item->u.vitals.rev, floors->vitals);
}

void timed_queue_clear(struct TimedQueue *queue)
{
    queue->count = 0;
    queue->aimed_count = 0;
}

/* Where `actor` is among those in the middle of something aimed at the own character, or -1. */
static int aimed_index(const struct TimedQueue *queue, uint32_t actor)
{
    int i;

    for (i = 0; i < queue->aimed_count; i++)
        if (queue->aimed[i] == actor)
            return i;
    return -1;
}

/* Whether an act record or a hit is aimed at `own`. A cast on the ground has no target. */
static int aims_at(const struct TimedItem *item, uint32_t own)
{
    if (item->kind == TIMED_HIT)
        return item->u.hit.target == own;
    return item->kind == TIMED_ACT && item->u.act.kind != ACT_IDLE && item->u.act.target == own;
}

int timed_queue_on_own_clock(const struct TimedQueue *queue, const struct TimedItem *item, uint32_t own)
{
    if (own == 0)
        return 0;
    if (item->actor == own || aims_at(item, own))
        return 1;
    return item->kind == TIMED_ACT && item->u.act.kind == ACT_IDLE && aimed_index(queue, item->actor) >= 0;
}

/* An item left the queue to be played. An act record of another actor says what that actor is
   in the middle of from now on: something aimed at the own character, whose end is then told
   on the own clock too, or not. */
static void taken(struct TimedQueue *queue, const struct TimedItem *item, uint32_t own)
{
    int at, i;

    if (item->kind != TIMED_ACT || item->actor == own || own == 0)
        return;
    at = aimed_index(queue, item->actor);
    if (at >= 0) {
        for (i = at + 1; i < queue->aimed_count; i++)
            queue->aimed[i - 1] = queue->aimed[i];
        queue->aimed_count--;
    }
    if (!aims_at(item, own))
        return;
    if (queue->aimed_count == TIMED_AIMED_ACTORS) {
        for (i = 1; i < queue->aimed_count; i++)
            queue->aimed[i - 1] = queue->aimed[i];
        queue->aimed_count--;
    }
    queue->aimed[queue->aimed_count++] = item->actor;
}

int timed_queue_file(struct TimedQueue *queue, const struct TimedItem *item, const struct RecordFloors *floors,
                     uint32_t own, struct TimedItem *early)
{
    int pushed = 0, at, i;

    if (item->kind == TIMED_ACT || item->kind == TIMED_VITALS) {
        uint16_t rev = rev_of(item);

        if (is_past(item, floors))
            return TIMED_DROPPED;
        for (i = 0; i < queue->count; i++) {
            const struct TimedItem *held = &queue->items[i];

            if (held->kind == item->kind && held->actor == item->actor
                && record_against(held->tick, rev_of(held), item->tick, rev) == RECORD_REFUSED)
                return TIMED_DROPPED;
        }
        for (i = queue->count - 1; i >= 0; i--) {
            const struct TimedItem *held = &queue->items[i];

            if (held->kind == item->kind && held->actor == item->actor
                && record_against(held->tick, rev_of(held), item->tick, rev) == RECORD_REPLACES)
                remove_at(queue, i);
        }
    }
    if (queue->count == TIMED_QUEUE_ITEMS) {
        *early = queue->items[0];
        remove_at(queue, 0);
        taken(queue, early, own);
        pushed = 1;
    }
    at = queue->count;
    while (at > 0 && (int32_t)(queue->items[at - 1].tick - item->tick) > 0) {
        queue->items[at] = queue->items[at - 1];
        at--;
    }
    queue->items[at] = *item;
    queue->count++;
    return pushed ? TIMED_FILED_EARLY : TIMED_FILED;
}

void record_floors_played(struct RecordFloors *floors, const struct TimedItem *item)
{
    if (item->kind == TIMED_ACT && (!floors->has_act || record_rev_newer(item->u.act.rev, floors->act))) {
        floors->act = item->u.act.rev;
        floors->has_act = 1;
    } else if (item->kind == TIMED_VITALS && (!floors->has_vitals || record_rev_newer(item->u.vitals.rev, floors->vitals))) {
        floors->vitals = item->u.vitals.rev;
        floors->has_vitals = 1;
    }
}

/* Whether a later act or hit of the actor of the item at `at` is due on the own clock: what the
   actor did before it is played first. */
static int brought_along(const struct TimedQueue *queue, int at, uint32_t own, uint32_t own_now)
{
    const struct TimedItem *held = &queue->items[at];
    int i;

    if (held->kind == TIMED_VITALS)
        return 0;
    for (i = at + 1; i < queue->count; i++) {
        const struct TimedItem *later = &queue->items[i];

        if ((int32_t)(own_now - later->tick) < 0)
            break;
        if (later->kind != TIMED_VITALS && later->actor == held->actor && timed_queue_on_own_clock(queue, later, own))
            return 1;
    }
    return 0;
}

int timed_queue_take(struct TimedQueue *queue, uint32_t own, uint32_t own_now, uint32_t render_now,
                     struct TimedItem *item, uint32_t *now)
{
    int i;

    for (i = 0; i < queue->count; i++) {
        int on_own = timed_queue_on_own_clock(queue, &queue->items[i], own);

        if ((int32_t)((on_own ? own_now : render_now) - queue->items[i].tick) < 0) {
            if (on_own || !brought_along(queue, i, own, own_now))
                continue;
            on_own = 1;
        }
        *item = queue->items[i];
        *now = on_own ? own_now : render_now;
        remove_at(queue, i);
        taken(queue, item, own);
        return 1;
    }
    return 0;
}

int timed_queue_take_vitals(struct TimedQueue *queue, uint32_t gid, uint32_t tick, struct TimedItem *item)
{
    int i;

    for (i = 0; i < queue->count; i++) {
        const struct TimedItem *held = &queue->items[i];

        if ((int32_t)(held->tick - tick) > 0)
            break;
        if (held->kind == TIMED_VITALS && held->actor == gid) {
            *item = *held;
            remove_at(queue, i);
            return 1;
        }
    }
    return 0;
}

int timed_queue_take_actor(struct TimedQueue *queue, uint32_t actor, struct TimedItem *item)
{
    int i, at = aimed_index(queue, actor);

    if (at >= 0) {
        for (i = at + 1; i < queue->aimed_count; i++)
            queue->aimed[i - 1] = queue->aimed[i];
        queue->aimed_count--;
    }
    for (i = 0; i < queue->count; i++)
        if (queue->items[i].actor == actor) {
            *item = queue->items[i];
            remove_at(queue, i);
            return 1;
        }
    return 0;
}
