#include "contents/settings_file.h"
#include "platform/filesystem.h"
#include "platform/sha256.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_FILE_LIMIT (1024 * 1024)

int settings_file_path(char out[MAX_PATH], const char *root, const char *address,
                      const char *port, const char *world, unsigned aid, unsigned gid)
{
    uint8_t digest[PLATFORM_SHA256_SIZE];
    char hex[PLATFORM_SHA256_SIZE * 2 + 1], relative[MAX_PATH];
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
        count = snprintf(relative, sizeof relative, "SaveData\\Accounts\\%s\\%u\\Characters\\%u.json", hex, aid, gid);
    else
        count = snprintf(relative, sizeof relative, "SaveData\\Accounts\\%s\\%u.json", hex, aid);
    ok = count > 0 && count < MAX_PATH && platform_path_join(out, MAX_PATH, root, relative);
done:
    if (!ok) out[0] = 0;
    return ok;
}

static char *last_separator(char *path)
{
    char *backslash = strrchr(path, '\\'), *slash = strrchr(path, '/');
    return slash > backslash ? slash : backslash;
}

int settings_file_read(const char *path, struct OwnedText *out)
{
    struct PlatformFile *file = platform_file_open_read(path);
    int64_t size;
    size_t count;
    char *text;
    int ok = 0;
    if (!file)
        return errno == ENOENT ? 2 : 0;
    if (!platform_file_size(file, &size) || size <= 0 || size > SETTINGS_FILE_LIMIT)
        goto done;
    count = (size_t)size;
    text = malloc(count + 1);
    if (!text) goto done;
    if (platform_file_read(file, text, count) && !memchr(text, 0, count)) {
        text[count] = 0;
        ok = owned_text_assign(out, text_view(text, count));
    }
    free(text);
done:
    platform_file_close(file);
    return ok;
}

int settings_file_write(const char *path, struct TextView text)
{
    char directory[MAX_PATH], temporary[MAX_PATH] = {0}, *p;
    struct PlatformFile *file;
    if (!text.data || !text.size || text.size > SETTINGS_FILE_LIMIT || strlen(path) >= MAX_PATH)
        return 0;
    strcpy(directory, path);
    p = last_separator(directory);
    if (!p) return 0;
    *p = 0;
    if (!platform_directory_create_all(directory))
        return 0;
    file = platform_file_create_temporary(directory, "acc", temporary, sizeof temporary);
    if (!file)
        return 0;
    if (!platform_file_write(file, text.data, text.size)) {
        platform_file_discard(file, temporary);
        return 0;
    }
    return platform_file_replace(file, temporary, path);
}
