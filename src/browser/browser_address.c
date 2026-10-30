#include "browser/browser_address.h"
#include <mbstring.h>
#include <stdlib.h>
#include <string.h>

BSTR browser_address_to_wide(const char *text)
{
    BSTR wide;
    int count;
    if (!text) return NULL;
    count = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (!count) return NULL;
    wide = SysAllocStringLen(NULL, (UINT)count - 1);
    if (!wide) return NULL;
    if (!MultiByteToWideChar(CP_ACP, 0, text, -1, wide, count)) {
        SysFreeString(wide);
        return NULL;
    }
    return wide;
}

char *browser_address_to_text(const WCHAR *text, UINT code_page)
{
    char *result;
    int count;
    if (!text) return NULL;
    count = WideCharToMultiByte(code_page, 0, text, -1, NULL, 0, NULL, NULL);
    if (!count) return NULL;
    result = malloc((size_t)count);
    if (!result) return NULL;
    if (!WideCharToMultiByte(code_page, 0, text, -1, result, count, NULL, NULL)) {
        free(result);
        return NULL;
    }
    return result;
}

int browser_address_prefix(const char *url, char **prefix)
{
    const char *port;
    char *copy = NULL;
    if (!url || !prefix) return 0;
    port = strstr(url, ":3000");
    if (port && port != url) {
        size_t length = (size_t)(port - url);
        copy = malloc(length + 1);
        if (!copy) return 0;
        memcpy(copy, url, length);
        copy[length] = 0;
    }
    *prefix = copy;
    return 1;
}

static int contains_address(const char *text, const char *prefix, const char *suffix)
{
    const unsigned char *at = (const unsigned char *)text;
    size_t length = strlen(prefix), suffix_length = strlen(suffix);
    while ((at = _mbsstr(at, (const unsigned char *)prefix)) != NULL) {
        if (!strncmp((const char *)at + length, suffix, suffix_length)) return 1;
        at = _mbsinc(at);
    }
    return 0;
}

enum BrowserAuthStatus browser_address_auth_status(const char *text, const char *prefix)
{
    if (!text || !*text || !prefix || !*prefix) return BROWSER_AUTH_PENDING;
    if (contains_address(text, prefix, ":3000/success")) return BROWSER_AUTH_SUCCESS;
    if (contains_address(text, prefix, ":3000/fail")) return BROWSER_AUTH_FAILED;
    if (_mbsstr((const unsigned char *)text,
                (const unsigned char *)"api.twitter.com/oauth/authenticate"))
        return BROWSER_AUTH_PENDING;
    return BROWSER_AUTH_UNEXPECTED;
}
