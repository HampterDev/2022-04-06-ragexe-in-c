#ifndef PLATFORM_CODE_PAGE_H
#define PLATFORM_CODE_PAGE_H

#include <stddef.h>
#include <stdint.h>

enum { CODE_PAGE_SYSTEM = 0, CODE_PAGE_THREAD = 3, CODE_PAGE_UTF8 = 65001 };

enum {
    CHARSET_ANSI = 0, CHARSET_DEFAULT = 1, CHARSET_SHIFTJIS = 128, CHARSET_HANGUL = 129, CHARSET_GB2312 = 134,
    CHARSET_CHINESEBIG5 = 136, CHARSET_GREEK = 161, CHARSET_TURKISH = 162, CHARSET_VIETNAMESE = 163,
    CHARSET_HEBREW = 177, CHARSET_ARABIC = 178, CHARSET_BALTIC = 186, CHARSET_RUSSIAN = 204,
    CHARSET_THAI = 222, CHARSET_EASTEUROPE = 238
};

enum CodePageByteType {
    CODE_PAGE_BYTE_ILLEGAL = -1,
    CODE_PAGE_BYTE_SINGLE,
    CODE_PAGE_BYTE_LEAD,
    CODE_PAGE_BYTE_TRAIL
};

extern unsigned int g_codePage;

/* Resource and file names in the GRF archives and data folder are CP949. */
#define RESOURCE_NAME_CODE_PAGE 949

int platform_code_page_is_system_lead_byte(unsigned char byte);
char *platform_code_page_next(unsigned int code_page, const char *text);
char *platform_code_page_prev(unsigned int code_page, const char *start, const char *text);
void platform_code_page_set_multibyte(unsigned int code_page);
int platform_code_page_byte_type(const char *text, size_t index);
char *platform_code_page_find(const char *text, const char *needle);
int platform_code_page_is_valid(unsigned int code_page, const char *text);
int platform_code_page_to_utf16(unsigned int code_page, const char *text, int length, uint16_t *out, int capacity);
int platform_code_page_from_utf16(unsigned int code_page, const uint16_t *text, int length, char *out, int capacity);
uint16_t *platform_utf16_find_ignore_case(const uint16_t *text, const uint16_t *needle);

#endif
