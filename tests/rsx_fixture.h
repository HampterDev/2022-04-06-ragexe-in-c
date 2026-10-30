#ifndef RSX_FIXTURE_H
#define RSX_FIXTURE_H

static unsigned char input[8192];
static int used, frame_offset, node_count_offset, start_offset[2], end_offset[2];
static int texture_offset[2], vertex_offset[2], uv_offset[2], face_offset[2], parent_offset[2];
static const char *node_names[2] = {"자식", "루트"};
static const char *texture_names[2] = {"첫째.bmp", "둘째.bmp"};

static void put(const void *value, size_t size)
{
    assert(size <= sizeof input - (size_t)used);
    memcpy(input + used, value, size);
    used += (int)size;
}
static void integer(int value) { put(&value, sizeof value); }
static void name_record(const char *value)
{
    char bytes[40] = {0};
    assert(strlen(value) <= sizeof bytes);
    memcpy(bytes, value, strlen(value)); put(bytes, sizeof bytes);
}
static void payload(unsigned seed, size_t size)
{
    unsigned char bytes[64];
    size_t i;
    assert(size <= sizeof bytes);
    for (i = 0; i < size; ++i) bytes[i] = (unsigned char)(seed + i * 7);
    put(bytes, size);
}
static void fixture(int major, int minor, int shared)
{
    unsigned char version[2] = {(unsigned char)major, (unsigned char)minor};
    unsigned char skipped[16] = {0};
    float scalar = 1.25f;
    int n, k;
    used = 0;
    put("GRSX", 4); put(version, 2);
    if (major == 1 && minor >= 2) {
        put(&scalar, sizeof scalar); integer(10); integer(20); integer(30);
    }
    if (major == 1 && minor == 3) integer(173);
    integer(7); frame_offset = used; integer(3); integer(19); put(skipped, sizeof skipped);
    integer(2); name_record(texture_names[0]); name_record(texture_names[1]);
    name_record(node_names[1]); node_count_offset = used; integer(2);
    for (n = 0; n < 2; ++n) {
        start_offset[n] = used; integer(0x12345678);
        name_record(node_names[n]); parent_offset[n] = used; name_record(n ? "" : node_names[1]);
        integer(3); texture_offset[n] = used; integer(1); integer(0); integer(1);
        vertex_offset[n] = used; integer(4);
        for (k = 0; k < 3; ++k) payload((unsigned)(n * 20 + k), 4 * 12);
        uv_offset[n] = used; integer(2); integer(shared);
        for (k = 0; k < (shared ? 1 : 3); ++k) payload((unsigned)(90 + n * 20 + k), 2 * 12);
        face_offset[n] = used; integer(2); payload((unsigned)(180 + n * 20), 2 * 24);
        end_offset[n] = used; integer(0x12345678);
    }
}

#endif
