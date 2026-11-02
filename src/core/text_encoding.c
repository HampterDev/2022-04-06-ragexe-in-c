#include "core/text_encoding.h"
#include "platform/code_page.h"
#include <stdint.h>
#include <stdlib.h>

uint16_t *encoding_to_wide(unsigned code_page, const char *text)
{
    uint16_t *output;
    int count;
    if (!text)
        return NULL;
    count = platform_code_page_to_utf16(code_page, text, -1, NULL, 0);
    if (!count || (size_t)count > SIZE_MAX / sizeof(*output))
        return NULL;
    output = malloc((size_t)count * sizeof(*output));
    if (!output)
        return NULL;
    if (!platform_code_page_to_utf16(code_page, text, -1, output, count)) {
        free(output);
        return NULL;
    }
    return output;
}

char *encoding_from_wide(unsigned code_page, const uint16_t *text)
{
    char *output;
    int count;
    if (!text)
        return NULL;
    count = platform_code_page_from_utf16(code_page, text, -1, NULL, 0);
    if (!count)
        return NULL;
    output = malloc((size_t)count);
    if (!output)
        return NULL;
    if (!platform_code_page_from_utf16(code_page, text, -1, output, count)) {
        free(output);
        return NULL;
    }
    return output;
}
