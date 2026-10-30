#include <windows.h>
#include <stdlib.h>
extern unsigned encoding_allocations, encoding_live, encoding_fail_allocation;
extern unsigned encoding_calls, encoding_fail_call;
void *encoding_test_malloc(size_t bytes);
void encoding_test_free(void *memory);
int encoding_test_to_wide(UINT cp, DWORD flags, const char *input, int bytes, wchar_t *out, int count);
int encoding_test_from_wide(UINT cp, DWORD flags, const wchar_t *input, int count, char *out, int bytes,
                            const char *replacement, BOOL *used);
void encoding_test_reset(void);
#ifndef ENCODING_FIXTURE_IMPLEMENTATION
#define malloc encoding_test_malloc
#define free encoding_test_free
#define MultiByteToWideChar encoding_test_to_wide
#define WideCharToMultiByte encoding_test_from_wide
#endif
