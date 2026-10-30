#include "core/fatal.h"
#include "core/abstract_method.h"
#include <windows.h>
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const messages[] = {NULL, "", "invalid vector capacity", "%s%n 한글"};

static void abort_observed(int number)
{
    assert(number == SIGABRT);
    ExitProcess(81);
}

static void child(unsigned kind, unsigned observe)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (observe) assert(signal(SIGABRT, abort_observed) != SIG_ERR);
    if (kind == 4) abstract_method_called();
    assert(kind < 4);
    fatal_error(messages[kind]);
}

static void check_child(const char *exe, unsigned kind, unsigned observe)
{
    SECURITY_ATTRIBUTES security = {sizeof security, NULL, TRUE};
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    HANDLE read_end, write_end, input;
    char command[4096], output[256], expected[256];
    DWORD wait, status, count, used = 0;
    int n;

    assert(CreatePipe(&read_end, &write_end, &security, 0));
    assert(SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0));
    input = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        &security, OPEN_EXISTING, 0, NULL);
    assert(input != INVALID_HANDLE_VALUE);
    startup.cb = sizeof startup;
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input;
    startup.hStdOutput = startup.hStdError = write_end;
    n = snprintf(command, sizeof command, "\"%s\" child %u %u", exe, kind, observe);
    assert(n > 0 && (size_t)n < sizeof command);
    assert(CreateProcessA(exe, command, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                          NULL, NULL, &startup, &process));
    CloseHandle(write_end);
    CloseHandle(input);
    wait = WaitForSingleObject(process.hProcess, 10000);
    if (wait != WAIT_OBJECT_0) TerminateProcess(process.hProcess, 98);
    assert(wait == WAIT_OBJECT_0);
    assert(GetExitCodeProcess(process.hProcess, &status));
    assert(status == (observe ? 81u : 3u));
    while (ReadFile(read_end, output + used, sizeof output - 1 - used, &count, NULL)) {
        used += count;
        assert(used < sizeof output - 1 && count);
    }
    assert(GetLastError() == ERROR_BROKEN_PIPE);
    output[used] = 0;
    n = snprintf(expected, sizeof expected, "fatal: %s\r\n",
                 kind == 4 ? "abstract method called" : (messages[kind] ? messages[kind] : "unknown error"));
    assert(n >= 0 && (size_t)n < sizeof expected && !strcmp(output, expected));
    CloseHandle(read_end);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

int main(int argc, char **argv)
{
    char exe[2048];
    DWORD length;
    unsigned kind, observe;
    if (argc == 4 && !strcmp(argv[1], "child"))
        child((unsigned)atoi(argv[2]), (unsigned)atoi(argv[3]));
    assert(argc == 1);
    length = GetModuleFileNameA(NULL, exe, sizeof exe);
    assert(length && length < sizeof exe);
    for (kind = 0; kind < 5; ++kind)
        for (observe = 0; observe < 2; ++observe)
            check_child(exe, kind, observe);
    puts("C failures: literal and Korean diagnostics, null/empty messages, SIGABRT, process termination and abstract-method failure passed");
    return 0;
}
