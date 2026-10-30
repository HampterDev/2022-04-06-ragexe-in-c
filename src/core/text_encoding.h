#ifndef CORE_TEXT_ENCODING_H
#define CORE_TEXT_ENCODING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

wchar_t *encoding_to_wide(unsigned code_page, const char *text);
char *encoding_from_wide(unsigned code_page, const wchar_t *text);

#ifdef __cplusplus
}
#endif
#endif
