#include "platform/crash_dump.h"
#include <dbghelp.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

typedef BOOL (WINAPI *MiniDumpWriter)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
                                      PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);

static MiniDumpWriter minidump_writer;
static HANDLE request_event, done_event, worker;
static LONG stopping, requested;
static DWORD fault_thread;
static EXCEPTION_RECORD fault;
static CONTEXT context;
static EXCEPTION_POINTERS pointers;
static int have_exception;
static struct CrashDumpResult completed;

static void close_worker(void)
{
    if (worker) CloseHandle(worker);
    if (done_event) CloseHandle(done_event);
    if (request_event) CloseHandle(request_event);
    worker = done_event = request_event = NULL;
}

static HANDLE create_dump(wchar_t *path, size_t capacity)
{
    SYSTEMTIME time;
    wchar_t *name;
    DWORD length = GetModuleFileNameW(NULL, path, (DWORD)capacity);
    unsigned attempt;
    if (!length || length >= capacity) {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return INVALID_HANDLE_VALUE;
    }
    name = wcsrchr(path, L'\\');
    if (!name) {
        SetLastError(ERROR_BAD_PATHNAME);
        return INVALID_HANDLE_VALUE;
    }
    ++name;
    GetLocalTime(&time);
    for (attempt = 0; attempt < 100; ++attempt) {
        HANDLE file;
        int n = snprintf(completed.filename, sizeof completed.filename,
            "crash-%04u%02u%02u-%02u%02u%02u-%lu-%lu-%u.dmp",
            time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
            GetCurrentProcessId(), fault_thread, attempt);
        size_t i;
        if (n < 0 || (size_t)n >= sizeof completed.filename ||
            (size_t)(name - path) + (size_t)n + 1 > capacity) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return INVALID_HANDLE_VALUE;
        }
        for (i = 0; i <= (size_t)n; ++i)
            name[i] = (wchar_t)(unsigned char)completed.filename[i];
        file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE || GetLastError() != ERROR_FILE_EXISTS)
            return file;
    }
    SetLastError(ERROR_FILE_EXISTS);
    return INVALID_HANDLE_VALUE;
}

static DWORD WINAPI write_dump(void *unused)
{
    wchar_t path[MAX_PATH + 128];
    (void)unused;
    while (WaitForSingleObject(request_event, INFINITE) == WAIT_OBJECT_0) {
        HANDLE file;
        MINIDUMP_EXCEPTION_INFORMATION info;
        if (InterlockedCompareExchange(&stopping, 0, 0))
            break;
        memset(&completed, 0, sizeof completed);
        completed.status = CRASH_DUMP_FAILED;
        if (!minidump_writer) {
            completed.error = ERROR_PROC_NOT_FOUND;
        } else if ((file = create_dump(path, sizeof path / sizeof path[0])) == INVALID_HANDLE_VALUE) {
            completed.error = GetLastError();
        } else {
            info.ThreadId = fault_thread;
            info.ExceptionPointers = &pointers;
            info.ClientPointers = FALSE;
            if (minidump_writer(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal,
                                have_exception ? &info : NULL, NULL, NULL)) {
                completed.status = CRASH_DUMP_SAVED;
            } else {
                completed.error = GetLastError();
            }
            CloseHandle(file);
            if (completed.status != CRASH_DUMP_SAVED)
                DeleteFileW(path);
        }
        SetEvent(done_event);
    }
    return 0;
}

static MiniDumpWriter find_minidump_writer(void)
{
    HMODULE dbghelp = LoadLibraryExW(L"dbghelp.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    return dbghelp ? (MiniDumpWriter)GetProcAddress(dbghelp, "MiniDumpWriteDump") : NULL;
}

int crash_dump_start(void)
{
    if (worker) {
        if (WaitForSingleObject(worker, 0) != WAIT_OBJECT_0)
            return 0;
        close_worker();
    }
    minidump_writer = find_minidump_writer();
    stopping = requested = 0;
    request_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    done_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (request_event && done_event)
        worker = CreateThread(NULL, 0, write_dump, NULL, 0, NULL);
    if (!worker) {
        close_worker();
        return 0;
    }
    return 1;
}

int crash_dump_stop(void)
{
    if (!worker)
        return 1;
    InterlockedExchange(&stopping, 1);
    SetEvent(request_event);
    if (WaitForSingleObject(worker, 5000) != WAIT_OBJECT_0)
        return 0;
    close_worker();
    return 1;
}

void crash_dump_write(const EXCEPTION_POINTERS *exception, struct CrashDumpResult *result)
{
    DWORD wait;
    memset(result, 0, sizeof *result);
    result->status = CRASH_DUMP_UNAVAILABLE;
    if (!worker || InterlockedCompareExchange(&requested, 1, 0))
        return;
    fault_thread = GetCurrentThreadId();
    have_exception = exception && exception->ExceptionRecord && exception->ContextRecord;
    if (have_exception) {
        fault = *exception->ExceptionRecord;
        fault.ExceptionRecord = NULL;
        context = *exception->ContextRecord;
        pointers.ExceptionRecord = &fault;
        pointers.ContextRecord = &context;
    }
    if (!SetEvent(request_event)) {
        result->status = CRASH_DUMP_FAILED;
        result->error = GetLastError();
        return;
    }
    wait = WaitForSingleObject(done_event, 15000);
    if (wait == WAIT_OBJECT_0) {
        *result = completed;
    } else if (wait == WAIT_TIMEOUT) {
        result->status = CRASH_DUMP_TIMED_OUT;
    } else {
        result->status = CRASH_DUMP_FAILED;
        result->error = GetLastError();
    }
}
