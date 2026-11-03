#ifndef PLATFORM_WIN_UTF8_H
#define PLATFORM_WIN_UTF8_H

#include <windows.h>

/* Drop-in replacements for the ANSI message box and registry calls, taking the client's
   UTF-8 text. They call the W functions, so text does not depend on the Windows code
   page. Text that is not valid UTF-8 is taken as CP949, the encoding of resource
   names. */

int WINAPI win_utf8_message_box(HWND owner, LPCSTR text, LPCSTR caption, UINT type);

/* Key paths and value names are UTF-8. String values (REG_SZ, REG_EXPAND_SZ) are read
   and written as UTF-8; a string that does not fit in *size bytes, NUL included, gives
   ERROR_MORE_DATA with the size it needs and leaves data unchanged. A string written is
   the text up to its first NUL within size bytes. Other types pass through. */
LONG WINAPI win_utf8_reg_open_key(HKEY root, LPCSTR path, DWORD options, REGSAM access, PHKEY key);
LONG WINAPI win_utf8_reg_create_key(HKEY root, LPCSTR path, DWORD reserved, LPSTR class_name, DWORD options,
                                    REGSAM access, const LPSECURITY_ATTRIBUTES security, PHKEY key,
                                    LPDWORD disposition);
LONG WINAPI win_utf8_reg_query_value(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data,
                                     LPDWORD size);
LONG WINAPI win_utf8_reg_set_value(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data,
                                   DWORD size);

#endif
