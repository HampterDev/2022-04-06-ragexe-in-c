#ifndef _WIN32
#include "diag/crash_report.h"
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#if defined(__GLIBC__) || defined(__APPLE__)
#include <execinfo.h>
#define CRASH_BACKTRACE 64
#endif

#define CRASH_REPORT_FILE "crash_report.txt"
#define CRASH_SIGNALS 5

static const int crash_signals[CRASH_SIGNALS] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
static const char *const crash_names[CRASH_SIGNALS] = {
    "SIGSEGV (invalid memory access)", "SIGBUS (bus error)", "SIGILL (illegal instruction)",
    "SIGFPE (arithmetic error)", "SIGABRT (abort)"
};
static struct sigaction previous[CRASH_SIGNALS];
static void (*crash_callback)(void);
static volatile sig_atomic_t in_handler;
static int installed;

static void write_text(int file, const char *text)
{
    size_t length = strlen(text);

    while (length) {
        ssize_t written = write(file, text, length);
        if (written <= 0)
            return;
        text += written;
        length -= (size_t)written;
    }
}

static void write_address(int file, const void *address)
{
    char text[2 + sizeof(uintptr_t) * 2 + 1];
    uintptr_t value = (uintptr_t)address;
    size_t i, digits = sizeof value * 2;

    text[0] = '0';
    text[1] = 'x';
    for (i = 0; i < digits; ++i)
        text[2 + i] = "0123456789abcdef"[(value >> ((digits - 1 - i) * 4)) & 0xf];
    text[2 + digits] = 0;
    write_text(file, text);
}

static void write_report(int file, size_t index, const siginfo_t *info)
{
    write_text(file, "==================== crash report ====================\n");
    write_text(file, "Signal:     ");
    write_text(file, crash_names[index]);
    write_text(file, "\n");
    if (crash_signals[index] != SIGABRT) {
        write_text(file, "Address:    ");
        write_address(file, info->si_addr);
        write_text(file, "\n");
    }
#ifdef CRASH_BACKTRACE
    {
        void *frames[CRASH_BACKTRACE];
        int count = backtrace(frames, CRASH_BACKTRACE);
        write_text(file, "Backtrace:\n");
        backtrace_symbols_fd(frames, count, file);
    }
#endif
    write_text(file, "==================== end of report ====================\n");
}

static void crash_handler(int number, siginfo_t *info, void *context)
{
    size_t index = 0;
    int file;

    (void)context;
    while (index < CRASH_SIGNALS - 1 && crash_signals[index] != number)
        ++index;
    if (!in_handler) {
        in_handler = 1;
        file = open(CRASH_REPORT_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (file >= 0) {
            write_report(file, index, info);
            close(file);
        }
        write_report(STDERR_FILENO, index, info);
        if (crash_callback)
            crash_callback();
    }
    sigaction(number, &previous[index], NULL);
    raise(number);
}

int crash_report_install(void (*on_crash)(void))
{
    struct sigaction action;
    size_t i;

    if (installed)
        return 0;
#ifdef CRASH_BACKTRACE
    {
        void *frame;
        backtrace(&frame, 1);
    }
#endif
    in_handler = 0;
    crash_callback = on_crash;
    memset(&action, 0, sizeof action);
    action.sa_sigaction = crash_handler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    for (i = 0; i < CRASH_SIGNALS; ++i) {
        if (sigaction(crash_signals[i], &action, &previous[i])) {
            while (i--)
                sigaction(crash_signals[i], &previous[i], NULL);
            crash_callback = NULL;
            return 0;
        }
    }
    installed = 1;
    return 1;
}

void crash_report_uninstall(void)
{
    size_t i;

    if (!installed)
        return;
    for (i = 0; i < CRASH_SIGNALS; ++i)
        sigaction(crash_signals[i], &previous[i], NULL);
    installed = 0;
    crash_callback = NULL;
}
#endif
