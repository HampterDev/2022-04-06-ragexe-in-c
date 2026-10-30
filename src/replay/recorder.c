#include "session/state.h"
#include <stdlib.h>
#include <limits.h>
#include "replay/replay.h"
#include "platform/service.h"
#include "session/session_state.h"
#include "core/packet_table.h"

#include <string.h>
#include <mmsystem.h>

struct ReplayRecorder g_replayRecorder;
INIT_ONCE g_replayRecorderGuard;
int g_replayPlaying;

#define STORED_PACKET_A 0x99b
#define STORED_PACKET_B 0x1d6

#define OPTION_RECORD_CHAT 6

static void recorder_atexit(void)
{
    replay_recorder_destruct(&g_replayRecorder);
}

struct ReplayRecorder *replay_recorder_instance(void)
{
    if (once_begin(&g_replayRecorderGuard)) {
        replay_recorder_construct(&g_replayRecorder);
        atexit(recorder_atexit);
        once_complete(&g_replayRecorderGuard);
    }
    return &g_replayRecorder;
}

struct ReplayRecorder *replay_recorder_construct(struct ReplayRecorder *self)
{
    memset(self, 0, sizeof *self);
    self->state = REPLAY_STATE_IDLE;
    self->speed = 1.0f;
    self->openMode = REPLAY_OPEN_NONE;
    self->lastTick = timeGetTime();
    memset(self->info.options, 1, sizeof self->info.options);
    self->info.magic = self->info.magicCopy = REPLAY_MAGIC;
    return self;
}

void replay_recorder_destruct(struct ReplayRecorder *self)
{
    replay_recorder_close(self);
    replay_buffer_destroy(&self->storedPackets[0]);
    replay_buffer_destroy(&self->storedPackets[1]);
    memset(self->chunks, 0, sizeof self->chunks);
}

void replay_recorder_reset_session(struct ReplayRecorder *self)
{
    BYTE options[sizeof self->info.options];
    replay_recorder_close(self);
    memcpy(options, self->info.options, sizeof options);
    memset(&self->info, 0, sizeof self->info);
    memcpy(self->info.options, options, sizeof options);
    self->info.magic = self->info.magicCopy = REPLAY_MAGIC;
    memset(self->chunks, 0, sizeof self->chunks);
    self->recordIndex = self->fileOffset = 0;
    self->recordsSinceFlush = 0;
    self->writeFailed = 0;
}

int replay_recorder_tick(struct ReplayRecorder *self)
{
    DWORD now;

    if (!g_replayPlaying) {
        self->clock = (int)((unsigned)self->clock + 1u);
        return 1;
    }
    now = timeGetTime();
    if (self->state != REPLAY_STATE_STOPPED) {
        DWORD delta = now - self->lastTick;
        double advance = (double)((float)delta * self->speed);
        int end = self->info.totalTime > INT_MAX ? INT_MAX : (int)self->info.totalTime;
        self->clock = (int)((unsigned)self->clock + 1u);
        if (self->elapsed < 0)
            self->elapsed = 0;
        if (!(advance >= 0))
            advance = 0;
        if (self->elapsed >= end || advance >= end - self->elapsed) {
            self->lastTick = now;
            self->elapsed = end;
            self->state = REPLAY_STATE_STOPPED;
            return 0;
        }
        self->elapsed += (int)advance;
    }
    self->lastTick = now;
    return 1;
}

void replay_recorder_reset_clock(struct ReplayRecorder *self)
{
    if (self->state == REPLAY_STATE_PAUSED_A || self->state == REPLAY_STATE_PAUSED_B)
        return;
    self->recordIndex = 0;
    self->clock = 0;
    self->elapsed = 0;
}

void replay_recorder_resume(struct ReplayRecorder *self)
{
    DWORD now;

    if (self->state == REPLAY_STATE_PAUSED_A || self->state == REPLAY_STATE_PAUSED_B)
        return;
    self->clockBase = self->clock;
    now = timeGetTime();
    self->lastTick = now;
    self->wallBase = now;
}

void replay_recorder_stamp_total_time(struct ReplayRecorder *self)
{
    self->info.totalTime = timeGetTime() - self->wallBase;
}

struct ReplayDuration replay_recorder_duration(struct ReplayRecorder *self)
{
    struct ReplayDuration d;
    unsigned seconds = self->info.totalTime / 1000;
    unsigned minutes = seconds / 60;
    unsigned hours = minutes / 60;

    d.hours = (WORD)hours;
    d.minutes = (BYTE)(minutes - hours * 60);
    d.seconds = (BYTE)(seconds - minutes * 60);
    return d;
}

BYTE replay_recorder_set_mode(struct ReplayRecorder *self, int mode)
{
    int state;

    if (mode == 4)
        state = 1;
    else if (mode == 5)
        state = 6;
    else
        state = ((unsigned)mode - 1) <= 7 ? mode : 1;
    self->state = state;

    if (((unsigned)mode - 6) <= 2)
        g_replayPlaying = 1;
    else if (((unsigned)mode - 1) <= 2)
        g_replayPlaying = 0;
    return 1;
}

float replay_recorder_duration_scale(struct ReplayRecorder *self)
{
    float speed = self->speed;

    if (speed == 0.25f) return 1.4f;
    if (speed == 0.5f) return 1.2f;
    if (speed == 1.0f) return 1.0f;
    if (speed == 2.0f) return 0.8f;
    if (speed == 4.0f) return 0.6f;
    if (speed == 8.0f) return 0.4f;
    if (speed == 16.0f) return 0.2f;
    return 1.0f;
}

float replay_recorder_inverse_speed(struct ReplayRecorder *self)
{
    float speed = self->speed;

    if (speed == 0.25f) return 4.0f;
    if (speed == 0.5f) return 2.0f;
    if (speed == 1.0f) return 1.0f;
    if (speed == 2.0f) return 0.5f;
    if (speed == 4.0f) return 0.25f;
    if (speed == 8.0f) return 0.125f;
    if (speed == 16.0f) return 0.0625f;
    return 1.0f;
}

int replay_recorder_speed_index(struct ReplayRecorder *self)
{
    float speed = self->speed;

    if (speed == 0.25f) return 1;
    if (speed == 0.5f) return 2;
    if (speed == 1.0f) return 3;
    if (speed == 2.0f) return 4;
    if (speed == 4.0f) return 5;
    if (speed == 8.0f) return 6;
    if (speed == 16.0f) return 7;
    return 0;
}

void replay_recorder_speed_up(struct ReplayRecorder *self)
{
    float speed = self->speed;

    if (speed == 0.25f)
        self->speed = 0.5f;
    else if (speed == 0.5f)
        self->speed = 1.0f;
    else if (speed == 1.0f)
        self->speed = 2.0f;
    else if (speed == 2.0f || speed == 4.0f)
        self->speed = 4.0f;
}

void replay_recorder_slow_down(struct ReplayRecorder *self)
{
    float speed = self->speed;

    if (speed == 0.25f || speed == 0.5f)
        self->speed = 0.25f;
    else if (speed == 1.0f)
        self->speed = 0.5f;
    else if (speed == 2.0f)
        self->speed = 1.0f;
    else if (speed == 4.0f)
        self->speed = 2.0f;
}

BYTE replay_recorder_set_session_info(struct ReplayRecorder *self,
                                             DWORD cipherKeyA, DWORD cipherKeyB, const char *name,
                                             const BYTE *mapData, DWORD mapType, DWORD playerX,
                                             DWORD playerY, DWORD playerDirection, DWORD serviceType,
                                             DWORD serverType, DWORD sex)
{
    if (!name || !mapData)
        return 0;
    memcpy(self->info.mapData, mapData, sizeof self->info.mapData);
    memcpy(self->info.name, name, sizeof self->info.name);
    self->info.serviceType = serviceType;
    self->info.serverType = serverType;
    self->info.sex = sex;
    self->info.cipherKeyA = cipherKeyA;
    self->info.cipherKeyB = cipherKeyB;
    self->info.mapType = mapType;
    self->info.playerX = playerX;
    self->info.playerY = playerY;
    self->info.playerDirection = playerDirection;
    self->info.magic = REPLAY_MAGIC;
    self->info.magicCopy = REPLAY_MAGIC;
    return 1;
}

BYTE replay_recorder_set_options(struct ReplayRecorder *self,
                                          const BYTE *options, size_t length)
{
    if (length > sizeof self->info.options || (!options && length))
        return 0;
    if (length)
        memmove(self->info.options, options, length);
    return 1;
}

BYTE replay_recorder_server_type_matches(struct ReplayRecorder *self, int which)
{
    int value = (int)self->info.serverType;

    if (which == 2)
        return value == 0 || value == 13 || value == 1 || value == 2;
    return g_serverType == value;
}

BYTE replay_recorder_should_record_packet(struct ReplayRecorder *self,
                                                  unsigned short packetId)
{
    if (packetId == 0x8ae)
        return 1;
    if (!packet_length_lookup(&g_packetLengthMap, packetId))
        return 0;
    switch (packetId) {
    case 0x8d: case 0x8e: case 0x97: case 0x109: case 0x17f: case 0x2dc:
        return self->info.options[OPTION_RECORD_CHAT];
    }
    return 1;
}

BYTE replay_recorder_blocks_action(struct ReplayRecorder *self, int actionId)
{
    if (!g_replayPlaying)
        return 0;
    switch (actionId) {
    case 0x107: case 0x1f6: case 0xc1: case 0x116:
        return 0;
    case 0xbb: return self->info.options[1] == 0;
    case 0xba: return self->info.options[2] == 0;
    case 0xbc: return self->info.options[3] == 0;
    case 0xbf: return self->info.options[5] == 0;
    case 0xb8: return self->info.options[8] == 0;
    }
    return 1;
}

void replay_recorder_capture_packet(struct ReplayRecorder *self,
                                            unsigned short packetId, unsigned short length, const char *data)
{
    struct ReplayBuffer *slot;
    unsigned char *copy;
    if (!length || !data || (packetId != STORED_PACKET_A && packetId != STORED_PACKET_B))
        return;
    slot = &self->storedPackets[packetId == STORED_PACKET_A ? 0 : 1];
    copy = malloc(length);
    if (!copy)
        return;
    memcpy(copy, data, length);
    replay_buffer_destroy(slot);
    slot->data = copy;
    slot->size = length;
}

void replay_recorder_write_stored_packets(struct ReplayRecorder *self)
{
    static const short sessionPackets[] = { 0x10f, 0x235, 0x29d };
    struct Vec packet;
    int i;

    for (i = 0; i < 2; ++i)
        replay_recorder_write_packet(self, (unsigned short)self->storedPackets[i].size,
                                   self->storedPackets[i].data, REPLAY_TIME_ZERO, 0);
    for (i = 0; i < 3; i++) {
        session_get_stored_packet(&g_session1, &packet, sessionPackets[i]);
        replay_recorder_write_packet_vector(self, &packet, REPLAY_TIME_ZERO, 0);
        vec_free(&packet);
    }
    if (g_session1.cartType) {
        session_get_stored_packet2(&g_session1, &packet);
        replay_recorder_write_packet_vector(self, &packet, REPLAY_TIME_ZERO, 0);
        vec_free(&packet);
    }
}
