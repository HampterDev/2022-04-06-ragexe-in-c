#ifndef PLATFORM_WIN_TYPES_H
#define PLATFORM_WIN_TYPES_H

/* The 32-bit Windows client links RAD's granny2.dll and mss32.dll. Every other build implements
   the parts of their APIs the client uses: resources/granny_gltf.c and platform/miles_sdl.c. */
#if defined(_WIN32) && !defined(_WIN64)
#define PLATFORM_RAD_DLLS 1
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <stdint.h>

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int16_t SHORT;
typedef uint16_t USHORT;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef int BOOL;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef float FLOAT;
typedef uint16_t WCHAR;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef DWORD COLORREF;

typedef void *HANDLE;
typedef struct PlatformWindowHandle *HWND;
typedef struct PlatformModuleHandle *HINSTANCE;
typedef struct PlatformDcHandle *HDC;
typedef struct PlatformGdiObject *HGDIOBJ;
typedef struct PlatformBitmapHandle *HBITMAP;
typedef struct PlatformFontHandle *HFONT;

typedef struct _GUID {
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[8];
} GUID;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE;

#define TRUE 1
#define FALSE 0
#define MAX_PATH 260

#define WINAPI
#define CALLBACK
#define __stdcall
#define __cdecl

#define LOBYTE(w) ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w) ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffff))
#define MAKEWORD(a, b) ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b) ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))

#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) (LOBYTE(rgb))
#define GetGValue(rgb) (LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) (LOBYTE((rgb) >> 16))

#include "platform/vk.h"
#endif

#endif
