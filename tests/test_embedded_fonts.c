#include "platform/t2embed_api.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

_Static_assert(_Generic(&TTLoadEmbeddedFont, EmbeddedFontLoadFn: 1, default: 0),
               "font-load signature must match the Windows SDK");
_Static_assert(_Generic(&TTDeleteEmbeddedFont, EmbeddedFontDeleteFn: 1, default: 0),
               "font-delete signature must match the Windows SDK");

static HMODULE WINAPI mock_load_library(LPCWSTR name);
static FARPROC WINAPI mock_get_proc(HMODULE module, LPCSTR name);
static BOOL WINAPI mock_free_library(HMODULE module);
static int WINAPI mock_widen(UINT page, DWORD flags, LPCCH text, int length,
                            LPWSTR out, int capacity);
static FILE *mock_open(const wchar_t *path, const wchar_t *mode);
static int mock_close(FILE *file);

#define LoadLibraryW mock_load_library
#define GetProcAddress mock_get_proc
#define FreeLibrary mock_free_library
#define MultiByteToWideChar mock_widen
#define _wfopen mock_open
#define fclose mock_close
#include "../src/platform/embedded_fonts.c"
#undef LoadLibraryW
#undef GetProcAddress
#undef FreeLibrary
#undef MultiByteToWideChar
#undef _wfopen
#undef fclose

static const char *const expected_files[] = {
    "RixSquirrel_10.eot", "RixLoveangel_10.eot", "RixNHCgogo_10.eot",
    "RixDiary_10.eot", "RixMiniHeart_10.eot", "RixFreshman_10.eot",
    "RixKid_10.eot", "RixMagic_10.eot", "RixJJangu_10.eot",
};
static int module_token;
static int module_loads, module_frees, api_loads, api_deletes, widen_calls;
static int file_opens, file_closes, open_attempts, open_index;
static FILE *open_file;
static int live_fonts[9], delete_order[9];
static LONG results[9];
static int dll_missing, missing_export, fail_open_at, fail_conversion_at, no_handle_at;

static int file_index(const char *name)
{
    int i;
    for (i = 0; i < 9; ++i)
        if (!strcmp(name, expected_files[i]))
            return i;
    abort();
}

static LONG WINAPI mock_load_font(HANDLE *font, ULONG flags, ULONG *privileges,
    ULONG requested, ULONG *status, READEMBEDPROC read, LPVOID stream,
    LPWSTR family, LPSTR mac_family, TTLOADINFO *info)
{
    unsigned char bytes[8], expected[8] = {'E', 'O', 'T', 0, 0x80, 0x81, 0, 0xff};
    wchar_t expected_family[32];
    int i = file_index(mac_family);
    ++api_loads;
    assert(open_file && stream == open_file && i == open_index);
    assert(flags == TTLOAD_PRIVATE && requested == LICENSE_EDITABLE);
    assert(*privileges == 2 && *status == 1 && !*font);
    assert(info->usStructSize == sizeof *info);
    assert(info->usRefStrSize == wcslen(L"http://www.ragnarok.co.kr") + 1);
    assert(!wcscmp(info->pusRefStr, L"http://www.ragnarok.co.kr"));
    assert(MultiByteToWideChar(CP_ACP, 0, expected_files[i], -1, expected_family, 32));
    assert(!wcscmp(family, expected_family));
    expected[3] = (unsigned char)i;
    assert(read(stream, bytes, 3) == 3);
    assert(read(stream, bytes + 3, 5) == 5);
    assert(!memcmp(bytes, expected, sizeof bytes));
    assert(read(stream, bytes, 1) == 0);
    if (results[i] == E_NONE && i != no_handle_at) {
        assert(!live_fonts[i]);
        live_fonts[i] = 1;
        *font = (HANDLE)(UINT_PTR)(0x100 + i);
    }
    *privileges = LICENSE_EDITABLE;
    *status = TTLOAD_FONT_SUBSETTED;
    return results[i];
}

static LONG WINAPI mock_delete_font(HANDLE font, ULONG flags, ULONG *status)
{
    int i = (int)(UINT_PTR)font - 0x100;
    assert(i >= 0 && i < 9 && live_fonts[i]);
    assert(flags == 0 && status && !open_file);
    assert(module_loads == module_frees + 1);
    live_fonts[i] = 0;
    delete_order[api_deletes++ % 9] = i;
    *status = 0;
    return E_NONE;
}

static HMODULE WINAPI mock_load_library(LPCWSTR name)
{
    assert(!wcscmp(name, L"t2embed.dll"));
    assert(module_loads == module_frees);
    if (dll_missing)
        return NULL;
    ++module_loads;
    return (HMODULE)&module_token;
}
static FARPROC WINAPI mock_get_proc(HMODULE module, LPCSTR name)
{
    assert(module == (HMODULE)&module_token);
    if (!strcmp(name, "TTLoadEmbeddedFont"))
        return missing_export == 1 ? NULL : (FARPROC)mock_load_font;
    assert(!strcmp(name, "TTDeleteEmbeddedFont"));
    return missing_export == 2 ? NULL : (FARPROC)mock_delete_font;
}
static BOOL WINAPI mock_free_library(HMODULE module)
{
    int i;
    assert(module == (HMODULE)&module_token && !open_file);
    for (i = 0; i < 9; ++i) assert(!live_fonts[i]);
    ++module_frees;
    assert(module_frees == module_loads);
    return TRUE;
}
static int WINAPI mock_widen(UINT page, DWORD flags, LPCCH input, int length,
                            LPWSTR out, int capacity)
{
    int i = file_index(input);
    ++widen_calls;
    assert(page == CP_ACP && flags == MB_PRECOMPOSED && length == -1);
    if (i == fail_conversion_at)
        return 0;
    return MultiByteToWideChar(page, flags, input, length, out, capacity);
}
static FILE *mock_open(const wchar_t *path, const wchar_t *mode)
{
    char name[32];
    unsigned char bytes[8] = {'E', 'O', 'T', 0, 0x80, 0x81, 0, 0xff};
    const wchar_t prefix[] = L"System\\Font\\";
    assert(!open_file && !wcscmp(mode, L"rb"));
    assert(!wcsncmp(path, prefix, wcslen(prefix)));
    assert(WideCharToMultiByte(CP_ACP, 0, path + wcslen(prefix), -1, name, sizeof name, NULL, NULL));
    open_index = file_index(name);
    ++open_attempts;
    if (open_index == fail_open_at)
        return NULL;
    open_file = tmpfile();
    assert(open_file);
    ++file_opens;
    bytes[3] = (unsigned char)open_index;
    assert(fwrite(bytes, 1, sizeof bytes, open_file) == sizeof bytes);
    rewind(open_file);
    return open_file;
}
static int mock_close(FILE *file)
{
    assert(file == open_file && file);
    open_file = NULL;
    ++file_closes;
    return fclose(file);
}

static void assert_released(void)
{
    int i;
    assert(!open_file && file_opens == file_closes && module_loads == module_frees);
    for (i = 0; i < 9; ++i) assert(!live_fonts[i]);
}
static void reset(void)
{
    embedded_fonts_clear();
    assert_released();
    module_loads = module_frees = api_loads = api_deletes = widen_calls = 0;
    file_opens = file_closes = open_attempts = 0;
    dll_missing = missing_export = 0;
    fail_open_at = fail_conversion_at = no_handle_at = -1;
    memset(results, 0, sizeof results);
}

int main(void)
{
    const char *missing;
    int i, j;
    static const LONG failures[] = {E_FONTDATAINVALID, E_NOFREEMEMORY, -1};

    reset();
    missing = "stale";
    assert(embedded_fonts_load(&missing) && !missing);
    assert(api_loads == 9 && file_opens == 9 && file_closes == 9);
    missing = "stale";
    assert(embedded_fonts_load(&missing) && !missing);
    assert(api_loads == 9 && module_loads == 1);
    embedded_fonts_clear();
    assert(api_deletes == 9);
    for (i = 0; i < 9; ++i) assert(delete_order[i] == 8 - i);
    assert_released();
    embedded_fonts_clear();
    assert(api_deletes == 9 && module_frees == 1);

    for (i = 1; i <= 3; ++i) {
        reset();
        if (i == 3) dll_missing = 1; else missing_export = i;
        missing = "stale";
        assert(!embedded_fonts_load(&missing) && !missing);
        assert(!open_attempts && !api_loads);
        assert_released();
    }
    for (i = 0; i < 9; ++i) {
        reset(); fail_open_at = i;
        assert(!embedded_fonts_load(&missing));
        assert(missing && !strcmp(missing, expected_files[i]));
        assert(api_loads == i && api_deletes == i && open_attempts == i + 1);
        assert_released();
        fail_open_at = -1;
        assert(embedded_fonts_load(&missing) && !missing);
        embedded_fonts_clear(); assert_released();

        reset(); fail_conversion_at = i;
        missing = "stale";
        assert(embedded_fonts_load(&missing) && !missing);
        assert(!api_deletes && open_attempts == 8 && widen_calls == 9 && api_loads == 8 && !live_fonts[i]);
        embedded_fonts_clear(); assert(api_deletes == 8); assert_released();
        for (j = 0; j < sizeof failures / sizeof failures[0]; ++j) {
            reset(); results[i] = failures[j];
            missing = "stale";
            assert(embedded_fonts_load(&missing) && !missing);
            assert(api_loads == 9 && !api_deletes && file_opens == file_closes && !live_fonts[i]);
            embedded_fonts_clear(); assert(api_deletes == 8); assert_released();
        }
        reset(); no_handle_at = i;
        assert(embedded_fonts_load(NULL) && !live_fonts[i]);
        embedded_fonts_clear(); assert(api_deletes == 8); assert_released();
    }
    reset(); results[2] = E_FONTALREADYEXISTS; results[7] = E_FONTNAMEALREADYEXISTS;
    assert(embedded_fonts_load(NULL));
    assert(api_loads == 9 && !live_fonts[2] && !live_fonts[7]);
    embedded_fonts_clear(); assert(api_deletes == 7); assert_released();
    for (i = 0; i < 20; ++i) {
        reset(); assert(embedded_fonts_load(NULL));
        embedded_fonts_clear(); assert_released();
    }
    puts("Embedded fonts: SDK signatures, names and licensing, real stream reads, owned registrations, a failing font leaving the others loaded, missing-file rollback and repeated cleanup passed");
    return 0;
}
