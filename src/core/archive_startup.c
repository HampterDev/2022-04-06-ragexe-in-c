#include "core/archive_startup.h"
#include <windows.h>

void create_data_archive_if_missing(void)
{
    HANDLE file = CreateFileA("data.grf ", GENERIC_READ | GENERIC_WRITE, 0,
                              NULL, OPEN_ALWAYS,
                              FILE_FLAG_WRITE_THROUGH | FILE_ATTRIBUTE_NORMAL, NULL);

    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
}
