#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
extern unsigned encoding_allocations, encoding_live, encoding_fail_allocation;
extern unsigned encoding_calls, encoding_fail_call;
void *encoding_test_malloc(size_t bytes);
void encoding_test_free(void *memory);
int encoding_test_to_wide(unsigned int cp, const char *input, int bytes, uint16_t *out, int count);
int encoding_test_from_wide(unsigned int cp, const uint16_t *input, int count, char *out, int bytes);
void encoding_test_reset(void);
#ifndef ENCODING_FIXTURE_IMPLEMENTATION
#define malloc encoding_test_malloc
#define free encoding_test_free
#define platform_code_page_to_utf16 encoding_test_to_wide
#define platform_code_page_from_utf16 encoding_test_from_wide
#endif
