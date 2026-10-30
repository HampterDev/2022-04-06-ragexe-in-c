#include "replay/replay.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>

#define REPLAY_LAST_CHUNK_MARK 0x6f

static int file_size(FILE *file, DWORD *size)
{
    long end;
    if (!file || fseek(file, 0, SEEK_END) || (end = ftell(file)) < 0)
        return 0;
    *size = (DWORD)end;
    return 1;
}

static int read_at(FILE *file, DWORD offset, void *data, size_t size)
{
    return file && offset <= LONG_MAX && size <= LONG_MAX - offset &&
           !fseek(file, (long)offset, SEEK_SET) && fread(data, 1, size, file) == size;
}

static int write_at(struct ReplayRecorder *self, DWORD offset, const void *data, size_t size)
{
    if (!self->file || self->openMode != REPLAY_OPEN_WRITE || self->writeFailed)
        return 0;
    if (offset > LONG_MAX || size > LONG_MAX - offset ||
        fseek(self->file, (long)offset, SEEK_SET) || fwrite(data, 1, size, self->file) != size) {
        self->writeFailed = 1;
        return 0;
    }
    return 1;
}

int replay_recorder_find_chunk(struct ReplayRecorder *self, int id)
{
    int i;
    if (id <= 0 || id >= REPLAY_CHUNK_COUNT)
        return -1;
    for (i = 0; i < REPLAY_CHUNK_COUNT; ++i)
        if (self->chunks[i].id == id)
            return i;
    return -1;
}

int replay_read_chunk(struct ReplayRecorder *self, int id, struct ReplayBuffer *out)
{
    struct ReplayBuffer result = {0};
    const struct ReplayChunkEntry *entry;
    DWORD end;
    int index = replay_recorder_find_chunk(self, id);
    if (index < 0 || !file_size(self->file, &end))
        return 0;
    entry = &self->chunks[index];
    if (entry->offset < REPLAY_FIRST_DATA || entry->offset > end || entry->size > end - entry->offset)
        return 0;
    if (entry->size) {
        result.data = malloc(entry->size);
        if (!result.data)
            return 0;
        result.size = entry->size;
        if (!read_at(self->file, entry->offset, result.data, result.size)) {
            replay_buffer_destroy(&result);
            return 0;
        }
        replay_cipher(self, (DWORD)result.size, result.data);
    }
    replay_buffer_destroy(out);
    *out = result;
    return 1;
}

static int last_record_time(struct ReplayRecorder *self, DWORD *time)
{
    struct ReplayRecordHeader header;
    DWORD offset, end, i;
    int index = replay_recorder_find_chunk(self, REPLAY_FIRST_RECORD);
    *time = 0;
    if (!self->info.recordCount)
        return 1;
    if (index < 0 || !file_size(self->file, &end))
        return 0;
    offset = self->chunks[index].offset;
    if (offset > end || self->info.recordCount > (end - offset) / (sizeof header + 2))
        return 0;
    for (i = 0; i < self->info.recordCount; ++i) {
        if (end - offset < sizeof header || !read_at(self->file, offset, &header, sizeof header))
            return 0;
        offset += sizeof header;
        if (header.length < 2 || header.length > end - offset)
            return 0;
        offset += header.length;
        *time = header.time;
    }
    return 1;
}

static int read_header(struct ReplayRecorder *candidate)
{
    struct ReplayChunkEntry entries[REPLAY_CHUNK_COUNT];
    struct ReplayBuffer chunk = {0};
    DWORD end, time;
    int i, parsed, first;
    unsigned seen = 0;
    if (!file_size(candidate->file, &end) || end < REPLAY_FIRST_DATA ||
        !read_at(candidate->file, REPLAY_BANNER_SIZE, &candidate->info, REPLAY_FILE_HEADER_SIZE) ||
        (candidate->info.magic != REPLAY_MAGIC && candidate->info.magic != REPLAY_MAGIC_PARTIAL) ||
        !read_at(candidate->file, REPLAY_DIRECTORY_OFFSET, entries, sizeof entries))
        return 0;
    for (i = 0; i < REPLAY_CHUNK_COUNT; ++i) {
        const struct ReplayChunkEntry *entry = &entries[i];
        if (!entry->id) {
            if (entry->size || entry->offset)
                return 0;
            continue;
        }
        if (entry->id == REPLAY_LAST_CHUNK_MARK && i == REPLAY_LAST_CHUNK && !entry->size && !entry->offset) {
            candidate->chunks[i] = *entry;
            continue;
        }
        if (entry->id <= 0 || entry->id >= REPLAY_CHUNK_COUNT || (seen & (1u << entry->id)) ||
            entry->offset < REPLAY_FIRST_DATA || entry->offset > end || entry->size > end - entry->offset)
            return 0;
        seen |= 1u << entry->id;
        candidate->chunks[entry->id] = *entry;
    }
    if (!replay_read_chunk(candidate, REPLAY_SESSION_CHUNK, &chunk))
        return 0;
    parsed = replay_parse_session_chunk(&candidate->info, chunk.data, chunk.size);
    replay_buffer_destroy(&chunk);
    if (parsed < 0 || !memchr(candidate->info.name, 0, sizeof candidate->info.name) ||
        !memchr(candidate->info.mapData, 0, 16))
        return 0;
    first = replay_recorder_find_chunk(candidate, REPLAY_FIRST_RECORD);
    if (candidate->info.recordCount && (first < 0 ||
        candidate->info.recordCount > (end - candidate->chunks[first].offset) / (REPLAY_RECORD_HEADER + 2)))
        return 0;
    if (candidate->info.dataEnd > end)
        return 0;
    candidate->fileOffset = first < 0 ? 0 : candidate->chunks[first].offset;
    if (candidate->info.magic == REPLAY_MAGIC_PARTIAL || candidate->info.magic != candidate->info.magicCopy) {
        if (!last_record_time(candidate, &time))
            return 0;
        candidate->info.totalTime = time;
    }
    return 1;
}

static void adopt_header(struct ReplayRecorder *self, const struct ReplayRecorder *candidate)
{
    self->info = candidate->info;
    memcpy(self->chunks, candidate->chunks, sizeof self->chunks);
    self->fileOffset = candidate->fileOffset;
    self->recordIndex = 0;
}

BYTE replay_recorder_open(struct ReplayRecorder *self, const char *path, int mode)
{
    struct ReplayRecorder candidate = {0};
    if (!path || (mode != REPLAY_OPEN_READ && mode != REPLAY_OPEN_WRITE))
        return 0;
    if (mode == REPLAY_OPEN_READ) {
        candidate.file = fopen(path, "rb");
        if (!candidate.file)
            return 0;
        if (!read_header(&candidate)) {
            fclose(candidate.file);
            return 0;
        }
        replay_recorder_close(self);
        self->file = candidate.file;
        adopt_header(self, &candidate);
    } else {
        replay_recorder_close(self);
        self->file = fopen(path, "wb");
        if (!self->file)
            return 0;
        memset(self->chunks, 0, sizeof self->chunks);
        self->fileOffset = REPLAY_FIRST_DATA;
        self->recordIndex = 0;
    }
    self->openMode = mode;
    self->recordsSinceFlush = 0;
    self->writeFailed = 0;
    return 1;
}

void replay_recorder_close(struct ReplayRecorder *self)
{
    if (self->file) {
        if (self->openMode == REPLAY_OPEN_WRITE && !self->writeFailed)
            replay_recorder_on_event(self, REPLAY_EVENT_CLOSE);
        if (fclose(self->file))
            self->writeFailed = 1;
        self->file = NULL;
    }
    self->openMode = REPLAY_OPEN_NONE;
}

BYTE replay_recorder_write_chunk(struct ReplayRecorder *self, int slot, DWORD size,
                              const void *data, int placement)
{
    struct ReplayChunkEntry entry;
    unsigned char *encoded;
    int written;
    if (slot <= 0 || slot >= REPLAY_CHUNK_COUNT || !size || !data ||
        !self->file || self->openMode != REPLAY_OPEN_WRITE || self->writeFailed)
        return 0;
    entry = self->chunks[slot];
    if (placement == REPLAY_CHUNK_APPEND) {
        entry.id = (short)slot;
        entry.size = size;
        entry.offset = self->fileOffset;
    } else if (placement != REPLAY_CHUNK_REWRITE || entry.id != slot || entry.size != size) {
        return 0;
    }
    if (entry.offset < REPLAY_FIRST_DATA || entry.offset > LONG_MAX || size > LONG_MAX - entry.offset)
        return 0;
    encoded = malloc(size);
    if (!encoded)
        return 0;
    memcpy(encoded, data, size);
    replay_cipher(self, size, encoded);
    written = write_at(self, entry.offset, encoded, size);
    free(encoded);
    if (!written)
        return 0;
    self->chunks[slot] = entry;
    if (placement == REPLAY_CHUNK_APPEND)
        self->fileOffset += size;
    return 1;
}

int replay_recorder_on_event(struct ReplayRecorder *self, int event)
{
    struct ReplayBuffer chunk = {0};
    int written, placement;
    if (!self->file || self->openMode != REPLAY_OPEN_WRITE || self->writeFailed ||
        (event != REPLAY_EVENT_START && event != REPLAY_EVENT_RECORD && event != REPLAY_EVENT_CLOSE))
        return 0;
    self->info.recordCount = self->recordIndex;
    self->info.dataEnd = self->fileOffset;
    self->info.magicCopy = self->info.magic = REPLAY_MAGIC;
    if (event == REPLAY_EVENT_RECORD) {
        if (++self->recordsSinceFlush < REPLAY_FLUSH_INTERVAL)
            return 1;
        self->info.magicCopy = self->info.magic = REPLAY_MAGIC_PARTIAL;
    }
    placement = event == REPLAY_EVENT_START ? REPLAY_CHUNK_APPEND : REPLAY_CHUNK_REWRITE;
    if (!replay_build_session_chunk(&self->info, &chunk)) {
        self->writeFailed = 1;
        return 0;
    }
    written = replay_recorder_write_chunk(self, REPLAY_SESSION_CHUNK, (DWORD)chunk.size, chunk.data, placement);
    replay_buffer_destroy(&chunk);
    if (!written) {
        self->writeFailed = 1;
        return 0;
    }
    if (event == REPLAY_EVENT_START) {
        char banner[REPLAY_BANNER_SIZE] = {0};
        self->chunks[REPLAY_LAST_CHUNK].id = REPLAY_LAST_CHUNK_MARK;
        snprintf(banner, sizeof banner, "<< Ragnarok Replay File Version 0.05 >>\r\n ※ Do not modify file! ");
        memcpy(banner + 0x5f, "\r\n", 3);
        if (!write_at(self, 0, banner, sizeof banner) ||
            !write_at(self, REPLAY_BANNER_SIZE, &self->info, REPLAY_FILE_HEADER_SIZE))
            return 0;
    }
    if (!write_at(self, REPLAY_DIRECTORY_OFFSET, self->chunks, sizeof self->chunks) || fflush(self->file)) {
        self->writeFailed = 1;
        return 0;
    }
    self->recordsSinceFlush = 0;
    return 1;
}

void replay_recorder_rewind(struct ReplayRecorder *self)
{
    int index = replay_recorder_find_chunk(self, REPLAY_FIRST_RECORD);
    self->recordIndex = 0;
    self->fileOffset = index < 0 ? 0 : self->chunks[index].offset;
    if (index < 0)
        self->state = REPLAY_STATE_STOPPED;
}

BYTE replay_recorder_mark_first_record(struct ReplayRecorder *self)
{
    if (!self->file || self->openMode != REPLAY_OPEN_WRITE || self->writeFailed)
        return 0;
    if (!self->recordIndex) {
        self->chunks[REPLAY_FIRST_RECORD].id = REPLAY_FIRST_RECORD;
        self->chunks[REPLAY_FIRST_RECORD].size = 0;
        self->chunks[REPLAY_FIRST_RECORD].offset = self->fileOffset;
    }
    return 1;
}

BYTE replay_recorder_write_packet(struct ReplayRecorder *self, unsigned short length,
                               const void *packet, int timeMode, DWORD timeAt)
{
    struct ReplayRecordHeader header = {0};
    unsigned short id;
    unsigned char *record;
    size_t size = sizeof header + length;
    int written;
    if (!packet || length < sizeof id || !self->file || self->openMode != REPLAY_OPEN_WRITE ||
        self->writeFailed || self->fileOffset > LONG_MAX || size > LONG_MAX - self->fileOffset ||
        self->recordIndex == MAXDWORD ||
        (timeMode != REPLAY_TIME_ZERO && timeMode != REPLAY_TIME_NOW && timeMode != REPLAY_TIME_AT))
        return 0;
    memcpy(&id, packet, sizeof id);
    if (!replay_recorder_should_record_packet(self, id))
        return 0;
    record = malloc(size);
    if (!record) {
        self->writeFailed = 1;
        return 0;
    }
    header.length = length;
    if (timeMode != REPLAY_TIME_ZERO) {
        header.clock = (DWORD)self->clock - (DWORD)self->clockBase;
        header.time = timeGetTime() - self->wallBase;
        if (timeMode == REPLAY_TIME_AT)
            header.time += timeAt;
    }
    memcpy(record, &header, sizeof header);
    memcpy(record + sizeof header, packet, length);
    replay_cipher(self, length, record + sizeof header);
    written = write_at(self, self->fileOffset, record, size);
    free(record);
    if (!written)
        return 0;
    ++self->recordIndex;
    self->fileOffset += (DWORD)size;
    return 1;
}

BYTE replay_recorder_write_packet_vector(struct ReplayRecorder *self, struct Vec *packet,
                                     int timeMode, DWORD timeAt)
{
    size_t size;
    if (!packet->first)
        return 0;
    size = (char *)packet->last - (char *)packet->first;
    if (size > USHRT_MAX)
        return 0;
    return replay_recorder_write_packet(self, (unsigned short)size, packet->first, timeMode, timeAt);
}
