#ifndef PLATFORM_CODE_PAGE_H
#define PLATFORM_CODE_PAGE_H

#include <stddef.h>
#include <stdint.h>

enum { CODE_PAGE_SYSTEM = 0, CODE_PAGE_THREAD = 3, CODE_PAGE_UTF8 = 65001 };

enum CodePageByteType {
    CODE_PAGE_BYTE_ILLEGAL = -1,
    CODE_PAGE_BYTE_SINGLE,
    CODE_PAGE_BYTE_LEAD,
    CODE_PAGE_BYTE_TRAIL
};

extern unsigned int g_codePage;
extern unsigned char g_useCharsetTag;

int get_charset_from_lang(unsigned short lang);
unsigned int get_code_page_from_charset(int charset);
unsigned int code_page_from_lang(unsigned short lang);

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
