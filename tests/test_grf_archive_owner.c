#include "fileio/file.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "owned_storage_test_allocator.h"

static unsigned live_handles, live_views, map_calls;
static int fail_mapping, fail_view, fail_seek, short_read;
static size_t reported_size = SIZE_MAX;
static HANDLE fixture_open(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
                           DWORD creation, DWORD flags, HANDLE template_file)
{
    HANDLE h = CreateFileA(name, access, share, security, creation, flags, template_file);
    if (h != INVALID_HANDLE_VALUE) ++live_handles;
    return h;
}
static HANDLE fixture_mapping(HANDLE h, LPSECURITY_ATTRIBUTES security, DWORD protection,
                              DWORD high, DWORD low, LPCSTR name)
{
    HANDLE mapping = fail_mapping ? NULL : CreateFileMappingA(h, security, protection, high, low, name);
    if (mapping) ++live_handles;
    return mapping;
}
static LPVOID fixture_view(HANDLE h, DWORD access, DWORD high, DWORD low, SIZE_T size)
{
    void *view;
    ++map_calls;
    view = fail_view ? NULL : MapViewOfFile(h, access, high, low, size);
    if (view) ++live_views;
    return view;
}
static BOOL fixture_unmap(LPCVOID p)
{ assert(live_views); --live_views; return UnmapViewOfFile(p); }
static BOOL fixture_close(HANDLE h)
{ assert(live_handles); --live_handles; return CloseHandle(h); }
static BOOL fixture_size(HANDLE h, PLARGE_INTEGER size)
{
    BOOL ok = GetFileSizeEx(h, size);
    if (ok && reported_size != SIZE_MAX) size->QuadPart = reported_size;
    return ok;
}
static BOOL fixture_seek(HANDLE h, LARGE_INTEGER at, PLARGE_INTEGER out, DWORD origin)
{ return fail_seek ? FALSE : SetFilePointerEx(h, at, out, origin); }
static BOOL fixture_read(HANDLE h, LPVOID data, DWORD size, LPDWORD read, LPOVERLAPPED overlapped)
{ return ReadFile(h, data, short_read && size ? size - 1 : size, read, overlapped); }
static void *checked_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void *checked_realloc(void *p, size_t size)
{
    void *next = checked_malloc(size);
    if (next && p) {
        size_t old = ((struct Allocation *)p - 1)->size;
        memcpy(next, p, old < size ? old : size); checked_free(p);
    }
    return next;
}
#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#define CreateFileA fixture_open
#define CreateFileMappingA fixture_mapping
#define MapViewOfFile fixture_view
#define UnmapViewOfFile fixture_unmap
#define CloseHandle fixture_close
#define GetFileSizeEx fixture_size
#define SetFilePointerEx fixture_seek
#define ReadFile fixture_read
#include "../src/fileio/grf_mapfile.c"
#include "../src/fileio/grf_pak.c"
#include "../src/fileio/file_mgr.c"
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef CreateFileA
#undef CreateFileMappingA
#undef MapViewOfFile
#undef UnmapViewOfFile
#undef CloseHandle
#undef GetFileSizeEx
#undef SetFilePointerEx
#undef ReadFile
#include "../src/fileio/grf_cipher.c"
#include "grf_fixture.h"

void *mem_alloc(size_t n) { (void)n; abort(); }
void mem_free(void *p)
{ (void)p; abort(); }

static void write_fixture(const char *path)
{
    FILE *file = fopen(path, "wb"); assert(file);
    assert(fwrite(archive_bytes, archive_size, 1, file) == 1);
    assert(!fclose(file));
}
static void empty_owners(void)
{ assert(!outstanding && !live_handles && !live_views); }

static void reference_cases(const char *path, const char *gold_path)
{
    const unsigned versions[] = {0x100,0x101,0x102,0x103,0x200,0x201};
    unsigned mode, v, i;
    for (mode = 0; mode < 2; ++mode) {
        FILE *gold = fopen(gold_path, "rb"); assert(gold);
        fail_mapping = (int)mode;
        for (v = 0; v < sizeof versions / sizeof versions[0]; ++v) {
            struct GrfArchive *archive;
            make_grf_fixture(versions[v]); write_fixture(path);
            archive = grf_archive_create(path); assert(archive);
            assert(archive->entry_count == GRF_FIXTURE_COUNT && archive->version == versions[v]);
            for (i = 0; i < GRF_FIXTURE_COUNT; ++i) {
                struct GrfEntry entry, expected;
                struct Hash key;
                char name[252]; BYTE bytes[GRF_FIXTURE_PAYLOAD + 16], payload[GRF_FIXTURE_PAYLOAD];
                fixture_name(i, name); hash_construct(&key, name);
                assert(grf_archive_find(archive, &key, &entry));
                assert(grf_archive_find(archive, &key, NULL));
                assert(fread(&expected, sizeof expected, 1, gold) == 1);
                assert(fread(payload, sizeof payload, 1, gold) == 1);
                assert(!memcmp(&entry, &expected, sizeof entry));
                memset(bytes, 0xa5, sizeof bytes);
                assert(!grf_archive_read(archive, &entry, bytes, sizeof payload - 1));
                assert(bytes[0] == 0xa5);
                assert(grf_archive_read(archive, &entry, bytes, sizeof payload));
                assert(!memcmp(bytes, payload, sizeof payload));
                for (unsigned j = sizeof payload; j < sizeof bytes; ++j) assert(bytes[j] == 0xa5);
            }
            grf_archive_destroy(archive); empty_owners();
        }
        assert(fgetc(gold) == EOF); assert(!fclose(gold));
    }
    fail_mapping = 0;
    puts("GRF ownership: 240 archived entries/payloads match in mapped and fallback modes, including duplicate/Korean names and six versions");
}

static void mapping_failures(const char *path)
{
    struct GrfFile file;
    const BYTE *data;
    unsigned i;
    grf_file_init(&file);
    assert(!grf_file_read(&file, 0, 0));
    make_grf_fixture(0x200); write_fixture(path);
    assert(grf_file_open(&file, path));
    assert(!grf_file_read(&file, UINT_MAX, 8));
    assert(!grf_file_read(&file, 1, UINT_MAX));
    assert(grf_file_read(&file, archive_size, 0));
    fail_view = 1;
    data = grf_file_read(&file, 1, 16); assert(data && !memcmp(data, archive_bytes + 1, 16));
    assert(map_calls > 1 && !file.mapping && !file.view);
    fail_seek = 1; assert(!grf_file_read(&file, 0, 16)); fail_seek = 0;
    short_read = 1; assert(!grf_file_read(&file, 0, 16)); short_read = 0;
    fail_view = 0;
    fail_at = allocation_calls + 1;
    assert(!grf_file_read(&file, 0, 32)); fail_at = 0;
    assert(grf_file_read(&file, 0, 16) == file.buffer);
    grf_file_clear(&file); grf_file_clear(&file); empty_owners();
    memset(archive_bytes, 0, sizeof archive_bytes);
    for (i = 0; i < sizeof archive_bytes; ++i) archive_bytes[i] = (BYTE)(i ^ (i >> 8));
    archive_size = sizeof archive_bytes; write_fixture(path);
    assert(grf_file_open(&file, path)); file.map_chunk = 32768;
    data = grf_file_read(&file, 32760, 32);
    assert(data && !memcmp(data, archive_bytes + 32760, 32) && !file.mapping);
    grf_file_clear(&file); empty_owners();
}

static void allocation_and_read_failures(const char *path)
{
    unsigned version, mode;
    for (version = 0x103; version <= 0x200; version += 0xfd) {
        make_grf_fixture(version); write_fixture(path);
        for (mode = 0; mode < 2; ++mode) {
            struct GrfArchive *archive; unsigned calls, failure;
            fail_mapping = mode; allocation_calls = 0;
            archive = grf_archive_create(path); assert(archive); calls = allocation_calls;
            grf_archive_destroy(archive); empty_owners();
            for (failure = 1; failure <= calls; ++failure) {
                allocation_calls = 0; fail_at = failure;
                assert(!grf_archive_create(path)); empty_owners();
            }
            fail_at = 0;
        }
        fail_mapping = 0;
        for (reported_size = 0; reported_size < archive_size; ++reported_size) {
            assert(!grf_archive_create(path)); empty_owners();
        }
        reported_size = SIZE_MAX;
    }
    puts("GRF ownership: every allocation and byte truncation fails cleanly for both table formats");
}

static void malformed_records(const char *path)
{
    unsigned v, which;
    for (v = 0x103; v <= 0x200; v += 0xfd) {
        for (which = 0; which < 8; ++which) {
            make_grf_fixture(v);
            if (which == 0) archive_bytes[0] = 0;
            if (which == 1) store32(archive_bytes + 0x22, UINT_MAX);
            if (which == 2) store32(archive_bytes + 0x26, 0);
            if (which == 3) store32(archive_bytes + 0x26, UINT_MAX);
            if (which == 4) store32(archive_bytes + 0x1e, UINT_MAX);
            if (which == 5) store32(archive_bytes + 0x2a, 0x104);
            if (which == 6) {
                if (v < 0x200) store32(archive_bytes + archive_table, UINT_MAX);
                else store32(archive_bytes + archive_table + 4, UINT_MAX);
            }
            if (which == 7) {
                if (v < 0x200) store32(archive_bytes + archive_table + fixture_record_offsets[0] + 13, UINT_MAX);
                else archive_bytes[archive_table + 8] ^= 0xff;
            }
            write_fixture(path);
            if (which == 7 && v < 0x200) {
                struct GrfArchive *archive = grf_archive_create(path);
                assert(archive && archive->entry_count == GRF_FIXTURE_COUNT - 1);
                grf_archive_destroy(archive);
            } else assert(!grf_archive_create(path));
            empty_owners();
        }
    }
}

static void deleted_record_skipped(const char *path)
{
    struct GrfArchive *archive;
    struct GrfEntry entry;
    struct Hash key;
    BYTE output[GRF_FIXTURE_PAYLOAD];
    char name[252];
    unsigned long packed;
    make_grf_fixture(0x200);
    memset(table_bytes + fixture_record_offsets[2], 0, 13);
    store32(table_bytes + fixture_record_offsets[2] + 13, UINT_MAX - 0x2d);
    packed = sizeof archive_bytes - archive_table - 8;
    assert(compress(archive_bytes + archive_table + 8, &packed, table_bytes, table_size) == Z_OK);
    store32(archive_bytes + archive_table, (unsigned)packed);
    archive_size = archive_table + 8 + (unsigned)packed;
    write_fixture(path);
    archive = grf_archive_create(path);
    assert(archive && archive->entry_count == GRF_FIXTURE_COUNT - 1);
    fixture_name(2, name); hash_construct(&key, name);
    assert(!grf_archive_find(archive, &key, NULL));
    fixture_name(3, name); hash_construct(&key, name);
    assert(grf_archive_find(archive, &key, &entry) && grf_archive_read(archive, &entry, output, sizeof output));
    grf_archive_destroy(archive); empty_owners();
    puts("GRF records: a record outside the archive is left out and the others stay readable");
}

static void empty_and_bounded_records(const char *path)
{
    struct GrfArchive *archive;
    struct GrfEntry entry;
    struct Hash key;
    BYTE output[GRF_FIXTURE_PAYLOAD + 16];
    char name[252];
    unsigned i;
    make_grf_fixture(0x200); write_fixture(path);
    archive = grf_archive_create(path); assert(archive);
    fixture_name(3, name); hash_construct(&key, name);
    assert(grf_archive_find(archive, &key, &entry));
    entry.size -= 5; memset(output, 0x9b, sizeof output);
    assert(grf_archive_read(archive, &entry, output, entry.size));
    for (i = entry.size; i < sizeof output; ++i) assert(output[i] == 0x9b);
    entry.offset = UINT_MAX; assert(!grf_archive_read(archive, &entry, output, sizeof output));
    grf_archive_destroy(archive); empty_owners();
    make_grf_fixture(0x100);
    store32(archive_bytes + 0x26, 20);
    archive_size = archive_table; write_fixture(path);
    archive = grf_archive_create(path);
    assert(archive && !archive->entries && !archive->entry_count && !archive->scratch);
    assert(!grf_archive_find(archive, &key, NULL));
    grf_archive_destroy(archive); empty_owners();
    grf_archive_destroy(NULL);
}

static void manager_lifetime(const char *first, const char *second)
{
    struct GameFileMgr manager = {0};
    char name[252]; int size; BYTE *bytes;
    unsigned calls, failure, saved_handles, saved_views, saved_allocations;
    make_grf_fixture(0x200); write_fixture(first);
    archive_bytes[0x2e + read32(table_bytes + fixture_record_offsets[3] + 13)] ^= 0x55;
    write_fixture(second); fixture_name(3, name);
    assert(game_file_manager_add_archive(&manager, first));
    saved_handles = live_handles; saved_views = live_views; saved_allocations = outstanding;
    allocation_calls = 0;
    assert(game_file_manager_add_archive(&manager, second)); calls = allocation_calls;
    bytes = game_file_mgr_get_data(&manager, name, &size, 0);
    assert(bytes && size == GRF_FIXTURE_PAYLOAD && bytes[0] == ((3 * 19) ^ 0x55));
    game_file_mgr_release_data(&manager, bytes);
    game_file_manager_clear(&manager); game_file_manager_clear(&manager); empty_owners();
    for (failure = 1; failure <= calls; ++failure) {
        struct GrfArchive *prior;
        assert(game_file_manager_add_archive(&manager, first)); prior = manager.archives[0];
        allocation_calls = 0; fail_at = failure;
        assert(!game_file_manager_add_archive(&manager, second)); fail_at = 0;
        assert(manager.archive_count == 1 && manager.archives[0] == prior);
        assert(live_handles == saved_handles && live_views == saved_views && outstanding == saved_allocations);
        bytes = game_file_mgr_get_data(&manager, name, &size, 0);
        assert(bytes && size == GRF_FIXTURE_PAYLOAD && bytes[0] == 3 * 19);
        game_file_mgr_release_data(&manager, bytes);
        game_file_manager_clear(&manager); empty_owners();
    }
    puts("GRF manager: newest-first lookup, atomic publication, every failure rollback and repeated complete shutdown passed");
}

int main(int argc, char **argv)
{
    char directory[MAX_PATH], first[MAX_PATH], second[MAX_PATH];
    assert(argc == 2 && GetTempPathA(sizeof directory, directory));
    assert(GetTempFileNameA(directory, "grf", 0, first));
    assert(GetTempFileNameA(directory, "grf", 0, second));
    reference_cases(first, argv[1]);
    mapping_failures(first); allocation_and_read_failures(first); malformed_records(first);
    deleted_record_skipped(first);
    empty_and_bounded_records(first);
    manager_lifetime(first, second);
    assert(DeleteFileA(first) && DeleteFileA(second));
    return 0;
}
