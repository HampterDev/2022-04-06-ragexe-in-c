#include "platform/win_utf8.h"
#include "platform/code_page.h"
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

/* Bytes as UTF-16: as UTF-8 when they are valid UTF-8, otherwise as CP949. Returns a
   NUL-terminated copy to free, and its length in units; NULL for NULL text or no
   memory. */
static wchar_t *widen(const char *text, int bytes, int *units)
{
    UINT code_page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    wchar_t *wide;
    int count;

    if (!text)
        return NULL;
    if (bytes < 0)
        bytes = (int)strlen(text);
    count = bytes ? MultiByteToWideChar(code_page, flags, text, bytes, NULL, 0) : 0;
    if (bytes && !count) {
        code_page = RESOURCE_NAME_CODE_PAGE;
        flags = 0;
        count = MultiByteToWideChar(code_page, flags, text, bytes, NULL, 0);
        if (!count)
            return NULL;
    }
    wide = malloc(((size_t)count + 1) * sizeof *wide);
    if (!wide)
        return NULL;
    if (count && MultiByteToWideChar(code_page, flags, text, bytes, wide, count) != count) {
        free(wide);
        return NULL;
    }
    wide[count] = 0;
    if (units)
        *units = count;
    return wide;
}

int WINAPI win_utf8_message_box(HWND owner, LPCSTR text, LPCSTR caption, UINT type)
{
    wchar_t *wide_text = widen(text, -1, NULL), *wide_caption = widen(caption, -1, NULL);
    int result;

    if ((text && !wide_text) || (caption && !wide_caption))
        result = MessageBoxA(owner, text, caption, type);
    else
        result = MessageBoxW(owner, wide_text, wide_caption, type);
    free(wide_text);
    free(wide_caption);
    return result;
}

LONG WINAPI win_utf8_reg_open_key(HKEY root, LPCSTR path, DWORD options, REGSAM access, PHKEY key)
{
    wchar_t *wide = widen(path, -1, NULL);
    LONG result;

    if (path && !wide)
        return ERROR_NOT_ENOUGH_MEMORY;
    result = RegOpenKeyExW(root, wide, options, access, key);
    free(wide);
    return result;
}

LONG WINAPI win_utf8_reg_create_key(HKEY root, LPCSTR path, DWORD reserved, LPSTR class_name, DWORD options,
                                    REGSAM access, const LPSECURITY_ATTRIBUTES security, PHKEY key,
                                    LPDWORD disposition)
{
    wchar_t *wide = widen(path, -1, NULL), *wide_class = widen(class_name, -1, NULL);
    LONG result;

    if ((path && !wide) || (class_name && !wide_class))
        result = ERROR_NOT_ENOUGH_MEMORY;
    else
        result = RegCreateKeyExW(root, wide, reserved, wide_class, options, access, security, key, disposition);
    free(wide);
    free(wide_class);
    return result;
}

static int is_string(DWORD type)
{
    return type == REG_SZ || type == REG_EXPAND_SZ;
}

/* A string value read as UTF-16 and returned as UTF-8. */
static LONG query_string(HKEY key, const wchar_t *name, DWORD stored, LPBYTE data, LPDWORD size)
{
    wchar_t *wide;
    DWORD units = stored / sizeof *wide, read = stored;
    LONG result;
    int needed;

    wide = malloc(((size_t)units + 1) * sizeof *wide);
    if (!wide)
        return ERROR_NOT_ENOUGH_MEMORY;
    result = RegQueryValueExW(key, name, NULL, NULL, (LPBYTE)wide, &read);
    if (result == ERROR_SUCCESS) {
        units = read / sizeof *wide;
        wide[units] = 0;
        units = (DWORD)wcslen(wide);
        needed = units ? WideCharToMultiByte(CP_UTF8, 0, wide, (int)units, NULL, 0, NULL, NULL) : 0;
        if (units && !needed) {
            result = (LONG)GetLastError();
        } else if (!data) {
            *size = (DWORD)needed + 1;
        } else if ((DWORD)needed + 1 > *size) {
            *size = (DWORD)needed + 1;
            result = ERROR_MORE_DATA;
        } else {
            if (needed)
                WideCharToMultiByte(CP_UTF8, 0, wide, (int)units, (char *)data, needed, NULL, NULL);
            data[needed] = 0;
            *size = (DWORD)needed + 1;
        }
    }
    free(wide);
    return result;
}

LONG WINAPI win_utf8_reg_query_value(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data,
                                     LPDWORD size)
{
    wchar_t *wide = widen(name, -1, NULL);
    DWORD stored_type = REG_NONE, stored = 0;
    LONG result;

    if (name && !wide)
        return ERROR_NOT_ENOUGH_MEMORY;
    result = RegQueryValueExW(key, wide, reserved, &stored_type, NULL, &stored);
    if (result == ERROR_SUCCESS) {
        if (type)
            *type = stored_type;
        if (!is_string(stored_type))
            result = RegQueryValueExW(key, wide, reserved, type, data, size);
        else if (!size)
            result = data ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
        else
            result = query_string(key, wide, stored, data, size);
    }
    free(wide);
    return result;
}

LONG WINAPI win_utf8_reg_set_value(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data,
                                   DWORD size)
{
    wchar_t *wide = widen(name, -1, NULL), *value = NULL;
    LONG result;
    int units = 0;

    if (name && !wide)
        return ERROR_NOT_ENOUGH_MEMORY;
    if (!is_string(type) || !data) {
        result = RegSetValueExW(key, wide, reserved, type, data, size);
    } else {
        const char *text = (const char *)data, *end = memchr(text, 0, size);

        value = widen(text, end ? (int)(end - text) : (int)size, &units);
        result = value ? RegSetValueExW(key, wide, reserved, type, (const BYTE *)value,
                                        ((DWORD)units + 1) * sizeof *value)
                       : ERROR_NOT_ENOUGH_MEMORY;
    }
    free(wide);
    free(value);
    return result;
}
