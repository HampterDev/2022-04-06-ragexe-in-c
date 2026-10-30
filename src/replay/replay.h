#ifndef REPLAY_H
#define REPLAY_H

#include "base.h"
#include "replay/snapshot.h"
#include "login/server_entry.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define REPLAY_MAGIC            0x52414705
#define REPLAY_MAGIC_PARTIAL    0x51404605
#define REPLAY_BANNER_SIZE      100
#define REPLAY_FILE_HEADER_SIZE 12
#define REPLAY_DIRECTORY_OFFSET 0x70
#define REPLAY_CHUNK_COUNT      25
#define REPLAY_FIRST_DATA       0x16a
#define REPLAY_RECORD_HEADER    10
#define REPLAY_FLUSH_INTERVAL   1200

#define REPLAY_OPEN_NONE        (-1)
#define REPLAY_OPEN_WRITE       0x1f
#define REPLAY_OPEN_READ        0x20

#define REPLAY_EVENT_START      0xb
#define REPLAY_EVENT_RECORD     0xc
#define REPLAY_EVENT_CLOSE      0xd

#define REPLAY_TIME_ZERO        0x29
#define REPLAY_TIME_NOW         0x2a
#define REPLAY_TIME_AT          0x2b

#define REPLAY_STATE_IDLE       1
#define REPLAY_STATE_RECORDING  2

#define REPLAY_STATE_PAUSED_A   3
#define REPLAY_STATE_STOPPED    6
#define REPLAY_STATE_PLAYING    7
#define REPLAY_STATE_PAUSED_B   8

enum {
    REPLAY_TAG_BEGIN        = 0x3b6,
    REPLAY_TAG_END          = 0x3b7,
    REPLAY_TAG_MAGIC        = 0x3c0,
    REPLAY_TAG_SERVICE_TYPE     = 0x3c1,
    REPLAY_TAG_SERVER_TYPE     = 0x3c2,
    REPLAY_TAG_SEX     = 0x3c3,
    REPLAY_TAG_NAME         = 0x3c4,
    REPLAY_TAG_MAP_DATA     = 0x3c5,
    REPLAY_TAG_MAP_TYPE     = 0x3c6,
    REPLAY_TAG_PLAYER_X     = 0x3c7,
    REPLAY_TAG_PLAYER_Y     = 0x3c8,
    REPLAY_TAG_PLAYER_DIRECTION     = 0x3c9,
    REPLAY_TAG_TOTAL_TIME   = 0x3ca,
    REPLAY_TAG_RECORD_COUNT = 0x3cb,
    REPLAY_TAG_DATA_END     = 0x3cc,
    REPLAY_TAG_OPTIONS      = 0x3cd
};
#define REPLAY_SESSION_CHUNK    2
#define REPLAY_FIRST_RECORD     1
#define REPLAY_LAST_CHUNK       24

#define REPLAY_CHUNK_APPEND     0xe
#define REPLAY_CHUNK_REWRITE    0xf

#pragma pack(push, 1)

struct ReplayChunkEntry {
    short id;
    DWORD size;
    DWORD offset;
};

struct ReplayRecordHeader {
    DWORD clock;
    DWORD time;
    WORD length;
};

struct ReplaySessionInfo {
    DWORD magic;
    DWORD cipherKeyA;
    DWORD cipherKeyB;
    DWORD serviceType;
    DWORD serverType;
    DWORD sex;
    char name[64];
    BYTE mapData[20];
    DWORD mapType;
    DWORD playerX;
    DWORD playerY;
    DWORD playerDirection;
    DWORD totalTime;
    DWORD recordCount;
    DWORD dataEnd;
    BYTE options[30];
    DWORD magicCopy;
};

#pragma pack(pop)

SIZE_ASSERT(ReplayChunkEntry, 10);
SIZE_ASSERT(ReplayRecordHeader, 10);
SIZE_ASSERT(ReplaySessionInfo, 0xaa);

struct ReplayRecorder {
    struct ReplayBuffer storedPackets[2];
    int state, clockBase;
    DWORD wallBase, lastTick;
    int clock, elapsed;
    float speed;
    struct ReplaySessionInfo info;
    struct ReplayChunkEntry chunks[REPLAY_CHUNK_COUNT];
    int openMode;
    FILE *file;
    DWORD fileOffset, recordIndex;
    DWORD cipherCounter, cipherKeyA, cipherKeyB;
    unsigned recordsSinceFlush;
    int writeFailed;
    int windowX, windowY;
};

struct ReplayFileEntry { int number; char name[MAX_PATH]; };
struct ReplayFileList {
    int selected;
    struct ReplayFileEntry *files;
    size_t count, capacity;
    struct ReplayRecorder *recorder;
    char path[MAX_PATH];
    BYTE useCustomName, customNameSet;
    int aid, sex;
    int serverCount;
    struct LoginServerEntry servers[100];
};

int replay_read_chunk(struct ReplayRecorder *self, int id, struct ReplayBuffer *out);
int replay_build_session_chunk(const struct ReplaySessionInfo *info, struct ReplayBuffer *out);
int replay_parse_session_chunk(struct ReplaySessionInfo *info, const void *data, size_t size);

struct ReplayDuration {
    WORD hours;
    BYTE minutes;
    BYTE seconds;
};

extern struct ReplayRecorder g_replayRecorder;
extern INIT_ONCE g_replayRecorderGuard;
extern struct ReplayFileList g_replayFileList;
extern INIT_ONCE g_replayFileListGuard;
extern int g_replayPlaying;
struct ReplayRecorder *replay_recorder_instance(void);
struct ReplayRecorder *replay_recorder_construct(struct ReplayRecorder *self);
void replay_recorder_destruct(struct ReplayRecorder *self);
void replay_recorder_reset_session(struct ReplayRecorder *self);
int replay_recorder_tick(struct ReplayRecorder *self);
void replay_recorder_reset_clock(struct ReplayRecorder *self);
void replay_recorder_resume(struct ReplayRecorder *self);
BYTE replay_recorder_set_mode(struct ReplayRecorder *self, int mode);
void replay_recorder_stamp_total_time(struct ReplayRecorder *self);
struct ReplayDuration replay_recorder_duration(struct ReplayRecorder *self);
float replay_recorder_duration_scale(struct ReplayRecorder *self);
float replay_recorder_inverse_speed(struct ReplayRecorder *self);
int replay_recorder_speed_index(struct ReplayRecorder *self);
void replay_recorder_speed_up(struct ReplayRecorder *self);
void replay_recorder_slow_down(struct ReplayRecorder *self);
BYTE replay_recorder_set_session_info(struct ReplayRecorder *self,
                                             DWORD cipherKeyA, DWORD cipherKeyB, const char *name,
                                             const BYTE *mapData, DWORD mapType, DWORD playerX,
                                             DWORD playerY, DWORD playerDirection, DWORD serviceType,
                                             DWORD serverType, DWORD sex);
BYTE replay_recorder_set_options(struct ReplayRecorder *self,
                                          const BYTE *options, size_t length);
BYTE replay_recorder_server_type_matches(struct ReplayRecorder *self, int which);
BYTE replay_recorder_should_record_packet(struct ReplayRecorder *self,
                                                  unsigned short packetId);
BYTE replay_recorder_blocks_action(struct ReplayRecorder *self, int actionId);
void replay_recorder_capture_packet(struct ReplayRecorder *self,
                                            unsigned short packetId, unsigned short length, const char *data);

BYTE replay_cipher(struct ReplayRecorder *self, DWORD length, void *data);
BYTE replay_recorder_open(struct ReplayRecorder *self, const char *path,
                                    int mode);
void replay_recorder_close(struct ReplayRecorder *self);
int replay_recorder_find_chunk(struct ReplayRecorder *self, int id);
BYTE replay_recorder_write_chunk(struct ReplayRecorder *self, int slot,
                                          DWORD size, const void *data, int placement);
int replay_recorder_on_event(struct ReplayRecorder *self, int event);
void replay_recorder_rewind(struct ReplayRecorder *self);
BYTE replay_recorder_mark_first_record(struct ReplayRecorder *self);
BYTE replay_recorder_write_packet_vector(struct ReplayRecorder *self,
                                                 struct Vec *packet, int timeMode, DWORD timeAt);
BYTE replay_recorder_write_packet(struct ReplayRecorder *self,
                                           unsigned short length, const void *packet, int timeMode,
                                           DWORD timeAt);
void replay_recorder_write_stored_packets(struct ReplayRecorder *self);

struct ReplayFileList *replay_file_list_instance(void);
struct ReplayFileList *replay_file_list_construct(struct ReplayFileList *self);
void replay_file_list_destruct(struct ReplayFileList *self);
void replay_file_list_reset(struct ReplayFileList *self);
BYTE replay_file_list_custom_name_taken(struct ReplayFileList *self);
BYTE replay_file_list_scan(struct ReplayFileList *self);
const char *replay_file_list_get_name(struct ReplayFileList *self, unsigned index);
BYTE replay_file_list_select(struct ReplayFileList *self, int index);
char *replay_file_list_set_name(struct ReplayFileList *self, const char *name);
BYTE replay_file_list_set_recorder(struct ReplayFileList *self,
                                           struct ReplayRecorder *recorder);
int replay_file_list_make_recording_name(struct ReplayFileList *self);
int replay_file_list_open_selected(struct ReplayFileList *self, int mode);

#endif
