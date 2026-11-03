#include "platform/win_utf8.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define KEY ((HKEY)(uintptr_t)0x4321)
#define ROOT HKEY_CURRENT_USER

/* 한국 😀, 메시지 in CP949, /기쁨 and 서버 in UTF-8. */
#define KOREAN_EMOJI "\xed\x95\x9c\xea\xb5\xad \xf0\x9f\x98\x80"
#define MESSAGE_CP949 "\xb8\xde\xbd\xc3\xc1\xf6"
#define JOY "/\xea\xb8\xb0\xec\x81\xa8"
#define SERVER "\xec\x84\x9c\xeb\xb2\x84"

static int fail_alloc, boxes_w, boxes_a, opens, creates, queries, sets;
static wchar_t last_text[64], last_caption[64], last_path[128], last_name[64];
static int text_null, caption_null;
static UINT last_type;
static HWND last_owner;

/* One stored value. */
static wchar_t stored_name[64];
static DWORD stored_type;
static BYTE stored[256];
static DWORD stored_size;
static int has_value;

void *win_utf8_test_malloc(size_t size)
{
    return fail_alloc ? NULL : malloc(size);
}

static void copy_wide(wchar_t *out, size_t capacity, const wchar_t *text, int *null)
{
    if (null)
        *null = !text;
    out[0] = 0;
    if (text) {
        assert(wcslen(text) < capacity);
        wcscpy(out, text);
    }
}

int WINAPI win_utf8_test_message_box_w(HWND owner, LPCWSTR text, LPCWSTR caption, UINT type)
{
    ++boxes_w;
    last_owner = owner;
    last_type = type;
    copy_wide(last_text, 64, text, &text_null);
    copy_wide(last_caption, 64, caption, &caption_null);
    return IDOK;
}

int WINAPI win_utf8_test_message_box_a(HWND owner, LPCSTR text, LPCSTR caption, UINT type)
{
    (void)owner; (void)text; (void)caption; (void)type;
    ++boxes_a;
    return IDCANCEL;
}

LONG WINAPI win_utf8_test_open(HKEY root, LPCWSTR path, DWORD options, REGSAM access, PHKEY key)
{
    assert(root == ROOT && !options && access == KEY_QUERY_VALUE && key);
    ++opens;
    copy_wide(last_path, 128, path, NULL);
    *key = KEY;
    return ERROR_SUCCESS;
}

LONG WINAPI win_utf8_test_create(HKEY root, LPCWSTR path, DWORD reserved, LPWSTR class_name, DWORD options,
                                 REGSAM access, const LPSECURITY_ATTRIBUTES security, PHKEY key,
                                 LPDWORD disposition)
{
    assert(root == ROOT && !reserved && !class_name && !options && access == KEY_ALL_ACCESS && !security && key);
    ++creates;
    copy_wide(last_path, 128, path, NULL);
    *key = KEY;
    if (disposition)
        *disposition = REG_OPENED_EXISTING_KEY;
    return ERROR_SUCCESS;
}

LONG WINAPI win_utf8_test_query(HKEY key, LPCWSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data,
                                LPDWORD size)
{
    assert(key == KEY && !reserved);
    ++queries;
    copy_wide(last_name, 64, name, NULL);
    if (!has_value || wcscmp(name, stored_name))
        return ERROR_FILE_NOT_FOUND;
    if (type)
        *type = stored_type;
    if (!size)
        return data ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
    if (data && *size < stored_size) {
        *size = stored_size;
        return ERROR_MORE_DATA;
    }
    if (data)
        memcpy(data, stored, stored_size);
    *size = stored_size;
    return ERROR_SUCCESS;
}

LONG WINAPI win_utf8_test_set(HKEY key, LPCWSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD size)
{
    assert(key == KEY && !reserved && size <= sizeof stored);
    ++sets;
    copy_wide(stored_name, 64, name, NULL);
    stored_type = type;
    memcpy(stored, data, size);
    stored_size = size;
    has_value = 1;
    return ERROR_SUCCESS;
}

static void store_wide(const wchar_t *name, DWORD type, const wchar_t *text, DWORD bytes)
{
    wcscpy(stored_name, name);
    stored_type = type;
    memcpy(stored, text, bytes);
    stored_size = bytes;
    has_value = 1;
}

static void message_boxes(void)
{
    HWND owner = (HWND)(uintptr_t)0x99;
    assert(win_utf8_message_box(owner, KOREAN_EMOJI, "Error", MB_OK | MB_ICONWARNING) == IDOK);
    assert(boxes_w == 1 && last_owner == owner && last_type == (MB_OK | MB_ICONWARNING));
    assert(!wcscmp(last_text, L"\xd55c\xad6d \xd83d\xde00") && !wcscmp(last_caption, L"Error"));
    /* Not UTF-8: a resource name in CP949. */
    assert(win_utf8_message_box(NULL, "data\\" MESSAGE_CP949 ".bmp", NULL, 0) == IDOK);
    assert(!wcscmp(last_text, L"data\\\xba54\xc2dc\xc9c0.bmp") && caption_null && !text_null);
    assert(win_utf8_message_box(NULL, "", "", 0) == IDOK && !last_text[0] && !caption_null);
    fail_alloc = 1;
    assert(win_utf8_message_box(NULL, "text", "caption", 0) == IDCANCEL && boxes_a == 1 && boxes_w == 3);
    fail_alloc = 0;
}

static void keys(void)
{
    HKEY key = NULL;
    DWORD disposition = 0;
    assert(win_utf8_reg_open_key(ROOT, "Software\\Gravity Soft\\Ragnarok\\" SERVER, 0, KEY_QUERY_VALUE, &key)
           == ERROR_SUCCESS && key == KEY && opens == 1);
    assert(!wcscmp(last_path, L"Software\\Gravity Soft\\Ragnarok\\\xc11c\xbc84"));
    assert(win_utf8_reg_create_key(ROOT, "Software\\Test", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition)
           == ERROR_SUCCESS && creates == 1 && disposition == REG_OPENED_EXISTING_KEY);
    assert(!wcscmp(last_path, L"Software\\Test"));
    fail_alloc = 1;
    assert(win_utf8_reg_open_key(ROOT, "x", 0, KEY_QUERY_VALUE, &key) == ERROR_NOT_ENOUGH_MEMORY && opens == 1);
    assert(win_utf8_reg_create_key(ROOT, "x", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL)
           == ERROR_NOT_ENOUGH_MEMORY && creates == 1);
    fail_alloc = 0;
}

static void string_values(void)
{
    char text[16];
    DWORD type, size;

    store_wide(L"2", REG_SZ, L"/\xae30\xc068", 8);
    memset(text, 0x55, sizeof text);
    type = 0; size = sizeof text;
    assert(win_utf8_reg_query_value(KEY, "2", NULL, &type, (BYTE *)text, &size) == ERROR_SUCCESS);
    assert(type == REG_SZ && size == sizeof JOY && !strcmp(text, JOY));
    /* Too small: the size it needs, and the buffer as it was. */
    memset(text, 0x55, sizeof text);
    size = sizeof JOY - 1;
    assert(win_utf8_reg_query_value(KEY, "2", NULL, &type, (BYTE *)text, &size) == ERROR_MORE_DATA);
    assert(size == sizeof JOY && (BYTE)text[0] == 0x55);
    size = 0;
    assert(win_utf8_reg_query_value(KEY, "2", NULL, NULL, NULL, &size) == ERROR_SUCCESS && size == sizeof JOY);
    assert(win_utf8_reg_query_value(KEY, "2", NULL, &type, NULL, NULL) == ERROR_SUCCESS);
    /* Without a terminator, or with text after a NUL. */
    store_wide(L"ID", REG_SZ, L"abc", 6);
    size = sizeof text;
    assert(win_utf8_reg_query_value(KEY, "ID", NULL, &type, (BYTE *)text, &size) == ERROR_SUCCESS);
    assert(!strcmp(text, "abc") && size == 4);
    store_wide(L"ID", REG_EXPAND_SZ, L"ab\0cd", 12);
    size = sizeof text;
    assert(win_utf8_reg_query_value(KEY, "ID", NULL, &type, (BYTE *)text, &size) == ERROR_SUCCESS);
    assert(type == REG_EXPAND_SZ && !strcmp(text, "ab") && size == 3);
    store_wide(L"ID", REG_SZ, L"", 0);
    size = sizeof text;
    assert(win_utf8_reg_query_value(KEY, "ID", NULL, &type, (BYTE *)text, &size) == ERROR_SUCCESS);
    assert(!text[0] && size == 1);
    /* Missing values leave the type alone. */
    type = 77;
    assert(win_utf8_reg_query_value(KEY, "missing", NULL, &type, (BYTE *)text, &size) == ERROR_FILE_NOT_FOUND);
    assert(type == 77 && !wcscmp(last_name, L"missing"));
    fail_alloc = 1;
    assert(win_utf8_reg_query_value(KEY, "ID", NULL, &type, (BYTE *)text, &size) == ERROR_NOT_ENOUGH_MEMORY);
    fail_alloc = 0;

    /* Writing: the text up to the first NUL within size, as UTF-16. */
    memset(text, 0, sizeof text);
    memcpy(text, "abc", 3);
    assert(win_utf8_reg_set_value(KEY, "ID", 0, REG_SZ, (const BYTE *)text, sizeof text) == ERROR_SUCCESS);
    assert(!wcscmp(stored_name, L"ID") && stored_type == REG_SZ && stored_size == 8);
    assert(!memcmp(stored, L"abc", 8));
    assert(win_utf8_reg_set_value(KEY, "2", 0, REG_SZ, (const BYTE *)JOY, sizeof JOY) == ERROR_SUCCESS);
    assert(stored_size == 8 && !memcmp(stored, L"/\xae30\xc068", 8));
    assert(win_utf8_reg_set_value(KEY, "3", 0, REG_SZ, (const BYTE *)"xyz", 2) == ERROR_SUCCESS);
    assert(stored_size == 6 && !memcmp(stored, L"xy", 6));
    assert(win_utf8_reg_set_value(KEY, "4", 0, REG_SZ, (const BYTE *)MESSAGE_CP949, 6) == ERROR_SUCCESS);
    assert(stored_size == 8 && !memcmp(stored, L"\xba54\xc2dc\xc9c0", 8));
    /* Written and read back. */
    size = sizeof text;
    assert(win_utf8_reg_query_value(KEY, "4", NULL, &type, (BYTE *)text, &size) == ERROR_SUCCESS);
    assert(!strcmp(text, "\xeb\xa9\x94\xec\x8b\x9c\xec\xa7\x80"));
}

static void other_values(void)
{
    DWORD value = 0x12345678, out = 0, type = 0, size = sizeof out;
    GUID guid = {0x1, 0x2, 0x3, {4, 5, 6, 7, 8, 9, 10, 11}}, back;
    assert(win_utf8_reg_set_value(KEY, "CHECKSAVE", 0, REG_DWORD, (const BYTE *)&value, 4) == ERROR_SUCCESS);
    assert(stored_type == REG_DWORD && stored_size == 4 && !memcmp(stored, &value, 4));
    assert(win_utf8_reg_query_value(KEY, "CHECKSAVE", NULL, &type, (BYTE *)&out, &size) == ERROR_SUCCESS);
    assert(type == REG_DWORD && out == value && size == 4);
    size = 2;
    assert(win_utf8_reg_query_value(KEY, "CHECKSAVE", NULL, &type, (BYTE *)&out, &size) == ERROR_MORE_DATA);
    assert(win_utf8_reg_set_value(KEY, "GUIDDRIVER", 0, REG_BINARY, (const BYTE *)&guid, sizeof guid)
           == ERROR_SUCCESS && stored_size == sizeof guid);
    size = sizeof back;
    assert(win_utf8_reg_query_value(KEY, "GUIDDRIVER", NULL, &type, (BYTE *)&back, &size) == ERROR_SUCCESS);
    assert(type == REG_BINARY && !memcmp(&back, &guid, sizeof guid));
    fail_alloc = 1;
    assert(win_utf8_reg_set_value(KEY, "x", 0, REG_DWORD, (const BYTE *)&value, 4) == ERROR_NOT_ENOUGH_MEMORY);
    fail_alloc = 0;
}

int main(void)
{
    message_boxes();
    keys();
    string_values();
    other_values();
    puts("Win32 UTF-8 calls: message boxes, key paths and string values as UTF-16, CP949 names, sizes and failures passed.");
    return 0;
}
