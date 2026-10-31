#define CRASH_DUMP_TEST_DRIVER
#include "crash_dump_fixture.h"
#include "platform/crash_dump.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static wchar_t directory[MAX_PATH], executable[MAX_PATH], saved_path[MAX_PATH + 128];
static HANDLE owned[3], entered, release_writer;
static int event_calls, fail_event, fail_thread, writer_mode, force_timeout, missing_writer;
static LONG writer_calls;
static DWORD caller_thread, writer_thread;
static const EXCEPTION_RECORD *original_record;
static const CONTEXT *original_context;
static EXCEPTION_RECORD expected_record;
static CONTEXT expected_context;

void WINAPI test_dump_time(SYSTEMTIME *time)
{
    memset(time, 0, sizeof *time);
    time->wYear = 2026;
    time->wMonth = time->wDay = 1;
}

DWORD WINAPI test_dump_module_name(HMODULE module, wchar_t *path, DWORD capacity)
{
    size_t length = wcslen(executable);
    (void)module;
    assert(length + 1 < capacity);
    memcpy(path, executable, (length + 1) * sizeof *path);
    return (DWORD)length;
}

HANDLE WINAPI test_dump_event(SECURITY_ATTRIBUTES *attributes, BOOL manual, BOOL initial, const wchar_t *name)
{
    unsigned slot = (unsigned)event_calls++;
    assert(slot < 2);
    if ((int)slot + 1 == fail_event)
        return NULL;
    owned[slot] = CreateEventW(attributes, manual, initial, name);
    return owned[slot];
}

HANDLE WINAPI test_dump_thread(SECURITY_ATTRIBUTES *attributes, SIZE_T stack, LPTHREAD_START_ROUTINE start,
                              void *parameter, DWORD flags, DWORD *id)
{
    if (fail_thread)
        return NULL;
    owned[2] = CreateThread(attributes, stack, start, parameter, flags, id);
    return owned[2];
}

DWORD WINAPI test_dump_wait(HANDLE object, DWORD timeout)
{
    if (force_timeout && (timeout == 15000 || timeout == 5000))
        return WaitForSingleObject(object, 0);
    return WaitForSingleObject(object, timeout);
}

BOOL WINAPI test_dump_close(HANDLE object)
{
    unsigned i;
    for (i = 0; i < 3; ++i)
        if (owned[i] == object) owned[i] = NULL;
    return CloseHandle(object);
}

HMODULE WINAPI test_dump_library(const wchar_t *name, HANDLE file, DWORD flags)
{
    assert(!wcscmp(name, L"dbghelp.dll") && !file && flags == LOAD_LIBRARY_SEARCH_SYSTEM32);
    return GetModuleHandleW(NULL);
}

FARPROC WINAPI test_dump_proc(HMODULE module, const char *name)
{
    assert(module == GetModuleHandleW(NULL) && !strcmp(name, "MiniDumpWriteDump"));
    return missing_writer ? NULL : (FARPROC)test_dump_write;
}

BOOL WINAPI test_dump_write(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE type,
                           MINIDUMP_EXCEPTION_INFORMATION *exception,
                           MINIDUMP_USER_STREAM_INFORMATION *streams,
                           MINIDUMP_CALLBACK_INFORMATION *callback)
{
    InterlockedIncrement(&writer_calls);
    writer_thread = GetCurrentThreadId();
    assert(writer_thread != caller_thread);
    assert(type == MiniDumpNormal && !streams && !callback);
    assert(process == GetCurrentProcess() && pid == GetCurrentProcessId());
    SetEvent(entered);
    assert(WaitForSingleObject(release_writer, 5000) == WAIT_OBJECT_0);
    if (original_record) {
        assert(exception && exception->ThreadId == caller_thread && !exception->ClientPointers);
        assert(exception->ExceptionPointers->ExceptionRecord != original_record);
        assert(exception->ExceptionPointers->ContextRecord != original_context);
        assert(!memcmp(exception->ExceptionPointers->ExceptionRecord, &expected_record, sizeof expected_record));
        assert(!memcmp(exception->ExceptionPointers->ContextRecord, &expected_context, sizeof expected_context));
    } else {
        assert(!exception);
    }
    if (writer_mode == 1) {
        SetLastError(ERROR_WRITE_FAULT);
        return FALSE;
    }
    if (writer_mode == 2)
        return MiniDumpWriteDump(process, pid, file, type, exception, streams, callback);
    return TRUE;
}

static void reset(void)
{
    assert(!owned[0] && !owned[1] && !owned[2]);
    event_calls = fail_event = fail_thread = writer_mode = force_timeout = 0;
    writer_calls = 0;
    ResetEvent(entered);
    SetEvent(release_writer);
}

static void path_for(const char *name)
{
    wchar_t wide[128];
    assert(MultiByteToWideChar(CP_ACP, 0, name, -1, wide, 128));
    assert(swprintf(saved_path, MAX_PATH + 128, L"%s\\%s", directory, wide) > 0);
}

static void verify_dump(void)
{
    HANDLE file = CreateFileW(saved_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE mapping;
    void *base, *stream;
    MINIDUMP_DIRECTORY *entry;
    MINIDUMP_EXCEPTION_STREAM *exception;
    MINIDUMP_THREAD_LIST *threads;
    MINIDUMP_MODULE_LIST *modules;
    ULONG size;
    DWORD file_size, i;
    int found = 0;
    assert(file != INVALID_HANDLE_VALUE);
    file_size = GetFileSize(file, NULL);
    mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    assert(mapping);
    base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    assert(base);
    assert(MiniDumpReadDumpStream(base, ExceptionStream, &entry, &stream, &size));
    exception = stream;
    assert(size >= sizeof *exception);
    assert(exception->ThreadId == caller_thread);
    assert(exception->ExceptionRecord.ExceptionCode == expected_record.ExceptionCode);
    assert(exception->ExceptionRecord.ExceptionAddress == (ULONG64)(UINT_PTR)expected_record.ExceptionAddress);
    assert(exception->ThreadContext.DataSize == sizeof expected_context);
    assert(exception->ThreadContext.Rva + sizeof expected_context <= file_size);
    assert(!memcmp((char *)base + exception->ThreadContext.Rva, &expected_context, sizeof expected_context));
    assert(MiniDumpReadDumpStream(base, ThreadListStream, &entry, &stream, &size));
    threads = stream;
    for (i = 0; i < threads->NumberOfThreads; ++i)
        if (threads->Threads[i].ThreadId == caller_thread) found = 1;
    assert(found);
    assert(MiniDumpReadDumpStream(base, ModuleListStream, &entry, &stream, &size));
    modules = stream;
    assert(modules->NumberOfModules > 0);
    UnmapViewOfFile(base);
    CloseHandle(mapping);
    CloseHandle(file);
}

int main(void)
{
    wchar_t temporary[MAX_PATH], earlier_path[MAX_PATH + 128];
    struct CrashDumpResult result, second;
    EXCEPTION_RECORD record = {0};
    CONTEXT context = {0};
    EXCEPTION_POINTERS exception = {&record, &context};
    unsigned i;
    assert(GetTempPathW(MAX_PATH, temporary));
    assert(GetTempFileNameW(temporary, L"hcd", 0, directory));
    assert(DeleteFileW(directory));
    assert(CreateDirectoryW(directory, NULL));
    assert(swprintf(executable, MAX_PATH, L"%s\\test.exe", directory) > 0);
    entered = CreateEventW(NULL, TRUE, FALSE, NULL);
    release_writer = CreateEventW(NULL, TRUE, TRUE, NULL);
    assert(entered && release_writer);
    caller_thread = GetCurrentThreadId();
    record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
    record.ExceptionAddress = (void *)main;
    record.ExceptionRecord = &record;
    record.NumberParameters = 2;
    record.ExceptionInformation[1] = 0x1234;
    RtlCaptureContext(&context);
    expected_record = record;
    expected_record.ExceptionRecord = NULL;
    expected_context = context;
    original_record = &record;
    original_context = &context;
    crash_dump_write(&exception, &result);
    assert(result.status == CRASH_DUMP_UNAVAILABLE);
    for (i = 1; i <= 3; ++i) {
        reset();
        if (i <= 2) fail_event = (int)i;
        else fail_thread = 1;
        assert(!crash_dump_start());
        assert(crash_dump_stop());
        assert(!owned[0] && !owned[1] && !owned[2]);
    }
    reset();
    assert(crash_dump_start());
    assert(!crash_dump_start());
    crash_dump_write(&exception, &result);
    assert(result.status == CRASH_DUMP_SAVED && writer_calls == 1);
    path_for(result.filename);
    wcscpy(earlier_path, saved_path);
    crash_dump_write(&exception, &second);
    assert(second.status == CRASH_DUMP_UNAVAILABLE && writer_calls == 1);
    assert(crash_dump_stop());
    reset();
    writer_mode = 1;
    assert(crash_dump_start());
    crash_dump_write(&exception, &result);
    assert(result.status == CRASH_DUMP_FAILED && result.error == ERROR_WRITE_FAULT);
    path_for(result.filename);
    assert(wcscmp(saved_path, earlier_path));
    assert(GetFileAttributesW(saved_path) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(earlier_path) != INVALID_FILE_ATTRIBUTES);
    assert(crash_dump_stop());
    assert(DeleteFileW(earlier_path));
    reset();
    missing_writer = 1;
    assert(crash_dump_start());
    crash_dump_write(&exception, &result);
    assert(result.status == CRASH_DUMP_FAILED && result.error == ERROR_PROC_NOT_FOUND && !writer_calls);
    assert(!result.filename[0]);
    assert(crash_dump_stop());
    missing_writer = 0;
    reset();
    writer_mode = 2;
    assert(crash_dump_start());
    crash_dump_write(&exception, &result);
    assert(result.status == CRASH_DUMP_SAVED);
    path_for(result.filename);
    verify_dump();
    assert(DeleteFileW(saved_path));
    assert(crash_dump_stop());
    reset();
    force_timeout = 1;
    ResetEvent(release_writer);
    assert(crash_dump_start());
    crash_dump_write(&exception, &result);
    assert(result.status == CRASH_DUMP_TIMED_OUT);
    assert(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0);
    memset(&record, 0x55, sizeof record);
    memset(&context, 0x55, sizeof context);
    assert(!crash_dump_stop());
    assert(!crash_dump_start());
    assert(owned[0] && owned[1] && owned[2]);
    SetEvent(release_writer);
    assert(WaitForSingleObject(owned[2], 5000) == WAIT_OBJECT_0);
    force_timeout = 0;
    assert(crash_dump_stop());
    reset();
    original_record = NULL;
    assert(crash_dump_start());
    crash_dump_write(NULL, &result);
    assert(result.status == CRASH_DUMP_SAVED);
    path_for(result.filename);
    assert(DeleteFileW(saved_path));
    assert(crash_dump_stop());
    {
        WIN32_FIND_DATAW data;
        HANDLE search;
        assert(swprintf(saved_path, MAX_PATH + 128, L"%s\\*.dmp", directory) > 0);
        search = FindFirstFileW(saved_path, &data);
        assert(search != INVALID_HANDLE_VALUE);
        assert(swprintf(saved_path, MAX_PATH + 128, L"%s\\%s", directory, data.cFileName) > 0);
        assert(DeleteFileW(saved_path));
        assert(!FindNextFileW(search, &data));
        FindClose(search);
    }
    assert(RemoveDirectoryW(directory));
    CloseHandle(entered);
    CloseHandle(release_writer);
    puts("PASS native crash dumps: lifecycle, owned snapshots, timeout, failure cleanup, missing writer, SDK streams");
    return 0;
}
