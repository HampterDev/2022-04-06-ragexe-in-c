#include "world/move_track.h"
#include "body/grid_path.h"

#define RECORD_HEAD 25

static unsigned get16(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static uint32_t get32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int move_record_read(const unsigned char *packet, size_t size, uint32_t *gid, struct MoveRecord *record)
{
    unsigned steps, i;

    if (size < RECORD_HEAD || get16(packet + 2) != size)
        return 0;
    steps = packet[22];
    if (steps > MOVE_TRACK_STEPS || size < RECORD_HEAD + (3 * steps + 7) / 8
        || (packet[21] & 0x7f) > MOVE_CAUSE_PLACED)
        return 0;
    *gid = get32(packet + 4);
    record->at_tick = get32(packet + 8);
    record->x = (int)get16(packet + 12);
    record->y = (int)get16(packet + 14);
    record->land_ms = get16(packet + 16);
    record->speed_ms = get16(packet + 18);
    record->dir = packet[20];
    record->cause = (unsigned char)(packet[21] & 0x7f);
    record->old = (unsigned char)(packet[21] >> 7);
    record->steps = (unsigned char)steps;
    record->rev = (uint16_t)get16(packet + 23);
    for (i = 0; i < steps; i++) {
        unsigned bit = 3 * i;
        /* the last step's bits may end in the packet's last byte */
        unsigned two = packet[RECORD_HEAD + bit / 8];

        if (bit % 8 > 5)
            two |= (unsigned)packet[RECORD_HEAD + bit / 8 + 1] << 8;
        record->path[i] = (unsigned char)(two >> bit % 8 & 7);
    }
    return 1;
}

int move_record_cells(const struct MoveRecord *record, struct MoveCell *out)
{
    /* the server's direction codes: 0 north (y + 1), then counter-clockwise */
    static const signed char dx[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
    static const signed char dy[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    int i;

    out[0].x = record->x;
    out[0].y = record->y;
    out[0].dir = record->dir;
    out[0].tick = record->at_tick;
    if (!record->steps)
        return 1;
    out[0].tick += record->land_ms - grid_path_step_ms((int)record->speed_ms, record->path[0] & 1);
    for (i = 0; i < record->steps; i++) {
        int dir = record->path[i];

        out[i + 1].x = out[i].x + dx[dir];
        out[i + 1].y = out[i].y + dy[dir];
        out[i + 1].dir = dir;
        out[i + 1].tick = i ? out[i].tick + grid_path_step_ms((int)record->speed_ms, dir & 1)
                            : record->at_tick + record->land_ms;
    }
    return record->steps + 1;
}

int move_record_stand(const struct MoveRecord *record, const struct MoveStandAsk *ask)
{
    int dx = record->x - ask->drawn_x, dy = record->y - ask->drawn_y;

    if (record->cause != MOVE_CAUSE_WALKED)
        return MOVE_STAND_SET;
    if (!dx && !dy)
        return !ask->walking ? MOVE_STAND_LEAVE : ask->held ? MOVE_STAND_HALT : MOVE_STAND_FINISH;
    if (dx > ask->jump_cells || dx < -ask->jump_cells || dy > ask->jump_cells || dy < -ask->jump_cells)
        return MOVE_STAND_SET;
    return MOVE_STAND_WALK;
}

int move_track_file(struct MoveTrack *track, const struct RecordFloors *floors, const struct MoveRecord *record)
{
    int i, kept = 0, at;

    if (floors->has_move && !record_rev_newer(record->rev, floors->move))
        return 0;
    for (i = 0; i < track->count; i++) {
        const struct MoveRecord *held = &track->held[i];

        if (held->rev == record->rev
            || (record_rev_newer(held->rev, record->rev) && (int32_t)(held->at_tick - record->at_tick) <= 0))
            return 0;
    }
    /* what is left is newer and later, or older and earlier: it stays in order by tick */
    for (i = 0; i < track->count; i++)
        if (record_rev_newer(track->held[i].rev, record->rev) || (int32_t)(track->held[i].at_tick - record->at_tick) < 0)
            track->held[kept++] = track->held[i];
    track->count = kept;
    if (track->count == MOVE_TRACK_HELD) {
        for (i = 1; i < MOVE_TRACK_HELD; i++)
            track->held[i - 1] = track->held[i];
        track->count--;
    }
    for (at = track->count; at > 0 && (int32_t)(track->held[at - 1].at_tick - record->at_tick) > 0; at--)
        track->held[at] = track->held[at - 1];
    track->held[at] = *record;
    track->count++;
    return 1;
}

void move_track_drop_after(struct MoveTrack *track, uint32_t now)
{
    while (track->count && (int32_t)(track->held[track->count - 1].at_tick - now) > 0)
        track->count--;
}

int move_track_take(struct MoveTrack *track, struct RecordFloors *floors, uint32_t now, struct MoveRecord *record)
{
    int due = 0, i;

    while (due < track->count && (int32_t)(now - track->held[due].at_tick) >= 0)
        due++;
    if (!due)
        return 0;
    *record = track->held[due - 1];
    if (!floors->has_move || record_rev_newer(record->rev, floors->move)) {
        floors->move = record->rev;
        floors->has_move = 1;
    }
    for (i = due; i < track->count; i++)
        track->held[i - due] = track->held[i];
    track->count -= due;
    return 1;
}

int move_track_will_move(const struct MoveTrack *track, int x, int y, uint32_t until)
{
    struct MoveStandAsk standing;
    int i;

    standing.drawn_x = x;
    standing.drawn_y = y;
    standing.walking = 0;
    standing.held = 0;
    standing.jump_cells = 0;
    for (i = 0; i < track->count && (int32_t)(track->held[i].at_tick - until) <= 0; i++)
        if (track->held[i].steps || move_record_stand(&track->held[i], &standing) != MOVE_STAND_LEAVE)
            return 1;
    return 0;
}
