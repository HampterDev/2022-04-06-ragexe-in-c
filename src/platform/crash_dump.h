#ifndef PLATFORM_CRASH_DUMP_H
#define PLATFORM_CRASH_DUMP_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum CrashDumpStatus {
    CRASH_DUMP_SAVED,
    CRASH_DUMP_FAILED,
    CRASH_DUMP_TIMED_OUT,
    CRASH_DUMP_UNAVAILABLE
};

struct CrashDumpResult {
    enum CrashDumpStatus status;
    DWORD error;
    char filename[128];
};

int crash_dump_start(void);
int crash_dump_stop(void);

void crash_dump_write(const EXCEPTION_POINTERS *exception, struct CrashDumpResult *result);

#endif
