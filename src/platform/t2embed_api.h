#ifndef PLATFORM_T2EMBED_API_H
#define PLATFORM_T2EMBED_API_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <t2embapi.h>

typedef LONG (WINAPI *EmbeddedFontLoadFn)(HANDLE *, ULONG, ULONG *, ULONG,
    ULONG *, READEMBEDPROC, LPVOID, LPWSTR, LPSTR, TTLOADINFO *);
typedef LONG (WINAPI *EmbeddedFontDeleteFn)(HANDLE, ULONG, ULONG *);

#endif
