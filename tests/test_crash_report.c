#include "session/state.h"
#include "diag/crash_report.h"
#include "platform/crash_dump.h"
#include "diag/diag.h"
#include "game/context.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

struct Session g_session1;
unsigned g_launchedEffectIds[32];

DWORD g_renderCheckpointLine = 42;
const char *g_renderCheckpointFile = "C:\\source\\game.c";
static wchar_t directory[MAX_PATH], executable[MAX_PATH], report_path[MAX_PATH];
static char shown[256 * 1024], oversized[4096];
static const char *map_name = "prontera", *job_name = "Swordsman";
static int dump_starts, dump_stops, dump_writes, fail_start, callbacks, other_callbacks;
static int dialogs, nested;
static enum CrashDumpStatus dump_status;
static LPTOP_LEVEL_EXCEPTION_FILTER filter;
static EXCEPTION_POINTERS *expected_exception;

DWORD WINAPI test_report_module_name(HMODULE module, wchar_t *path, DWORD capacity)
{
    size_t length = wcslen(executable);
    (void)module;
    assert(length + 1 < capacity);
    memcpy(path, executable, (length + 1) * sizeof *path);
    return (DWORD)length;
}

int session_get_drawn_job(struct Session *session)
{
    assert(session == &g_session1);
    return 7;
}

const char *session_get_job_name(struct Session *session, unsigned job, int sex)
{
    assert(session == &g_session1 && job == 7 && sex == -1);
    return job_name;
}

const char *game_current_map_name(void) { return map_name; }

int crash_dump_start(void) { ++dump_starts; return !fail_start; }
int crash_dump_stop(void) { ++dump_stops; return 1; }
void crash_dump_write(const EXCEPTION_POINTERS *exception, struct CrashDumpResult *result)
{
    assert(exception == expected_exception);
    ++dump_writes;
    memset(result, 0, sizeof *result);
    result->status = dump_status;
    result->error = ERROR_ACCESS_DENIED;
    strcpy(result->filename, "fixture.dmp");
}

static void callback(void) { assert(dialogs > callbacks); ++callbacks; }
static void other_callback(void) { ++other_callbacks; }
static LONG WINAPI previous(EXCEPTION_POINTERS *exception)
{
    (void)exception;
    assert(!"the previously installed filter must not run during our report");
    return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD report_size(void)
{
    HANDLE file = CreateFileW(report_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    DWORD size;
    assert(file != INVALID_HANDLE_VALUE);
    size = GetFileSize(file, NULL);
    CloseHandle(file);
    return size;
}

void crash_report_show_dialog(const char *text)
{
    size_t length = strlen(text);
    assert(length < sizeof shown);
    memcpy(shown, text, length + 1);
    ++dialogs;
    assert(report_size() > 0);
    if (nested) {
        int before = dump_writes;
        assert(filter(NULL) == EXCEPTION_EXECUTE_HANDLER);
        assert(before == dump_writes && !strcmp(shown, text));
    }
}

static void install(void)
{
    assert(crash_report_install(callback));
    filter = SetUnhandledExceptionFilter(NULL);
    assert(filter && filter != previous);
    assert(SetUnhandledExceptionFilter(filter) == NULL);
    assert(!crash_report_install(other_callback));
}

static void uninstall(void)
{
    crash_report_uninstall();
    assert(SetUnhandledExceptionFilter(previous) == previous);
    crash_report_uninstall();
}

int main(void)
{
    g_session1.playerPosX = 20;
    g_session1.playerPosY = 30;
    wchar_t temporary[MAX_PATH];
    LPTOP_LEVEL_EXCEPTION_FILTER original;
    EXCEPTION_RECORD record = {0};
    EXCEPTION_POINTERS exception = {&record, NULL};
    unsigned i;
    assert(GetTempPathW(MAX_PATH, temporary));
    assert(GetTempFileNameW(temporary, L"hcr", 0, directory));
    assert(DeleteFileW(directory) && CreateDirectoryW(directory, NULL));
    assert(swprintf(executable, MAX_PATH, L"%s\\test.exe", directory) > 0);
    assert(swprintf(report_path, MAX_PATH, L"%s\\crash_report.txt", directory) > 0);
    original = SetUnhandledExceptionFilter(previous);
    fail_start = 1;
    assert(!crash_report_install(callback));
    assert(SetUnhandledExceptionFilter(previous) == previous);
    fail_start = 0;
    record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
    record.ExceptionAddress = (void *)main;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = 1;
    record.ExceptionInformation[1] = 0x12345;
    expected_exception = &exception;
    for (i = 0; i < 32; ++i) g_launchedEffectIds[i] = 0x10000000 + i;
    install();
    nested = 1;
    dump_status = CRASH_DUMP_SAVED;
    assert(filter(&exception) == EXCEPTION_EXECUTE_HANDLER);
    assert(dialogs == 1 && callbacks == 1 && !other_callbacks);
    assert(strstr(shown, "access violation") && strstr(shown, "writing 00012345"));
    assert(strstr(shown, "Swordsman") && strstr(shown, "prontera  cell 20 30"));
    assert(strstr(shown, "1000001F") && strstr(shown, "game.c 42"));
    assert(strstr(shown, "fixture.dmp"));
    assert(filter(&exception) == EXCEPTION_EXECUTE_HANDLER && dialogs == 1);
    uninstall();
    nested = 0;
    expected_exception = NULL;
    g_renderCheckpointFile = NULL;
    job_name = NULL;
    map_name = "";
    for (i = CRASH_DUMP_FAILED; i <= CRASH_DUMP_UNAVAILABLE; ++i) {
        dump_status = (enum CrashDumpStatus)i;
        install();
        assert(filter(NULL) == EXCEPTION_EXECUTE_HANDLER);
        assert(strstr(shown, "(no exception record)") && strstr(shown, "(none) 42"));
        assert(strstr(shown, i == CRASH_DUMP_FAILED ? "failed (Windows error" :
                             i == CRASH_DUMP_TIMED_OUT ? "timed out" : "unavailable"));
        uninstall();
    }
    memset(oversized, 'x', sizeof oversized - 1);
    map_name = oversized;
    {
        LARGE_INTEGER size;
        HANDLE file = CreateFileW(report_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        assert(file != INVALID_HANDLE_VALUE);
        size.QuadPart = 2 * 1024 * 1024 - 17;
        assert(SetFilePointerEx(file, size, NULL, FILE_BEGIN) && SetEndOfFile(file));
        CloseHandle(file);
    }
    install();
    assert(filter(NULL) == EXCEPTION_EXECUTE_HANDLER);
    assert(strstr(shown, "text was dropped above") && strlen(shown) < sizeof shown);
    uninstall();
    assert(report_size() == 2 * 1024 * 1024);
    assert(dump_starts == 6 && dump_stops == 5 && dump_writes == 5);
    assert(DeleteFileW(report_path));
    assert(callbacks == 5 && dialogs == 5 && !other_callbacks);
    assert(RemoveDirectoryW(directory));
    SetUnhandledExceptionFilter(original);
    puts("PASS native crash report: filter ownership, bounded output, game context, reentry");
    return 0;
}
