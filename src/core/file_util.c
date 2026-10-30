#include <string.h>
#include "core/fatal.h"
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "core/file_util.h"
#include "core/app.h"
#include <lauxlib.h>

static int string_equals(const struct String *s, const char *text, size_t len)
{
    return s->size == len && memcmp(string_cstr(s), text, len) == 0;
}

static void destroy_string(void *value) { string_free(value); }

void dir_remove_tree(const char *path)
{
    struct List files, dirs;
    WIN32_FIND_DATAA found;
    char pattern[0x800];
    struct String dot, dotdot, name, full;
    struct ListNode *node;
    HANDLE find;
    size_t len;

    list_init(&files);
    list_init(&dirs);
    memset(&found, 0, sizeof found);
    strcpy(pattern, path);
    strcat(pattern, "\\*.*");
    find = FindFirstFileA(pattern, &found);
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
            list_push_back((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? &dirs : &files, &name,
                           sizeof name);
        }
        while (FindNextFileA(find, &found)) {
            len = strlen(found.cFileName);
            if (string_equals(&dot, found.cFileName, len) || string_equals(&dotdot, found.cFileName, len))
                continue;
            string_init(&name);
            string_assign(&name, found.cFileName, len);
            list_push_back((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? &dirs : &files, &name,
                           sizeof name);
        }
        FindClose(find);

        strcpy(pattern, path);
        strcat(pattern, "\\");
        for (node = files.head->next; node != files.head; node = node->next) {
            string_init(&full);
            string_assign_cstr(&full, pattern);
            string_append(&full, string_cstr(LIST_VALUE(node, struct String)), (LIST_VALUE(node, struct String))->size);
            _chmod(string_cstr(&full), _S_IWRITE);
            string_free(&full);
            string_init(&full);
            string_assign_cstr(&full, pattern);
            string_append(&full, string_cstr(LIST_VALUE(node, struct String)), (LIST_VALUE(node, struct String))->size);
            _unlink(string_cstr(&full));
            string_free(&full);
        }
        for (node = dirs.head->next; node != dirs.head; node = node->next) {
            string_init(&full);
            string_assign_cstr(&full, pattern);
            string_append(&full, string_cstr(LIST_VALUE(node, struct String)), (LIST_VALUE(node, struct String))->size);
            dir_remove_tree(string_cstr(&full));
            string_free(&full);
        }
        string_free(&dotdot);
        string_free(&dot);
    }
    _rmdir(path);
    list_free(&dirs, destroy_string);
    list_free(&files, destroy_string);
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

void dir_list_names_vector(struct Vec *names, const char *folder, const char *pattern)
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
            string_vec_push_move(names, &name);
            string_free(&name);
        }
        while (FindNextFileA(find, &found)) {
            len = strlen(found.cFileName);
            if (string_equals(&dot, found.cFileName, len) || string_equals(&dotdot, found.cFileName, len))
                continue;
            string_init(&name);
            string_assign(&name, found.cFileName, len);
            string_vec_push_move(names, &name);
            string_free(&name);
        }
        FindClose(find);
        string_free(&dotdot);
        string_free(&dot);
    }
    string_free(&search);
}

unsigned char make_directories(struct String path)
{
    struct String folder, rest, part;
    size_t next, skip, len;
    const char *at;
    unsigned char ok;

    if (path.size == 0) {
        ok = 0;
        goto done;
    }
    next = string_find(&path, "\\", 0);
    skip = next;
    string_init(&folder);
    string_copy(&rest, &path);
    while (path.size != folder.size) {
        string_init(&part);
        string_assign(&part, string_cstr(&path), path.size < next ? path.size : next);
        string_free(&folder);
        folder = part;

        skip++;
        if (rest.size < skip)
            fatal_error("invalid string position");
        string_init(&part);
        string_assign(&part, string_cstr(&rest) + skip, rest.size - skip);
        string_free(&rest);
        rest = part;

        skip = (size_t)-1;
        if (rest.size >= 1) {
            at = (const char *)memchr(string_cstr(&rest), '\\', rest.size);
            if (at)
                skip = (size_t)(at - string_cstr(&rest));
        }
        len = skip == (size_t)-1 ? path.size : next + 1 + skip;
        next = len;

        if (_access(string_cstr(&folder), 0) != 0 && _mkdir(string_cstr(&folder)) != 0) {
            ok = 0;
            goto cleanup;
        }
    }
    ok = 1;
cleanup:
    string_free(&rest);
    string_free(&folder);
done:
    string_free(&path);
    return ok;
}
