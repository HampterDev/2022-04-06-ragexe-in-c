#include <string.h>
#include "core/file_util.h"
#include "platform/filesystem.h"

void dir_list_names_list(struct List *names, const char *folder, const char *pattern)
{
    char search[0x800];
    struct PlatformFind *find;
    struct String name;

    if (!platform_path_join(search, sizeof search, folder, pattern))
        return;
    find = platform_find_first(search);
    if (!find)
        return;
    do {
        const char *found = platform_find_name(find);

        if (!strcmp(found, ".") || !strcmp(found, ".."))
            continue;
        string_init(&name);
        string_assign_cstr(&name, found);
        list_push_back(names, &name, sizeof name);
    } while (platform_find_next(find));
    platform_find_close(find);
}
