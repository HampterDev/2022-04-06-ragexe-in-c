#include "core/text_encoding.h"
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

wchar_t *encoding_to_wide(unsigned code_page, const char *text)
{
    wchar_t *output;
    int count;
    if (!text) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    count = MultiByteToWideChar(code_page, 0, text, -1, NULL, 0);
    if (!count)
        return NULL;
    if ((size_t)count > SIZE_MAX / sizeof(*output)) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    output = malloc((size_t)count * sizeof(*output));
    if (!output) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    if (!MultiByteToWideChar(code_page, 0, text, -1, output, count)) {
        DWORD error = GetLastError();
        free(output);
        SetLastError(error);
        return NULL;
    }
    return output;
}

char *encoding_from_wide(unsigned code_page, const wchar_t *text)
{
    char *output;
    int count;
    if (!text) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    count = WideCharToMultiByte(code_page, 0, text, -1, NULL, 0, NULL, NULL);
    if (!count)
        return NULL;
    output = malloc((size_t)count);
    if (!output) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    if (!WideCharToMultiByte(code_page, 0, text, -1, output, count, NULL, NULL)) {
        DWORD error = GetLastError();
        free(output);
        SetLastError(error);
        return NULL;
    }
    return output;
}
