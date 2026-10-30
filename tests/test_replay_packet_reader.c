#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "replay/replay.h"
#include "replay/packet_reader.h"

static struct ReplayRecorder recorder;
static unsigned char plain[65535], encoded[65535], guarded[65535 + 16];

struct ReplayRecorder *replay_recorder_instance(void) { return &recorder; }

static void open_record(unsigned length, size_t header_bytes, size_t payload_bytes)
{
    struct ReplayRecordHeader header = {0};
    unsigned i;
    memset(&recorder, 0, sizeof recorder);
    recorder.state = REPLAY_STATE_PLAYING;
    recorder.elapsed = 100;
    recorder.info.recordCount = 1;
    recorder.info.cipherKeyA = 0x12345678;
    recorder.info.cipherKeyB = 0x98765432;
    recorder.file = tmpfile();
    assert(recorder.file);
    for (i = 0; i < length; ++i)
        plain[i] = (unsigned char)(i * 19 + 7);
    if (length >= 2) {
        plain[0] = 0xf8;
        plain[1] = 0x09;
    }
    memcpy(encoded, plain, length);
    replay_cipher(&recorder, length, encoded);
    header.clock = 31;
    header.time = 50;
    header.length = (unsigned short)length;
    assert(fwrite(&header, 1, header_bytes, recorder.file) == header_bytes);
    assert(fwrite(encoded, 1, payload_bytes, recorder.file) == payload_bytes);
    assert(fflush(recorder.file) == 0);
    memset(guarded, 0xa5, sizeof guarded);
}

static void expect_failure(size_t capacity)
{
    unsigned short id = 123;
    size_t size = 456;
    DWORD offset = recorder.fileOffset, index = recorder.recordIndex;
    assert(!replay_read_packet(guarded + 7, capacity, &id, &size));
    assert(id == 0 && size == 0);
    assert(recorder.fileOffset == offset && recorder.recordIndex == index);
}

static void complete_packets(void)
{
    static const unsigned sizes[] = {2, 3, 4, 7, 143, 155, 65535};
    unsigned n, i;
    for (n = 0; n < sizeof sizes / sizeof sizes[0]; ++n) {
        unsigned short id;
        size_t size;
        open_record(sizes[n], sizeof(struct ReplayRecordHeader), sizes[n]);
        expect_failure(sizes[n] - 1);
        recorder.elapsed = 49;
        expect_failure(sizes[n]);
        recorder.elapsed = 50;
        assert(replay_read_packet(guarded + 7, sizes[n], &id, &size));
        assert(id == 0x9f8 && size == sizes[n]);
        assert(!memcmp(guarded + 7, plain, size));
        assert(recorder.recordIndex == 1);
        assert(recorder.fileOffset == sizeof(struct ReplayRecordHeader) + size);
        for (i = 0; i < 7; ++i) assert(guarded[i] == 0xa5);
        for (i = 7 + sizes[n]; i < sizeof guarded; ++i) assert(guarded[i] == 0xa5);
        expect_failure(sizes[n]);
        fclose(recorder.file);
    }
}

static void truncation_and_state(void)
{
    unsigned n;
    unsigned short id;
    size_t size;
    for (n = 0; n < sizeof(struct ReplayRecordHeader); ++n) {
        open_record(143, n, 0);
        expect_failure(143);
        fclose(recorder.file);
    }
    for (n = 0; n < 143; ++n) {
        open_record(143, sizeof(struct ReplayRecordHeader), n);
        expect_failure(143);
        fclose(recorder.file);
    }
    for (n = 0; n < 2; ++n) {
        open_record(n, sizeof(struct ReplayRecordHeader), n);
        expect_failure(143);
        fclose(recorder.file);
    }
    open_record(143, sizeof(struct ReplayRecordHeader), 143);
    recorder.state = 0;
    expect_failure(143);
    recorder.state = REPLAY_STATE_PLAYING;
    recorder.fileOffset = (DWORD)LONG_MAX + 1u;
    expect_failure(143);
    recorder.fileOffset = 0;
    assert(!replay_read_packet(NULL, 143, &id, &size));
    assert(!replay_read_packet(guarded, 143, NULL, &size));
    assert(!replay_read_packet(guarded, 143, &id, NULL));
    fclose(recorder.file);
    recorder.file = NULL;
    expect_failure(143);
}

static void cipher_reference(void)
{
    unsigned length, i;
    DWORD value;
    recorder.info.cipherKeyA = 0x76543210;
    recorder.info.cipherKeyB = 0xfedcba98;
    for (length = 0; length <= 257; ++length) {
        memset(guarded, 0xa5, 280);
        for (i = 0; i < length; ++i) guarded[3 + i] = (unsigned char)i;
        memcpy(encoded, guarded + 3, length);
        for (i = 0; i < length / 4; ++i) {
            memcpy(&value, encoded + 4 * i, sizeof value);
            value ^= ((recorder.info.cipherKeyA >> 5) + i + 1) * (recorder.info.cipherKeyB >> 3);
            memcpy(encoded + 4 * i, &value, sizeof value);
        }
        replay_cipher(&recorder, length, guarded + 3);
        assert(!memcmp(guarded + 3, encoded, length));
        assert(guarded[2] == 0xa5 && guarded[3 + length] == 0xa5);
        replay_cipher(&recorder, length, guarded + 3);
        for (i = 0; i < length; ++i) assert(guarded[3 + i] == (unsigned char)i);
    }
}

int main(void)
{
    complete_packets();
    truncation_and_state();
    cipher_reference();
    puts("Replay packets: exact spans, capacity, due times, every-byte truncation, offset bounds and 258 unaligned cipher comparisons passed");
    return 0;
}
