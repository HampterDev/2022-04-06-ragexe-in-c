#ifndef BROWSER_ADDRESS_H
#define BROWSER_ADDRESS_H

#include <windows.h>
#include <oleauto.h>

BSTR browser_address_to_wide(const char *text);
char *browser_address_to_text(const WCHAR *text, UINT code_page);
int browser_address_prefix(const char *url, char **prefix);

enum BrowserAuthStatus {
    BROWSER_AUTH_PENDING, BROWSER_AUTH_SUCCESS, BROWSER_AUTH_FAILED,
    BROWSER_AUTH_UNEXPECTED
};
enum BrowserAuthStatus browser_address_auth_status(const char *text, const char *prefix);

#endif
