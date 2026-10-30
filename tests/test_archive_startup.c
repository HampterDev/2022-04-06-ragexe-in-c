#include <assert.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "core/archive_startup.h"

static void verify_bytes(const unsigned char *expected, DWORD length)
{
    unsigned char actual[1024];
    DWORD read;
    HANDLE file = CreateFileA("data.grf", GENERIC_READ, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    assert(file != INVALID_HANDLE_VALUE);
    assert(GetFileSize(file, NULL) == length);
    assert(ReadFile(file, actual, sizeof actual, &read, NULL));
    assert(read == length && memcmp(actual, expected, length) == 0);
    assert(CloseHandle(file));
}

int main(void)
{
    char previous[MAX_PATH], temporary[MAX_PATH], directory[MAX_PATH];
    unsigned char contents[1024];
    DWORD written;
    HANDLE file;
    unsigned i;

    assert(GetCurrentDirectoryA(sizeof previous, previous) < sizeof previous);
    assert(GetTempPathA(sizeof temporary, temporary) < sizeof temporary);
    assert(GetTempFileNameA(temporary, "grf", 0, directory));
    assert(DeleteFileA(directory));
    assert(CreateDirectoryA(directory, NULL));
    assert(SetCurrentDirectoryA(directory));

    create_data_archive_if_missing();
    verify_bytes(contents, 0);
    create_data_archive_if_missing();
    verify_bytes(contents, 0);

    for (i = 0; i < sizeof contents; ++i)
        contents[i] = (unsigned char)(i * 37 + 11);
    file = CreateFileA("data.grf", GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    assert(WriteFile(file, contents, sizeof contents, &written, NULL));
    assert(written == sizeof contents && CloseHandle(file));
    create_data_archive_if_missing();
    verify_bytes(contents, sizeof contents);

    assert(SetFileAttributesA("data.grf", FILE_ATTRIBUTE_READONLY));
    create_data_archive_if_missing();
    verify_bytes(contents, sizeof contents);
    assert(SetFileAttributesA("data.grf", FILE_ATTRIBUTE_NORMAL));

    file = CreateFileA("data.grf", GENERIC_READ, 0, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    create_data_archive_if_missing();
    assert(CloseHandle(file));
    verify_bytes(contents, sizeof contents);
    assert(DeleteFileA("data.grf"));

    assert(CreateDirectoryA("data.grf", NULL));
    create_data_archive_if_missing();
    assert(GetFileAttributesA("data.grf") & FILE_ATTRIBUTE_DIRECTORY);
    assert(RemoveDirectoryA("data.grf"));
    assert(SetCurrentDirectoryA(previous));
    assert(RemoveDirectoryA(directory));
    puts("Archive startup: missing, empty, existing, read-only, locked and directory paths passed; existing bytes preserved.");
    return 0;
}
