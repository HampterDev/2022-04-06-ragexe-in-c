#include <windows.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/text_util.h"
#include "std/string.h"
#include "std/string_vec.h"
#include "ui/skin_manager.h"


struct Entry {
    const char *name;
    DWORD attributes;
};

static const struct Entry *listing;
static size_t listed, next;
static int finds, closes;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

static void fill(WIN32_FIND_DATAA *fd)
{
    memset(fd, 0, sizeof *fd);
    fd->dwFileAttributes = listing[next].attributes;
    strcpy(fd->cFileName, listing[next].name);
    ++next;
}

static HANDLE WINAPI test_find_first(LPCSTR pattern, LPWIN32_FIND_DATAA fd)
{
    assert(strcmp(pattern, ".\\skin\\*.*") == 0);
    ++finds;
    next = 0;
    if (!listed)
        return INVALID_HANDLE_VALUE;
    fill(fd);
    return (HANDLE)0x5c1;
}

static BOOL WINAPI test_find_next(HANDLE find, LPWIN32_FIND_DATAA fd)
{
    assert(find == (HANDLE)0x5c1);
    if (next == listed)
        return FALSE;
    fill(fd);
    return TRUE;
}

static BOOL WINAPI test_find_close(HANDLE find)
{
    (void)find;
    ++closes;
    return TRUE;
}

#define FindFirstFileA test_find_first
#define FindNextFileA test_find_next
#define FindClose test_find_close
#include "skin_scan.inc"
#undef FindFirstFileA
#undef FindNextFileA
#undef FindClose

static int cases;

static const char *at(const struct Vec *v, size_t i)
{
    return string_cstr((const struct String *)v->first + i);
}

static void check_skins(const struct SkinMgr *mgr, const char *const *names, const char *const *paths,
                        const int *zip, size_t count)
{
    size_t i;

    assert((size_t)((const struct String *)mgr->names.last - (const struct String *)mgr->names.first) == count);
    assert((size_t)((const struct String *)mgr->paths.last - (const struct String *)mgr->paths.first) == count);
    assert((size_t)((const int *)mgr->is_zip.last - (const int *)mgr->is_zip.first) == count);
    for (i = 0; i < count; ++i) {
        assert(strcmp(at(&mgr->names, i), names[i]) == 0);
        assert(strcmp(at(&mgr->paths, i), paths[i]) == 0);
        assert(((const int *)mgr->is_zip.first)[i] == zip[i]);
    }
    ++cases;
}

int main(void)
{
    static const struct Entry folder[] = {
        { ".", FILE_ATTRIBUTE_DIRECTORY }, { "..", FILE_ATTRIBUTE_DIRECTORY },
        { "Classic", FILE_ATTRIBUTE_DIRECTORY }, { "README", FILE_ATTRIBUTE_NORMAL },
        { "Blue.ZIP", FILE_ATTRIBUTE_ARCHIVE }, { "notes.txt", FILE_ATTRIBUTE_ARCHIVE },
        { "LICENSE", FILE_ATTRIBUTE_ARCHIVE }, { "Night.zip", FILE_ATTRIBUTE_ARCHIVE },
        { "zip", FILE_ATTRIBUTE_ARCHIVE },
    };
    static const char *const names[] = { "Classic", "Blue", "Night" };
    static const char *const paths[] = {
        "..\\..\\skin\\Classic\\", "..\\..\\_tmpskin_\\Blue\\", "..\\..\\_tmpskin_\\Night\\",
    };
    static const int zip[] = { 0, 1, 1 };
    static const struct Entry only_plain[] = { { "README", FILE_ATTRIBUTE_NORMAL } };
    struct SkinMgr mgr;

    memset(&mgr, 0, sizeof mgr);
    string_init(&mgr.name);
    string_assign_cstr(&mgr.name, "Night");
    listing = folder;
    listed = sizeof folder / sizeof *folder;
    skin_mgr_scan(&mgr);
    check_skins(&mgr, names, paths, zip, 3);
    assert(mgr.current == 2 && finds == 1 && closes == 1);

    listing = only_plain;
    listed = 1;
    skin_mgr_scan(&mgr);
    check_skins(&mgr, names, paths, zip, 0);
    assert(mgr.current == -1 && closes == 2);
    printf("Skin scan: %d scans; files without an extension are skipped like other files.\n", cases);
    return 0;
}
