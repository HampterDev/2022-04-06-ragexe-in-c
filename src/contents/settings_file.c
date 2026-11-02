#include "contents/settings_file.h"
#include "platform/sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_FILE_LIMIT (1024 * 1024)

int settings_file_path(char out[MAX_PATH], const char *root, const char *address,
                      const char *port, const char *world, unsigned aid, unsigned gid)
{
    uint8_t digest[PLATFORM_SHA256_SIZE];
    char hex[PLATFORM_SHA256_SIZE * 2 + 1];
    const void *parts[] = {address, port, world};
    size_t sizes[3];
    int ok = 0, i, count;
    out[0] = 0;
    if (!root || !*root || !address || !*address || !port || !*port || !world || !aid)
        return 0;
    for (i = 0; i < 3; ++i)
        sizes[i] = strlen(parts[i]) + 1;
    if (!platform_sha256(parts, sizes, 3, digest))
        goto done;
    for (i = 0; i < PLATFORM_SHA256_SIZE; ++i)
        sprintf(hex + i * 2, "%02x", digest[i]);
    if (gid)
        count = snprintf(out, MAX_PATH, "%s\\SaveData\\Accounts\\%s\\%u\\Characters\\%u.json", root, hex, aid, gid);
    else
        count = snprintf(out, MAX_PATH, "%s\\SaveData\\Accounts\\%s\\%u.json", root, hex, aid);
    ok = count > 0 && count < MAX_PATH;
done:
    if (!ok) out[0] = 0;
    return ok;
}

int settings_file_read(const char *path, struct OwnedText *out)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    LARGE_INTEGER size;
    DWORD count;
    char *text;
    int ok = 0;
    if (file == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? 2 : 0;
    }
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > SETTINGS_FILE_LIMIT)
        goto done;
    text = malloc((size_t)size.QuadPart + 1);
    if (!text) goto done;
    if (ReadFile(file, text, (DWORD)size.QuadPart, &count, NULL) && count == size.QuadPart &&
        !memchr(text, 0, count)) {
        text[count] = 0;
        ok = owned_text_assign(out, text_view(text, count));
    }
    free(text);
done:
    CloseHandle(file);
    return ok;
}

int settings_file_write(const char *path, struct TextView text)
{
    char directory[MAX_PATH], temporary[MAX_PATH] = {0}, *p;
    HANDLE file = INVALID_HANDLE_VALUE;
    DWORD count;
    int ok = 0;
    if (!text.data || !text.size || text.size > SETTINGS_FILE_LIMIT || strlen(path) >= MAX_PATH)
        return 0;
    strcpy(directory, path);
    p = strrchr(directory, '\\');
    if (!p) return 0;
    *p = 0;
    for (p = directory + 3; *p; ++p) {
        if (*p != '\\') continue;
        *p = 0;
        if (!CreateDirectoryA(directory, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            return 0;
        *p = '\\';
    }
    if ((!CreateDirectoryA(directory, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) ||
        !GetTempFileNameA(directory, "acc", 0, temporary))
        return 0;
    file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        ok = WriteFile(file, text.data, (DWORD)text.size, &count, NULL) && count == text.size &&
             FlushFileBuffers(file);
        if (!CloseHandle(file)) ok = 0;
    }
    if (ok)
        ok = MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok) DeleteFileA(temporary);
    return ok;
}
