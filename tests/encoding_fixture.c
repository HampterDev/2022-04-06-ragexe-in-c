#define ENCODING_FIXTURE_IMPLEMENTATION
#include "encoding_fixture.h"
#include <assert.h>

unsigned encoding_allocations, encoding_live, encoding_fail_allocation;
unsigned encoding_calls, encoding_fail_call;

void *encoding_test_malloc(size_t bytes)
{
    void *result;
    if (++encoding_allocations == encoding_fail_allocation)
        return NULL;
    result = malloc(bytes);
    if (result)
        ++encoding_live;
    return result;
}

void encoding_test_free(void *memory)
{
    if (memory) {
        assert(encoding_live);
        --encoding_live;
        free(memory);
        SetLastError(0xdead);
    }
}

int encoding_test_to_wide(UINT cp, DWORD flags, const char *input, int bytes, wchar_t *out, int count)
{
    if (++encoding_calls == encoding_fail_call) {
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
        return 0;
    }
    return MultiByteToWideChar(cp, flags, input, bytes, out, count);
}

int encoding_test_from_wide(UINT cp, DWORD flags, const wchar_t *input, int count, char *out, int bytes,
                            const char *replacement, BOOL *used)
{
    if (++encoding_calls == encoding_fail_call) {
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
        return 0;
    }
    return WideCharToMultiByte(cp, flags, input, count, out, bytes, replacement, used);
}

void encoding_test_reset(void)
{
    assert(!encoding_live);
    encoding_allocations = encoding_fail_allocation = 0;
    encoding_calls = encoding_fail_call = 0;
}
