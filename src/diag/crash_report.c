#include "session/state.h"
#include "diag/diag.h"
#include "diag/crash_report.h"
#include "platform/crash_dump.h"
#include "game/context.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include <tlhelp32.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define REPORT_CAP (256 * 1024)
#define FILE_CAP   (2 * 1024 * 1024)

static char report[REPORT_CAP];
static DWORD wp, flushed;
static int truncated;
static wchar_t path[MAX_PATH + 32];
static DWORD start_tick, main_tid;
static LONG in_handler;
static int installed;
static void (*crash_callback)(void);
static LPTOP_LEVEL_EXCEPTION_FILTER previous_filter;
static BYTE *image_base;
static DWORD image_size;

DWORD image_timestamp(void)
{
    const BYTE *base = (const BYTE *)GetModuleHandleA(NULL);
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    const IMAGE_NT_HEADERS *nt;
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;
    nt = (const IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return 0;
    return nt->FileHeader.TimeDateStamp;
}

static void find_image(void)
{
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS *nt;
    image_base = (BYTE *)GetModuleHandleA(NULL);
    image_size = 0;
    if (!image_base)
        return;
    dos = (const IMAGE_DOS_HEADER *)image_base;
    nt = (const IMAGE_NT_HEADERS *)(image_base + dos->e_lfanew);
    image_size = nt->OptionalHeader.SizeOfImage;
}

static int readable(const void *p, DWORD n)
{
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = (uintptr_t)p, end;
    if (!p || n > UINTPTR_MAX - address)
        return 0;
    end = address + n;
    while (address < end) {
        uintptr_t next;
        if (VirtualQuery((const void *)address, &mbi, sizeof(mbi)) != sizeof(mbi))
            return 0;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) || !(mbi.Protect & 0xee))
            return 0;
        next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        if (next <= address)
            return 0;
        address = next;
    }
    return 1;
}

static void put_raw(const char *s, DWORD n)
{
    DWORD i;
    if (n > REPORT_CAP - 128 - wp) {
        truncated = 1;
        return;
    }
    for (i = 0; i < n; i++)
        report[wp + i] = s[i];
    wp += n;
    report[wp] = 0;
}

static void put(const char *s)
{
    put_raw(s, (DWORD)strlen(s));
}

static void put_line(const char *s)
{
    put(s);
    put("\r\n");
}

static void put_fmt(const char *fmt, ...)
{
    char b[1024];
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    if (n < 0) {
        truncated = 1;
        return;
    }
    if ((size_t)n >= sizeof b) {
        truncated = 1;
        n = sizeof b - 1;
    }
    put_raw(b, (DWORD)n);
}

#define MODULE_LIMIT 512
static struct {
    DWORD base, size;
    char name[MAX_PATH * 3];
} modules[MODULE_LIMIT];
static unsigned module_count;
static DWORD module_error;

static void snapshot_modules(void)
{
    HANDLE snapshot;
    MODULEENTRY32W entry;
    unsigned attempts = 0;
    module_count = 0;
    module_error = 0;
    do {
        snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        if (snapshot != INVALID_HANDLE_VALUE)
            break;
        module_error = GetLastError();
    } while (module_error == ERROR_BAD_LENGTH && ++attempts < 3);
    if (snapshot == INVALID_HANDLE_VALUE)
        return;
    entry.dwSize = sizeof(entry);
    if (!Module32FirstW(snapshot, &entry)) {
        module_error = GetLastError();
        CloseHandle(snapshot);
        return;
    }
    do {
        unsigned i = module_count++;
        modules[i].base = (DWORD)(UINT_PTR)entry.modBaseAddr;
        modules[i].size = entry.modBaseSize;
        if (!WideCharToMultiByte(CP_UTF8, 0, entry.szModule, -1, modules[i].name,
                                 sizeof(modules[i].name), NULL, NULL))
            strcpy(modules[i].name, "(unknown)");
    } while (module_count < MODULE_LIMIT && Module32NextW(snapshot, &entry));
    module_error = 0;
    CloseHandle(snapshot);
}

static void put_module_tag(DWORD a)
{
    unsigned i;
    for (i = 0; i < module_count; ++i) {
        if (modules[i].base == (DWORD)(UINT_PTR)image_base)
            continue;
        if (a >= modules[i].base && a - modules[i].base < modules[i].size) {
            put_fmt(" [%s+%08X]", modules[i].name, a - modules[i].base);
            return;
        }
    }
}

static void put_addr(DWORD a)
{
    put_fmt("%08X", a);
    if (image_size && a >= (DWORD)image_base && a - (DWORD)image_base < image_size)
        put_fmt(" [exe+%08X]", a - (DWORD)image_base);
    else
        put_module_tag(a);
}

static void make_path(void)
{
    wchar_t *name;
    DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!n || n >= MAX_PATH || !(name = wcsrchr(path, L'\\'))) {
        path[0] = 0;
        return;
    }
    wcscpy(name + 1, L"crash_report.txt");
}

static void flush(void)
{
    HANDLE file;
    LARGE_INTEGER size;
    DWORD count, wrote;
    if (wp <= flushed || !path[0])
        return;
    file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;
    if (GetFileSizeEx(file, &size) && size.QuadPart < FILE_CAP) {
        count = wp - flushed;
        if (count > FILE_CAP - (DWORD)size.QuadPart)
            count = FILE_CAP - (DWORD)size.QuadPart;
        if (WriteFile(file, report + flushed, count, &wrote, NULL))
            flushed += wrote;
    }
    CloseHandle(file);
}

static const char *code_name(DWORD code)
{
    switch (code) {
    case 0xc0000005: return "access violation";
    case 0xc0000006: return "in-page error";
    case 0xc000001d: return "illegal instruction";
    case 0xc000008c: return "array bounds exceeded";
    case 0xc0000094: return "integer divide by zero";
    case 0xc0000095: return "integer overflow";
    case 0xc00000fd: return "stack overflow";
    case 0xc0000374: return "heap corruption";
    case 0xc0000409: return "fail fast / security check";
    case 0xe06d7363: return "C++ exception";
    case 0x80000003: return "breakpoint";
    case 0x80000004: return "single step";
    default:         return "";
    }
}

static void put_exception(const EXCEPTION_RECORD *er)
{
    DWORD i;
    int depth = 0;
    while (er && readable(er, sizeof(*er)) && depth < 4) {
        put_fmt("%s%08X %s  flags %08X  params %u", depth ? "Nested:     " : "Exception:  ", er->ExceptionCode,
                code_name(er->ExceptionCode), er->ExceptionFlags, er->NumberParameters);
        for (i = 0; i < er->NumberParameters && i < 4; i++)
            put_fmt("  [%u] %08X", i, er->ExceptionInformation[i]);
        put("\r\n");
        put("At:         ");
        put_addr((DWORD)er->ExceptionAddress);
        put("\r\n");
        if ((er->ExceptionCode == 0xc0000005 || er->ExceptionCode == 0xc0000006) && er->NumberParameters >= 2) {
            DWORD kind = er->ExceptionInformation[0];
            put_fmt("Access:     %s %08X", kind == 0 ? "reading" : kind == 1 ? "writing" : kind == 8 ? "executing" : "touching",
                    er->ExceptionInformation[1]);
            if (er->ExceptionInformation[1] < 0x10000)
                put("  (a null pointer plus a field offset)");
            put("\r\n");
        }
        er = er->ExceptionRecord;
        depth++;
    }
}

static void put_memory(void)
{
    MEMORYSTATUSEX ms;
    MEMORY_BASIC_INFORMATION mbi;
    uint64_t priv = 0, image = 0, mapped = 0, reserved = 0, freeTotal = 0;
    DWORD a = 0, largest = 0, freeBlocks = 0, bigBlocks = 0, regions = 0;
    int capped = 0;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        put_fmt("Memory:     load %u%%  avail phys %u of %u MB  avail VA %u of %u MB\r\n", ms.dwMemoryLoad,
                (DWORD)(ms.ullAvailPhys >> 20), (DWORD)(ms.ullTotalPhys >> 20), (DWORD)(ms.ullAvailVirtual >> 20),
                (DWORD)(ms.ullTotalVirtual >> 20));
    } else {
        MEMORYSTATUS m;
        m.dwLength = sizeof(m);
        GlobalMemoryStatus(&m);
        put_fmt("Memory:     load %u%%  avail phys %u MB  avail VA %u MB (saturating figures)\r\n", m.dwMemoryLoad,
                m.dwAvailPhys >> 20, m.dwAvailVirtual >> 20);
    }
    for (;;) {
        if (regions >= 0x10000) {
            capped = 1;
            break;
        }
        if (VirtualQuery((const void *)a, &mbi, sizeof(mbi)) != sizeof(mbi))
            break;
        regions++;
        if (mbi.State == MEM_FREE) {
            freeTotal += mbi.RegionSize;
            freeBlocks++;
            if (mbi.RegionSize >= 0x100000)
                bigBlocks++;
            if (mbi.RegionSize > largest)
                largest = mbi.RegionSize;
        } else if (mbi.State == MEM_RESERVE) {
            reserved += mbi.RegionSize;
        } else if (mbi.Type == MEM_IMAGE) {
            image += mbi.RegionSize;
        } else if (mbi.Type == MEM_MAPPED) {
            mapped += mbi.RegionSize;
        } else {
            priv += mbi.RegionSize;
        }
        if ((DWORD)mbi.BaseAddress + mbi.RegionSize < a || (DWORD)mbi.BaseAddress + mbi.RegionSize >= 0xffff0000)
            break;
        a = (DWORD)mbi.BaseAddress + mbi.RegionSize;
    }
    put_fmt("VA map:     commit private %u MB  image %u MB  mapped %u MB  reserved %u MB\r\n", (DWORD)(priv >> 20),
            (DWORD)(image >> 20), (DWORD)(mapped >> 20), (DWORD)(reserved >> 20));
    put_fmt("            free %u MB in %u block(s), %u of them 1 MB or more, largest %08X, over %u region(s)%s\r\n",
            (DWORD)(freeTotal >> 20), freeBlocks, bigBlocks, largest, regions, capped ? " (the walk was cut short)" : "");
}

static void put_modules(void)
{
    unsigned i;
    put_line("Modules:");
    if (module_error) {
        put_fmt("            (module snapshot failed: Windows error %u)\r\n", module_error);
        return;
    }
    for (i = 0; i < module_count; ++i)
        put_fmt("            %08X  %08X  %s\r\n", modules[i].base, modules[i].size, modules[i].name);
    if (module_count == MODULE_LIMIT)
        put_line("            (512 entries: the list was cut short)");
}

static void put_game_context(void)
{
    const char *checkpoint = g_renderCheckpointFile;
    const char *basename;
    const char *job = session_get_job_name(&g_session1, session_get_drawn_job(&g_session1), -1);
    unsigned i;
    put_fmt("Job:        %s\r\n", job ? job : "(unknown)");
    put_fmt("Map:        %s  cell %d %d\r\n", game_current_map_name(), g_session1.playerPosX, g_session1.playerPosY);
    put_line("Effects:    (recent launch ids)");
    for (i = 0; i < 32; ++i)
        put_fmt("%08X%s", g_launchedEffectIds[i], i % 8 == 7 ? "\r\n" : " ");
    if (!checkpoint)
        checkpoint = "(none)";
    basename = strrchr(checkpoint, '\\');
    if (basename) checkpoint = basename + 1;
    basename = strrchr(checkpoint, '/');
    if (basename) checkpoint = basename + 1;
    put_fmt("Checkpoint: %s %lu\r\n", checkpoint, g_renderCheckpointLine);
}

static void put_dump(const EXCEPTION_POINTERS *exception)
{
    struct CrashDumpResult result;
    crash_dump_write(exception, &result);
    switch (result.status) {
    case CRASH_DUMP_SAVED:
        put_fmt("Minidump:   %s (beside the executable)\r\n", result.filename);
        break;
    case CRASH_DUMP_FAILED:
        put_fmt("Minidump:   failed (Windows error %08lX)\r\n", result.error);
        break;
    case CRASH_DUMP_TIMED_OUT:
        put_line("Minidump:   timed out; the writer may still finish before exit");
        break;
    default:
        put_line("Minidump:   unavailable");
        break;
    }
}

static LONG WINAPI crash_filter(EXCEPTION_POINTERS *exception)
{
    SYSTEMTIME time;
    DWORD tid = GetCurrentThreadId();
    if (InterlockedExchange(&in_handler, 1))
        return EXCEPTION_EXECUTE_HANDLER;
    find_image();
    snapshot_modules();
    put_line("==================== crash report ====================");
    put_fmt("Build:      image %08X  (crash_report.c compiled %s %s)\r\n",
            image_timestamp(), __DATE__, __TIME__);
    put_fmt("Image:      base %08X  size %08X\r\n", (DWORD)image_base, image_size);
    GetLocalTime(&time);
    put_fmt("Time:       %04u-%02u-%02u %02u:%02u:%02u  uptime %lu s\r\n",
            time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
            (GetTickCount() - start_tick) / 1000);
    put_fmt("Process:    pid %lu  tid %lu%s  main thread %lu  debugger %s\r\n",
            GetCurrentProcessId(), tid, tid == main_tid ? " (the main thread)" : "",
            main_tid, IsDebuggerPresent() ? "present" : "absent");
    if (exception && exception->ExceptionRecord)
        put_exception(exception->ExceptionRecord);
    else
        put_line("Exception:  (no exception record)");
    flush();
    put_dump(exception);
    flush();
    put_game_context();
    put_memory();
    put_modules();
    if (truncated) {
        static const char notice[] = "\r\n*** the report buffer is full: text was dropped above ***\r\n";
        memcpy(report + wp, notice, sizeof notice);
        wp += sizeof notice - 1;
    } else {
        put_line("==================== end of report ====================");
    }
    flush();
    crash_report_show_dialog(report);
    if (crash_callback)
        crash_callback();
    return EXCEPTION_EXECUTE_HANDLER;
}

int crash_report_install(void (*on_crash)(void))
{
    if (installed)
        return 0;
    if (!crash_dump_start())
        return 0;
    start_tick = GetTickCount();
    main_tid = GetCurrentThreadId();
    wp = flushed = 0;
    report[0] = 0;
    truncated = 0;
    InterlockedExchange(&in_handler, 0);
    make_path();
    crash_callback = on_crash;
    previous_filter = SetUnhandledExceptionFilter(crash_filter);
    installed = 1;
    return 1;
}

void crash_report_uninstall(void)
{
    if (!installed)
        return;
    SetUnhandledExceptionFilter(previous_filter);
    previous_filter = NULL;
    installed = 0;
    crash_callback = NULL;
    crash_dump_stop();
}
