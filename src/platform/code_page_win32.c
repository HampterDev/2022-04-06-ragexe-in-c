#ifdef _WIN32
#include "platform/code_page.h"
#include "platform/win_types.h"
#include <mbctype.h>
#include <mbstring.h>
#include <shlwapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")

int platform_code_page_is_system_lead_byte(unsigned char byte)
{
    return IsDBCSLeadByte(byte);
}

char *platform_code_page_next(unsigned int code_page, const char *text)
{
    return CharNextExA((WORD)code_page, text, 0);
}

char *platform_code_page_prev(unsigned int code_page, const char *start, const char *text)
{
    return CharPrevExA((WORD)code_page, start, text, 0);
}

void platform_code_page_set_multibyte(unsigned int code_page)
{
    _setmbcp((int)code_page);
}

int platform_code_page_byte_type(const char *text, size_t index)
{
    switch (_mbsbtype((const unsigned char *)text, index)) {
    case _MBC_SINGLE: return CODE_PAGE_BYTE_SINGLE;
    case _MBC_LEAD:   return CODE_PAGE_BYTE_LEAD;
    case _MBC_TRAIL:  return CODE_PAGE_BYTE_TRAIL;
    default:          return CODE_PAGE_BYTE_ILLEGAL;
    }
}

char *platform_code_page_find(const char *text, const char *needle)
{
    return (char *)_mbsstr((const unsigned char *)text, (const unsigned char *)needle);
}

int platform_code_page_is_valid(unsigned int code_page, const char *text)
{
    return MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, text, -1, NULL, 0) != 0;
}

int platform_code_page_to_utf16(unsigned int code_page, const char *text, int length, uint16_t *out, int capacity)
{
    return MultiByteToWideChar(code_page, 0, text, length, out, capacity);
}

int platform_code_page_from_utf16(unsigned int code_page, const uint16_t *text, int length, char *out, int capacity)
{
    return WideCharToMultiByte(code_page, 0, text, length, out, capacity, NULL, NULL);
}

uint16_t *platform_utf16_find_ignore_case(const uint16_t *text, const uint16_t *needle)
{
    return StrStrIW(text, needle);
}
#endif
