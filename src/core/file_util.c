#include <string.h>
#include "core/file_util.h"

static int string_equals(const struct String *s, const char *text, size_t len)
{
    return s->size == len && memcmp(string_cstr(s), text, len) == 0;
}

void dir_list_names_list(struct List *names, const char *folder, const char *pattern)
{
    struct String search, dot, dotdot, name;
    WIN32_FIND_DATAA found;
    HANDLE find;
    size_t len;

    string_init(&search);
    string_assign_cstr(&search, folder);
    string_append_cstr(&search, pattern);
    memset(&found, 0, sizeof found);
    find = FindFirstFileA(string_cstr(&search), &found);
    if (find == INVALID_HANDLE_VALUE) {
        FindClose(find);
    } else {
        string_init(&dot);
        string_assign_cstr(&dot, ".");
        string_init(&dotdot);
        string_assign_cstr(&dotdot, "..");
        len = strlen(found.cFileName);
        if (!string_equals(&dot, found.cFileName, len) && !string_equal_cstr(&dotdot, found.cFileName)) {
            string_init(&name);
            string_assign(&name, found.cFileName, len);
            list_push_back(names, &name, sizeof name);
        }
        while (FindNextFileA(find, &found)) {
            len = strlen(found.cFileName);
            if (string_equals(&dot, found.cFileName, len) || string_equals(&dotdot, found.cFileName, len))
                continue;
            string_init(&name);
            string_assign(&name, found.cFileName, len);
            list_push_back(names, &name, sizeof name);
        }
        FindClose(find);
        string_free(&dotdot);
        string_free(&dot);
    }
    string_free(&search);
}
