#include "replay/packet_reader.h"
#include "replay/replay.h"
#include <limits.h>
#include <string.h>

BYTE replay_cipher(struct ReplayRecorder *self, DWORD length, void *data)
{
    unsigned char *bytes = data;
    DWORD i, value;
    if (!data || !length)
        return 0;
    self->cipherCounter = 0;
    self->cipherKeyB = self->info.cipherKeyB >> 3;
    self->cipherKeyA = self->info.cipherKeyA >> 5;
    for (i = 0; i < length / sizeof value; ++i) {
        memcpy(&value, bytes + i * sizeof value, sizeof value);
        ++self->cipherCounter;
        value ^= (self->cipherKeyA + self->cipherCounter) * self->cipherKeyB;
        memcpy(bytes + i * sizeof value, &value, sizeof value);
    }
    return 1;
}

int replay_read_packet(void *buffer, size_t capacity,
                       unsigned short *packet_id, size_t *size)
{
    struct ReplayRecorder *self;
    struct ReplayRecordHeader header;
    if (packet_id)
        *packet_id = 0;
    if (size)
        *size = 0;
    if (!buffer || !packet_id || !size || capacity < sizeof *packet_id)
        return 0;
    self = replay_recorder_instance();
    if (!self || self->state != REPLAY_STATE_PLAYING || !self->file ||
        self->recordIndex >= self->info.recordCount || self->fileOffset > LONG_MAX)
        return 0;
    if (fseek(self->file, (long)self->fileOffset, SEEK_SET) != 0 ||
        fread(&header, 1, sizeof header, self->file) != sizeof header ||
        (DWORD)self->elapsed < header.time ||
        header.length < sizeof *packet_id || header.length > capacity ||
        self->fileOffset > LONG_MAX - sizeof header - header.length)
        return 0;
    if (fread(buffer, 1, header.length, self->file) != header.length)
        return 0;
    replay_cipher(self, header.length, buffer);
    memcpy(packet_id, buffer, sizeof *packet_id);
    *size = header.length;
    self->fileOffset += sizeof header + header.length;
    ++self->recordIndex;
    return 1;
}
