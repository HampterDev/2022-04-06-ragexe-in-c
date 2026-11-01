#include "core/unique_file.h"
#include "game/box_report.h"
#include <windows.h>
#include <assert.h>
#include <direct.h>
#include <errno.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

static char directory[MAX_PATH];

static void path_for(char *path, const char *name)
{
    int n = snprintf(path, MAX_PATH, "%s\\%s", directory, name);
    assert(n > 0 && n < MAX_PATH);
}

static void expect_bytes(const char *path, const char *expected)
{
    char bytes[4096];
    FILE *file = fopen(path, "rb");
    size_t count;
    assert(file);
    count = fread(bytes, 1, sizeof bytes, file);
    assert(!ferror(file) && feof(file));
    assert(count == strlen(expected));
    assert(memcmp(bytes, expected, count) == 0);
    assert(fclose(file) == 0);
}

static void report_format(void)
{
    char path[MAX_PATH];
    FILE *file;
    path_for(path, "report.txt");
    file = file_open_unique(path);
    assert(file);
    assert(box_report_begin(file));
    assert(box_report_item(file, 501, "빨간포션", 2, 3));
    assert(box_report_item(file, 502, "Orange Potion", 1, 3));
    assert(box_report_end(file, 3));
    assert(fclose(file) == 0);
    expect_bytes(path, "ID\tname\tcount\tproperty\r\n\r\n"
                      "501\t빨간포션\t2\t66.66667%\r\n"
                      "502\tOrange Potion\t1\t33.33334%\r\n"
                      "\r\nresult item count:3\r\n");

    file = fopen(path, "r");
    assert(file);
    assert(setvbuf(file, NULL, _IONBF, 0) == 0);
    assert(!box_report_begin(file));
    clearerr(file);
    assert(!box_report_item(file, 1, "cannot write", 1, 1));
    clearerr(file);
    assert(!box_report_end(file, 1));
    fclose(file);

    file = file_open_unique(path);
    assert(file);
    assert(box_report_begin(file) && box_report_end(file, 0));
    assert(fclose(file) == 0);
    path_for(path, "report(1).txt");
    expect_bytes(path, "ID\tname\tcount\tproperty\r\n\r\n\r\nresult item count:0\r\n");
    assert(remove(path) == 0);
    path_for(path, "report.txt");
    assert(remove(path) == 0);
}

static void names_and_failures(void)
{
    static const char *names[] = {"plain", "many.dots.txt", "한글.txt"};
    static const char *numbered[] = {"plain(1)", "many.dots(1).txt", "한글(1).txt"};
    char path[MAX_PATH], other[MAX_PATH];
    size_t i;
    FILE *first, *second;
    for (i = 0; i < sizeof names / sizeof names[0]; ++i) {
        path_for(path, names[i]);
        first = file_open_unique(path);
        assert(first && fputs("original", first) >= 0);
        second = file_open_unique(path);
        assert(second && fputs("second", second) >= 0);
        assert(fclose(first) == 0 && fclose(second) == 0);
        expect_bytes(path, "original");
        path_for(other, numbered[i]);
        expect_bytes(other, "second");
        assert(remove(path) == 0 && remove(other) == 0);
    }
    path_for(path, "directory.txt");
    assert(_mkdir(path) == 0);
    first = file_open_unique(path);
    assert(first && fclose(first) == 0);
    path_for(other, "directory(1).txt");
    assert(remove(other) == 0 && _rmdir(path) == 0);
    path_for(path, "missing\\report.txt");
    assert(file_open_unique(path) == NULL && errno == ENOENT);
    assert(file_open_unique(NULL) == NULL && errno == EINVAL);
    assert(file_open_unique("") == NULL && errno == EINVAL);
    assert(file_open_unique("folder\\") == NULL && errno == EINVAL);
}

static HANDLE start_event;
static DWORD WINAPI concurrent_writer(void *unused)
{
    char path[MAX_PATH];
    FILE *file;
    (void)unused;
    assert(WaitForSingleObject(start_event, 10000) == WAIT_OBJECT_0);
    path_for(path, "concurrent.txt");
    file = file_open_unique(path);
    assert(file && fputs("complete", file) >= 0 && fclose(file) == 0);
    return 0;
}

static void concurrent_files(void)
{
    HANDLE threads[16];
    char name[64], path[MAX_PATH];
    unsigned i;
    start_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    assert(start_event);
    for (i = 0; i < 16; ++i) {
        threads[i] = CreateThread(NULL, 0, concurrent_writer, NULL, 0, NULL);
        assert(threads[i]);
    }
    assert(SetEvent(start_event));
    assert(WaitForMultipleObjects(16, threads, TRUE, 10000) == WAIT_OBJECT_0);
    for (i = 0; i < 16; ++i) {
        CloseHandle(threads[i]);
        if (i)
            snprintf(name, sizeof name, "concurrent(%u).txt", i);
        else
            strcpy(name, "concurrent.txt");
        path_for(path, name);
        expect_bytes(path, "complete");
        assert(remove(path) == 0);
    }
    CloseHandle(start_event);
}

int main(void)
{
    char temp[MAX_PATH];
    assert(GetTempPathA(sizeof temp, temp));
    assert(GetTempFileNameA(temp, "hco", 0, directory));
    assert(DeleteFileA(directory) && CreateDirectoryA(directory, NULL));
    report_format();
    names_and_failures();
    concurrent_files();
    assert(RemoveDirectoryA(directory));
    puts("PASS: C reports preserve CP949 bytes, CRLF and percentages; unique files, errors and 16 concurrent writers");
    return 0;
}
