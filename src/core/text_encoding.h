#ifndef CORE_TEXT_ENCODING_H
#define CORE_TEXT_ENCODING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t *encoding_to_wide(unsigned code_page, const char *text);
char *encoding_from_wide(unsigned code_page, const uint16_t *text);

#ifdef __cplusplus
}
#endif
#endif
