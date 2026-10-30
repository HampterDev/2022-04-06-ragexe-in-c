#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/thread_text.h"

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "thread text check failed at line %d: %s\n", __LINE__, #condition);    \
            exit(2);                                                                               \
        }                                                                                          \
    } while (0)

static FILE *trace;
static uint32_t digest = 2166136261u;
static unsigned cases;

static void record(const char *text)
{
    size_t i, length = strlen(text) + 1;
    CHECK(length <= THREAD_TEXT_CAPACITY);
    for (i = 0; i < length; ++i)
        digest = (digest ^ (unsigned char)text[i]) * 16777619u;
    if (trace)
        CHECK(fwrite(text, 1, length, trace) == length);
    ++cases;
}

static void check_formats(void)
{
    static const int values[] = {INT_MIN, -65536, -1, 0, 1, 65535, INT_MAX};
    static const char *patterns[] = {"Ab%123<iteml>42</iteml>", "한글 메시지%"};
    char input[THREAD_TEXT_CAPACITY];
    size_t p, length, i;
    CHECK(strlen("한") == 2);
    for (p = 0; p < sizeof patterns / sizeof *patterns; ++p)
        for (length = 0; length < sizeof input; ++length) {
            const char *result;
            for (i = 0; i < length; ++i)
                input[i] = patterns[p][i % strlen(patterns[p])];
            input[length] = 0;
            result = thread_text_format("%s", input);
            CHECK(strcmp(result, input) == 0);
            record(result);
        }
    for (i = 0; i < sizeof values / sizeof *values; ++i) {
        record(thread_text_format("메시지(%d)", values[i]));
        record(thread_text_format("%s ( %d )", "error", values[i]));
        record(thread_text_format("%d/%d", values[i], values[i]));
        record(thread_text_format("+%d", values[i]));
        record(thread_text_format("PetEgg Item %d not found or not petegg", values[i]));
        record(thread_text_format("%u:%08x:%#o", (unsigned)values[i], (unsigned)values[i],
                                  (unsigned)values[i]));
        record(thread_text_format("%*.*s", 20, 9, "가나다라마바사"));
        record(thread_text_format("%I64d/%lld", (long long)values[i] * INT_MAX,
                                  (long long)values[i] * INT_MAX));
    }
    record(thread_text_format("%s", ""));
    record(thread_text_format("error : %s", "로드 실패"));
    record(thread_text_format("mylog(%s).txt", "Mar 31 2022"));
    record(thread_text_format("%ls/%S/%lc/%C", L"wide", L"text", L'A', L'B'));
    record(thread_text_format("%+.3f/%#.5g/%e", -12.345, 1234.5, 0.125));
    record(thread_text_format("%% %c %s", 'A', "한글"));
}

enum { THREAD_COUNT = 8, THREAD_ITERATIONS = 10000 };
struct Worker {
    unsigned id;
    HANDLE start, ready, release;
    const char *borrowed;
    char expected[64];
};

static unsigned __stdcall worker_main(void *context)
{
    struct Worker *worker = context;
    unsigned i;
    CHECK(WaitForSingleObject(worker->start, 30000) == WAIT_OBJECT_0);
    snprintf(worker->expected, sizeof worker->expected, "thread %u 한글", worker->id);
    worker->borrowed = thread_text_format("thread %u 한글", worker->id);
    CHECK(SetEvent(worker->ready));
    CHECK(WaitForSingleObject(worker->release, 30000) == WAIT_OBJECT_0);
    for (i = 0; i < THREAD_ITERATIONS; ++i) {
        char expected[64];
        const char *result = thread_text_format("%u/%u 한글", worker->id, i);
        snprintf(expected, sizeof expected, "%u/%u 한글", worker->id, i);
        if (strcmp(result, expected) != 0)
            return 1;
        if ((i & 127) == 0)
            SwitchToThread();
    }
    return 0;
}

static void check_threads(void)
{
    struct Worker workers[THREAD_COUNT];
    HANDLE threads[THREAD_COUNT];
    const char *main_text;
    unsigned round, i, j;
    for (round = 0; round < 4; ++round) {
        main_text = thread_text_format("main thread %u", round);
        for (i = 0; i < THREAD_COUNT; ++i) {
            workers[i].id = i;
            workers[i].start = CreateEventA(NULL, FALSE, FALSE, NULL);
            workers[i].ready = CreateEventA(NULL, FALSE, FALSE, NULL);
            workers[i].release = CreateEventA(NULL, TRUE, FALSE, NULL);
            CHECK(workers[i].start && workers[i].ready && workers[i].release);
            threads[i] = (HANDLE)_beginthreadex(NULL, 0, worker_main, &workers[i], 0, NULL);
            CHECK(threads[i]);
            CHECK(SetEvent(workers[i].start));
            CHECK(WaitForSingleObject(workers[i].ready, 30000) == WAIT_OBJECT_0);
        }
        for (i = 0; i < THREAD_COUNT; ++i) {
            CHECK(strcmp(workers[i].borrowed, workers[i].expected) == 0);
            CHECK(workers[i].borrowed != main_text);
            for (j = 0; j < i; ++j)
                CHECK(workers[i].borrowed != workers[j].borrowed);
        }
        {
            char expected[32];
            snprintf(expected, sizeof expected, "main thread %u", round);
            CHECK(strcmp(main_text, expected) == 0);
        }
        for (i = 0; i < THREAD_COUNT; ++i)
            CHECK(SetEvent(workers[i].release));
        CHECK(WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, 30000) == WAIT_OBJECT_0);
        for (i = 0; i < THREAD_COUNT; ++i) {
            DWORD code;
            CHECK(GetExitCodeThread(threads[i], &code) && code == 0);
            CHECK(CloseHandle(threads[i]));
            CHECK(CloseHandle(workers[i].start));
            CHECK(CloseHandle(workers[i].ready));
            CHECK(CloseHandle(workers[i].release));
        }
    }
    puts("Thread text: 32 thread lifetimes and 320000 concurrent formats passed.");
}

static void check_bounds_and_borrowing(void)
{
    char input[8193];
    const char *result;
    size_t length, i;
    for (length = 2047; length <= sizeof input - 1; ++length) {
        memset(input, 'x', length);
        input[length] = 0;
        result = thread_text_format("%s", input);
        CHECK(strlen(result) == THREAD_TEXT_CAPACITY - 1);
        for (i = 0; i < THREAD_TEXT_CAPACITY - 1; ++i)
            CHECK(result[i] == 'x');
    }
    result = thread_text_format("%s", "한글");
    result = thread_text_format("prefix:%s", result);
    CHECK(strcmp(result, "prefix:한글") == 0);
    result = thread_text_format("%%s/%%d");
    result = thread_text_format(result, "borrowed format", 42);
    CHECK(strcmp(result, "borrowed format/42") == 0);
    result = thread_text_format("%ls", L"\xd800");
    CHECK(strcmp(result, "") == 0);
    puts("Thread text: termination, truncation, borrowed inputs and encoding failure passed.");
}

int main(int argc, char **argv)
{
    CHECK(setlocale(LC_ALL, "C"));
    if (argc > 1) {
        trace = fopen(argv[1], "wb");
        CHECK(trace);
    }
    check_formats();
    if (trace)
        CHECK(fclose(trace) == 0);
    CHECK(digest == 0xc33d011du);
    printf("Thread text formats: %u cases; digest=%08x.\n", cases, digest);
    check_bounds_and_borrowing();
    check_threads();
    return 0;
}
