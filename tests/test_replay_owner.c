#include "replay/replay.h"
#include "platform/service.h"
#include "session/state.h"
#include "session/session_state.h"
#include "core/packet_table.h"
#include <mmsystem.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static DWORD now;
static int fail_write, fail_read, fail_seek, fail_allocate;
static unsigned blocks;
static DWORD test_time(void) { return now; }
static void *allocate(size_t size)
{
    void *p;
    if (fail_allocate) return NULL;
    p = malloc(size); assert(p); ++blocks; return p;
}
static void release(void *p)
{
    if (p) { assert(blocks); --blocks; free(p); }
}
static size_t write_bytes(const void *p, size_t size, size_t count, FILE *f)
{ return fail_write ? 0 : fwrite(p, size, count, f); }
static size_t read_bytes(void *p, size_t size, size_t count, FILE *f)
{ return fail_read ? 0 : fread(p, size, count, f); }
static int seek_file(FILE *f, long offset, int origin)
{ return fail_seek ? -1 : fseek(f, offset, origin); }

struct PacketTable g_packetLengthMap;
int g_serverType;
int packet_length_lookup(struct PacketTable *table, int id)
{ (void)table; (void)id; return 8; }
struct Session g_session1;
struct Vec *session_get_stored_packet(struct Session *self, struct Vec *out, short id)
{ (void)self; (void)id; out->first = out->last = out->end = NULL; return out; }
struct Vec *session_get_stored_packet2(struct Session *self, struct Vec *out)
{ return session_get_stored_packet(self, out, 0); }
void std_vector_bytes_assign_range(struct Vec *vector, const char *first, const char *last)
{
    size_t size = last - first;
    vector->first = malloc(size); assert(vector->first);
    memcpy(vector->first, first, size);
    vector->last = vector->end = (char *)vector->first + size;
}

#define timeGetTime test_time
#define malloc allocate
#define free release
#define fwrite write_bytes
#define fread read_bytes
#define fseek seek_file
#include "../src/replay/recorder.c"
#include "../src/replay/snapshot.c"
#include "../src/replay/replay_session.c"
#include "../src/replay/replay_file.c"
#undef timeGetTime
#undef malloc
#undef free
#undef fwrite
#undef fread
#undef fseek

static void states_and_ownership(void)
{
    struct ReplayRecorder rec;
    struct ReplayDuration duration;
    unsigned char bytes[65535], options[31];
    unsigned i;
    int mode, previous;
    replay_recorder_construct(&rec);
    memset(bytes, 0x65, sizeof bytes);
    memset(options, 0x74, sizeof options);
    for (mode = -2; mode <= 10; ++mode) for (previous = 0; previous <= 1; ++previous) {
        int expected = mode == 4 ? 1 : mode == 5 ? 6 : mode >= 1 && mode <= 8 ? mode : 1;
        int playing = mode >= 6 && mode <= 8 ? 1 : mode >= 1 && mode <= 3 ? 0 : previous;
        g_replayPlaying = previous;
        assert(replay_recorder_set_mode(&rec, mode));
        assert(rec.state == expected && g_replayPlaying == playing);
    }
    replay_recorder_set_mode(&rec, INT_MIN); assert(rec.state == 1);
    replay_recorder_set_mode(&rec, INT_MAX); assert(rec.state == 1);
    for (i = 0; i < sizeof rec.info.options; ++i) assert(rec.info.options[i] == 1);
    assert(!replay_recorder_set_options(&rec, options, sizeof options));
    assert(!replay_recorder_set_options(&rec, NULL, 1));
    assert(replay_recorder_set_options(&rec, NULL, 0));
    assert(replay_recorder_set_options(&rec, options, 5));
    assert(rec.info.options[4] == 0x74 && rec.info.options[5] == 1);
    replay_recorder_capture_packet(&rec, 0x99b, sizeof bytes, (const char *)bytes);
    assert(rec.storedPackets[0].size == sizeof bytes && !memcmp(rec.storedPackets[0].data, bytes, sizeof bytes));
    bytes[0] = 2; assert(rec.storedPackets[0].data[0] == 0x65);
    fail_allocate = 1;
    replay_recorder_capture_packet(&rec, 0x99b, 8, (const char *)bytes);
    assert(rec.storedPackets[0].size == sizeof bytes);
    fail_allocate = 0;
    replay_recorder_capture_packet(&rec, 0x99b, 8, (const char *)rec.storedPackets[0].data);
    replay_recorder_capture_packet(&rec, 0x1d6, 8, (const char *)bytes);
    replay_recorder_capture_packet(&rec, 0xffff, 8, (const char *)bytes);
    assert(blocks == 2 && rec.storedPackets[0].size == 8 && rec.storedPackets[1].size == 8);
    rec.file = tmpfile(); assert(rec.file); rec.openMode = REPLAY_OPEN_READ;
    replay_recorder_reset_session(&rec);
    assert(!rec.file && rec.openMode == REPLAY_OPEN_NONE && rec.info.options[4] == 0x74 && blocks == 2);
    g_replayPlaying = 1;
    rec.state = REPLAY_STATE_PLAYING; rec.info.totalTime = 1000; rec.lastTick = 0xfffffff0u;
    rec.elapsed = 0; now = 16;
    assert(replay_recorder_tick(&rec) && rec.elapsed == 32);
    rec.speed = 0.25f; now += 3; assert(replay_recorder_tick(&rec) && rec.elapsed == 32);
    rec.speed = 4; now += 300; assert(!replay_recorder_tick(&rec) && rec.elapsed == 1000 && rec.state == 6);
    rec.state = 7; rec.info.totalTime = MAXDWORD; rec.elapsed = 1; rec.lastTick = now + 1;
    assert(!replay_recorder_tick(&rec) && rec.elapsed == INT_MAX);
    rec.info.totalTime = 3661000;
    duration = replay_recorder_duration(&rec);
    assert(duration.hours == 1 && duration.minutes == 1 && duration.seconds == 1);
    for (mode = 3; mode <= 8; mode += 5) {
        rec.state = mode; rec.elapsed = 41; rec.recordIndex = 9; rec.wallBase = 22;
        replay_recorder_reset_clock(&rec); replay_recorder_resume(&rec);
        assert(rec.elapsed == 41 && rec.recordIndex == 9 && rec.wallBase == 22);
    }
    replay_recorder_destruct(&rec); replay_recorder_destruct(&rec);
    assert(!blocks);
}

static void write_fixture(const char *path, const void *data, size_t size)
{
    FILE *file = fopen(path, "wb"); assert(file);
    assert(fwrite(data, 1, size, file) == size); assert(!fclose(file));
}

static void file_roundtrip(void)
{
    struct ReplayRecorder rec, playback;
    struct ReplayBuffer chunk = {0};
    struct ReplayChunkEntry entry;
    struct ReplaySessionInfo saved;
    unsigned char packet[8] = {0xae, 8, 3, 4, 5, 6, 7, 8};
    unsigned char data[13] = "sample bytes", fixture[2048], malformed[2048];
    char directory[MAX_PATH], path[MAX_PATH];
    size_t size, i;
    DWORD offset;
    FILE *file;
    assert(GetTempPathA(sizeof directory, directory));
    assert(GetTempFileNameA(directory, "rrf", 0, path));
    replay_recorder_construct(&rec); replay_recorder_construct(&playback);
    strcpy(rec.info.name, "Player"); strcpy((char *)rec.info.mapData, "prontera");
    rec.info.cipherKeyA = 0x12345678; rec.info.cipherKeyB = 0x98765432;
    assert(replay_recorder_open(&rec, path, REPLAY_OPEN_WRITE));
    assert(replay_recorder_write_chunk(&rec, 3, sizeof data, data, REPLAY_CHUNK_APPEND));
    assert(!memcmp(data, "sample bytes", sizeof data));
    entry = rec.chunks[3]; offset = rec.fileOffset;
    assert(!replay_recorder_write_chunk(&rec, 3, sizeof data - 1, data, REPLAY_CHUNK_REWRITE));
    assert(!memcmp(&entry, &rec.chunks[3], sizeof entry) && rec.fileOffset == offset);
    assert(replay_recorder_on_event(&rec, REPLAY_EVENT_START));
    assert(replay_recorder_mark_first_record(&rec));
    now = 50; rec.wallBase = 0;
    assert(replay_recorder_write_packet(&rec, sizeof packet, packet, REPLAY_TIME_NOW, 0));
    now = 125;
    assert(replay_recorder_write_packet(&rec, sizeof packet, packet, REPLAY_TIME_NOW, 0));
    rec.info.totalTime = now;
    replay_recorder_close(&rec);
    assert(!rec.writeFailed);
    file = fopen(path, "rb"); assert(file); size = fread(fixture, 1, sizeof fixture, file); fclose(file);
    assert(size == REPLAY_FIRST_DATA + sizeof data + 254 + 2 * (10 + sizeof packet));
    assert(replay_recorder_open(&playback, path, REPLAY_OPEN_READ));
    assert(playback.info.recordCount == 2 && playback.info.totalTime == 125);
    assert(replay_read_chunk(&playback, 3, &chunk) && chunk.size == sizeof data && !memcmp(chunk.data, data, sizeof data));
    fail_read = 1; assert(!replay_read_chunk(&playback, 3, &chunk)); fail_read = 0;
    fail_seek = 1; assert(!replay_read_chunk(&playback, 3, &chunk)); fail_seek = 0;
    fail_allocate = 1; assert(!replay_read_chunk(&playback, 3, &chunk)); fail_allocate = 0;
    assert(chunk.size == sizeof data && !memcmp(chunk.data, data, sizeof data));
    saved = playback.info;
    replay_recorder_close(&playback);
    for (i = 0; i < size; ++i) {
        write_fixture(path, fixture, i);
        assert(!replay_recorder_open(&playback, path, REPLAY_OPEN_READ));
        assert(!playback.file && !memcmp(&saved, &playback.info, sizeof saved));
    }
    memcpy(malformed, fixture, size);
    entry = rec.chunks[3]; entry.id = 25;
    memcpy(malformed + REPLAY_DIRECTORY_OFFSET + 3 * sizeof entry, &entry, sizeof entry);
    write_fixture(path, malformed, size);
    assert(!replay_recorder_open(&playback, path, REPLAY_OPEN_READ));
    entry.id = -1; memcpy(malformed + REPLAY_DIRECTORY_OFFSET + 3 * sizeof entry, &entry, sizeof entry);
    write_fixture(path, malformed, size); assert(!replay_recorder_open(&playback, path, REPLAY_OPEN_READ));
    entry.id = 3; entry.size = MAXDWORD;
    memcpy(malformed + REPLAY_DIRECTORY_OFFSET + 3 * sizeof entry, &entry, sizeof entry);
    write_fixture(path, malformed, size); assert(!replay_recorder_open(&playback, path, REPLAY_OPEN_READ));
    write_fixture(path, fixture, size);
    assert(replay_recorder_open(&playback, path, REPLAY_OPEN_READ));
    replay_recorder_close(&playback);
    assert(replay_recorder_open(&rec, path, REPLAY_OPEN_WRITE));
    offset = rec.fileOffset; entry = rec.chunks[3];
    fail_write = 1;
    assert(!replay_recorder_write_chunk(&rec, 3, sizeof data, data, REPLAY_CHUNK_APPEND));
    assert(rec.writeFailed && rec.fileOffset == offset && !memcmp(&entry, &rec.chunks[3], sizeof entry));
    assert(!replay_recorder_write_packet(&rec, sizeof packet, packet, REPLAY_TIME_ZERO, 0));
    fail_write = 0;
    replay_recorder_close(&rec);
    replay_buffer_destroy(&chunk);
    replay_recorder_destruct(&rec); replay_recorder_destruct(&playback);
    assert(DeleteFileA(path)); assert(!blocks);
}

int main(void)
{
    states_and_ownership(); file_roundtrip();
    puts("Replay owner: captured buffers, states, clocks, bounded options, round trips, every-byte truncation, directory bounds, I/O failures and repeat cleanup passed");
    return 0;
}
