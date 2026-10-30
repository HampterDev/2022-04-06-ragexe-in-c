#ifndef BROWSER_SITE_H
#define BROWSER_SITE_H

#include <windows.h>
#include <exdisp.h>

struct BrowserSite;
HRESULT browser_site_open(HWND hwnd, IWebBrowser2 *browser, struct BrowserSite **out);
void browser_site_close(struct BrowserSite *site);

#endif
