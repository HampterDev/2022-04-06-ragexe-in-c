#include <windows.h>
#include <wincrypt.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "owned_storage_test_allocator.h"
#include "session/state.h"
#include "core/thread_text.h"
#include "security/gameguard.h"
#include "security/gg_hash_mgr.h"
#include "security/themida.h"

static unsigned handles, views, locks, signature_calls, reports;
static int fail_open, fail_size, fail_map, fail_view, fail_lock, accept_signature;
static LONGLONG size_override = -1;
static HANDLE tracked_open(LPCWSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                           DWORD creation, DWORD flags, HANDLE template_file)
{
    HANDLE file = fail_open ? INVALID_HANDLE_VALUE : CreateFileW(path, access, share, sa, creation, flags, template_file);
    if (file != INVALID_HANDLE_VALUE) ++handles;
    return file;
}
static BOOL tracked_size(HANDLE file, PLARGE_INTEGER size)
{
    BOOL ok = !fail_size && GetFileSizeEx(file, size);
    if (ok && size_override >= 0) size->QuadPart = size_override;
    return ok;
}
static HANDLE tracked_map(HANDLE file, LPSECURITY_ATTRIBUTES sa, DWORD protect,
                          DWORD high, DWORD low, LPCWSTR name)
{
    HANDLE map = fail_map ? NULL : CreateFileMappingW(file, sa, protect, high, low, name);
    if (map) ++handles;
    return map;
}
static LPVOID tracked_view(HANDLE map, DWORD access, DWORD high, DWORD low, SIZE_T size)
{
    void *view = fail_view ? NULL : MapViewOfFile(map, access, high, low, size);
    if (view) ++views;
    return view;
}
static BOOL tracked_unmap(LPCVOID view) { assert(views); --views; return UnmapViewOfFile(view); }
static BOOL tracked_close(HANDLE file) { assert(handles); --handles; return CloseHandle(file); }
static BOOL tracked_lock(LPCRITICAL_SECTION cs, DWORD spin, DWORD flags)
{
    BOOL ok = !fail_lock && InitializeCriticalSectionEx(cs, spin, flags);
    if (ok) ++locks;
    return ok;
}
static void tracked_delete_lock(LPCRITICAL_SECTION cs) { assert(locks); --locks; DeleteCriticalSection(cs); }
static BOOL signature_decision(HCRYPTHASH hash, const BYTE *signature, DWORD size,
                               HCRYPTKEY key, LPCWSTR description, DWORD flags)
{
    BYTE digest[20]; DWORD count = sizeof digest;
    ++signature_calls;
    assert(CryptGetHashParam(hash, HP_HASHVAL, digest, &count, 0) && count == sizeof digest);
    return accept_signature || CryptVerifySignatureW(hash, signature, size, key, description, flags);
}
static SRWLOCK allocation_lock = SRWLOCK_INIT;
static void *synchronized_malloc(size_t size)
{
    void *p;
    AcquireSRWLockExclusive(&allocation_lock);
    p = checked_malloc(size);
    ReleaseSRWLockExclusive(&allocation_lock);
    return p;
}
static void synchronized_free(void *p)
{
    AcquireSRWLockExclusive(&allocation_lock);
    checked_free(p);
    ReleaseSRWLockExclusive(&allocation_lock);
}
static void *synchronized_realloc(void *p, size_t size)
{
    void *next;
    AcquireSRWLockExclusive(&allocation_lock);
    next = checked_malloc(size);
    if (next && p) {
        size_t old = ((struct Allocation *)p - 1)->size;
        memcpy(next, p, old < size ? old : size);
        checked_free(p);
    }
    ReleaseSRWLockExclusive(&allocation_lock);
    return next;
}
#define malloc synchronized_malloc
#define free synchronized_free
#define realloc synchronized_realloc
#define CreateFileW tracked_open
#define GetFileSizeEx tracked_size
#define CreateFileMappingW tracked_map
#define MapViewOfFile tracked_view
#define UnmapViewOfFile tracked_unmap
#define CloseHandle tracked_close
#define InitializeCriticalSectionEx tracked_lock
#define DeleteCriticalSection tracked_delete_lock
#define CryptVerifySignatureW signature_decision
#include "../src/initializers/md5.c"
#include "../src/initializers/gg_hash.c"
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#define gameguard_set_error real_gameguard_set_error
#include "../src/security/gameguard_mgr.c"
#undef gameguard_set_error
#include "../src/security/themida_mgr.c"
#include "../src/security/gg_hash_mgr.c"
#undef malloc
#undef free
#undef realloc
#undef CreateFileW
#undef GetFileSizeEx
#undef CreateFileMappingW
#undef MapViewOfFile
#undef UnmapViewOfFile
#undef CloseHandle
#undef InitializeCriticalSectionEx
#undef DeleteCriticalSection
#undef CryptVerifySignatureW
#include "gg_hash_fixture.h"

#define FILE_AUTH_TRAILER_HEAD 0x32812622u
#define FILE_AUTH_TRAILER_TAIL 0x32812621u

struct Session g_session1;
char *msg_str(int id) { assert(id == 0xef0); return "hash error"; }
const char *thread_text_format(const char *format, ...)
{
    static char message[128]; va_list args;
    va_start(args, format); vsnprintf(message, sizeof message, format, args); va_end(args);
    return message;
}
void gameguard_set_error(struct GameGuard *guard, const char *message)
{
    assert(guard == g_gameGuard && strstr(message, "hash error ( ") == message);
    ++reports;
    real_gameguard_set_error(guard, message);
}
void client_lua_loadfile_hook(const char *name);

static void no_resources(void) { assert(!handles && !views && !outstanding && !locks); }
static unsigned char signed_bytes[2048];
static unsigned signed_size;
static void signed_fixture(void)
{
    static const char name[] = "hash-fixture.dat";
    unsigned trailer[4] = {FILE_AUTH_TRAILER_HEAD, sizeof name - 1, 256, FILE_AUTH_TRAILER_TAIL};
    unsigned data_size = hash_word_count * 4;
    memcpy(signed_bytes, hash_words, data_size);
    memcpy(signed_bytes + data_size, name, sizeof name - 1);
    memset(signed_bytes + data_size + sizeof name - 1, 0, 256);
    signed_size = data_size + sizeof name - 1 + 256 + sizeof trailer;
    memcpy(signed_bytes + signed_size - sizeof trailer, trailer, sizeof trailer);
}
static void write_signed(unsigned size)
{
    FILE *out = fopen("hash-fixture.dat", "wb"); assert(out);
    assert(fwrite(signed_bytes, 1, size, out) == size && !fclose(out));
}
static void signed_loading(void)
{
    struct GGHash hash = {0}; unsigned n;
    wchar_t long_name[MAX_PATH + 2]; char long_ansi[MAX_PATH + 2];
    hash_fixture(3, 16); signed_fixture(); write_signed(signed_size);
    memset(long_ansi, 'a', sizeof long_ansi - 1); long_ansi[sizeof long_ansi - 1] = 0;
    for (n = 0; n < MAX_PATH + 1; ++n) long_name[n] = L'a'; long_name[n] = 0;
    assert(!gg_hash_lookup(&hash, long_name, 1));
    assert(!gg_hash_lookup_data_ansi(&hash, long_ansi, "x", 1));
    assert(!gg_hash_lookup_file_ansi(&hash, long_ansi));
    assert(!gg_hash_lookup_data(&hash, L"name", NULL, 1) && hash.last_error == GG_HASH_NO_DATA);
    assert(!gg_hash_lookup_data(&hash, NULL, "x", 1) && hash.last_error == GG_HASH_NO_FILE);
    gg_hash_clear(&hash); no_resources();
    puts("Security hash: lookup argument checks passed.");
}
static void file_hash_cleanup(void)
{
    struct Md5 md5; unsigned before, n;
    write_signed(0); assert(!md5_hash32_file(&md5, L"hash-fixture.dat")); no_resources();
    write_signed(signed_size);
    for (n = 0; n < 3; ++n) {
        fail_size = n == 0; fail_map = n == 1; fail_view = n == 2;
        assert(!md5_hash32_file(&md5, L"hash-fixture.dat")); no_resources();
    }
    fail_size = fail_map = fail_view = 0;
    before = md5_hash32_file(&md5, L"hash-fixture.dat");
    assert(before == md5_hash32(&md5, signed_bytes, signed_size)); no_resources();
}
static void manager_lifetime(void)
{
    struct GGHashManager *first; unsigned before, data = 19;
    allocation_calls = 0; fail_at = 1;
    assert(!gg_hash_manager_startup() && !g_ggHashMgr); no_resources(); fail_at = 0;
    fail_lock = 1; assert(!gg_hash_manager_startup() && !g_ggHashMgr); no_resources(); fail_lock = 0;
    assert(gg_hash_manager_startup()); first = g_ggHashMgr;
    assert(!gg_hash_manager_startup() && g_ggHashMgr == first);
    assert(!first->hash.count && !gg_hash_manager_is_intact(first));
    before = reports;
    assert(!gg_hash_manager_check_data(first, "empty", &data, sizeof data) && reports == before + 1);
    assert(!gg_hash_manager_check_file(first, "missing.invalid"));
    assert(security_queue_lua_file("MISSING_A") && security_queue_lua_file("missing_b"));
    assert(security_queue_lua_file("missing_a") && g_session1.luaFileNames.size == 2);
    before = reports; security_poll(first);
    assert(reports == before + 2 && !g_session1.luaFileNames.size);
    security_poll(first); assert(reports == before + 2);
    text_array_clear(&g_session1.luaFileNames);
    gg_hash_manager_shutdown(); gg_hash_manager_shutdown(); assert(!g_ggHashMgr); no_resources();
    assert(!gg_hash_manager_is_intact(NULL) && !gg_hash_manager_check_file(NULL, "x")); security_poll(NULL);
    puts("Security hash manager: startup rollback, reporting, queued checks and repeated shutdown passed.");
}
static void gameguard_lifetime(void)
{
    struct GameGuard *first;
    unsigned count;
    allocation_calls = 0; fail_at = 1;
    assert(!gameguard_startup() && !g_gameGuard); no_resources(); fail_at = 0;
    fail_lock = 1; assert(!gameguard_startup() && !g_gameGuard); no_resources(); fail_lock = 0;
    assert(gameguard_startup()); first = g_gameGuard;
    assert(!gameguard_startup() && g_gameGuard == first && !gameguard_has_error(first));
    gameguard_set_window(first); gameguard_set_account(first);
    gameguard_authenticate(first); gameguard_send_auth_answers(first);
    assert(!gameguard_has_error(first) && !first->connect_count);
    gameguard_on_connect(first); gameguard_on_connect(first); assert(first->connect_count == 2);
    first->connect_count = UINT_MAX; gameguard_on_connect(first); assert(!first->connect_count);
    real_gameguard_set_error(first, "the first diagnostic has owned storage");
    assert(gameguard_has_error(first) && !strcmp(owned_text_cstr(&first->error_text), "the first diagnostic has owned storage"));
    count = allocation_calls;
    real_gameguard_set_error(first, "ignored second error"); gameguard_check(first);
    assert(allocation_calls == count && !strcmp(owned_text_cstr(&first->error_text), "the first diagnostic has owned storage"));
    gameguard_shutdown(); gameguard_shutdown(); no_resources();
    assert(gameguard_startup()); gameguard_check(g_gameGuard);
    assert(gameguard_has_error(g_gameGuard) && !g_gameGuard->error_text.size);
    real_gameguard_set_error(g_gameGuard, "later text"); assert(!g_gameGuard->error_text.size);
    gameguard_shutdown(); no_resources();
    assert(gameguard_startup()); allocation_calls = 0; fail_at = 1;
    real_gameguard_set_error(g_gameGuard, "allocation failure");
    assert(gameguard_has_error(g_gameGuard) && !g_gameGuard->error_text.data);
    fail_at = 0; gameguard_shutdown(); no_resources();
    assert(gameguard_startup() && gg_hash_manager_startup());
    assert(!gg_hash_manager_check_data(g_ggHashMgr, "empty", "x", 1));
    assert(!strcmp(owned_text_cstr(&g_gameGuard->error_text), "hash error ( 3 )"));
    gameguard_check(g_gameGuard); assert(!strcmp(owned_text_cstr(&g_gameGuard->error_text), "hash error ( 3 )"));
    gg_hash_manager_shutdown(); gameguard_shutdown(); no_resources();
    count = allocation_calls;
    assert(themida_startup() && !themida_startup()); themida_shutdown(); themida_shutdown();
    assert(themida_startup()); themida_shutdown(); assert(allocation_calls == count); no_resources();
    assert(!gameguard_has_error(NULL));
    puts("Security managers: C startup rollback, first-error ownership, reporting integration and allocation-free Themida lifetime passed.");
}
static DWORD WINAPI queue_producer(void *context)
{
    unsigned i, offset = (unsigned)(uintptr_t)context;
    char name[64];
    for (i = 0; i < 200; ++i) {
        sprintf(name, "MISSING_CONCURRENT_%u", (i + offset) % 10);
        assert(security_queue_lua_file(name));
    }
    return 0;
}
static void queue_lifetime(void)
{
    unsigned n, before, i; HANDLE threads[4];
    struct OwnedText *items; char *text;
    char bytes[] = {'A', (char)0x80, (char)0xff, 'Z', 0};
    assert(!security_queue_lua_file(NULL)); no_resources();
    for (n = 1; n <= 2; ++n) {
        allocation_calls = 0; fail_at = n;
        assert(!security_queue_lua_file("NEW_FILE"));
        assert(!g_session1.luaFileNames.size && !g_session1.luaFileNames.items); no_resources(); fail_at = 0;
    }
    client_lua_loadfile_hook("PATH\\FILE.LUA");
    assert(g_session1.luaFileNames.size == 1 && !strcmp(owned_text_cstr(&g_session1.luaFileNames.items[0]), "path\\file.lua"));
    items = g_session1.luaFileNames.items; text = items[0].data;
    for (n = 1; n <= 2; ++n) {
        allocation_calls = 0; fail_at = n;
        assert(!security_queue_lua_file("ANOTHER_FILE"));
        assert(g_session1.luaFileNames.size == 1 && g_session1.luaFileNames.items == items && items[0].data == text);
        fail_at = 0;
    }
    assert(security_queue_lua_file(text) && g_session1.luaFileNames.size == 1);
    assert(security_queue_lua_file("") && security_queue_lua_file("") && g_session1.luaFileNames.size == 2);
    assert(security_queue_lua_file(bytes));
    for (i = 0; i < sizeof bytes - 1; ++i)
        assert(g_session1.luaFileNames.items[2].data[i] == (char)(tolower)((unsigned char)bytes[i]));
    text_array_clear(&g_session1.luaFileNames); text_array_clear(&g_session1.luaFileNames); no_resources();
    for (n = 0; n < 4; ++n) { threads[n] = CreateThread(NULL, 0, queue_producer, (void *)(uintptr_t)n, 0, NULL); assert(threads[n]); }
    assert(WaitForMultipleObjects(4, threads, TRUE, INFINITE) == WAIT_OBJECT_0);
    for (n = 0; n < 4; ++n) assert(CloseHandle(threads[n]));
    assert(g_session1.luaFileNames.size == 10);
    assert(gg_hash_manager_startup()); before = reports; security_poll(g_ggHashMgr);
    assert(reports == before + 10 && !g_session1.luaFileNames.size);
    for (n = 0; n < 4; ++n) { threads[n] = CreateThread(NULL, 0, queue_producer, (void *)(uintptr_t)n, 0, NULL); assert(threads[n]); }
    while (WaitForMultipleObjects(4, threads, TRUE, 1) == WAIT_TIMEOUT) security_poll(g_ggHashMgr);
    for (n = 0; n < 4; ++n) assert(CloseHandle(threads[n]));
    security_poll(g_ggHashMgr); assert(!g_session1.luaFileNames.size);
    gg_hash_manager_shutdown(); text_array_clear(&g_session1.luaFileNames); no_resources();
    puts("Lua filename queue: real hook, folding, duplicate/alias handling, every append failure and concurrent producers/polling passed.");
}
int main(void)
{
    signed_loading(); file_hash_cleanup(); manager_lifetime();
    gameguard_lifetime(); queue_lifetime();
    assert(!remove("hash-fixture.dat")); no_resources(); return 0;
}
