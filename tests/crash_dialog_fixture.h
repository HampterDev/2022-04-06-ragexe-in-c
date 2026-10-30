#ifndef CRASH_DIALOG_FIXTURE_H
#define CRASH_DIALOG_FIXTURE_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
INT_PTR WINAPI test_dialog_box(HINSTANCE instance, const char *resource, HWND parent, DLGPROC proc, LPARAM parameter);
LONG WINAPI test_dialog_set_data(HWND window, int index, LONG value);
LONG WINAPI test_dialog_get_data(HWND window, int index);
BOOL WINAPI test_dialog_text(HWND window, int control, const char *text);
BOOL WINAPI test_dialog_rect(HWND window, RECT *rect);
int WINAPI test_dialog_metrics(int index);
BOOL WINAPI test_dialog_move(HWND window, int x, int y, int width, int height, BOOL repaint);
BOOL WINAPI test_dialog_end(HWND window, INT_PTR result);
HWND WINAPI test_dialog_focus(HWND window);
HWND WINAPI test_dialog_item(HWND window, int control);
HGLOBAL WINAPI test_dialog_alloc(UINT flags, SIZE_T size);
void *WINAPI test_dialog_lock(HGLOBAL memory);
HGLOBAL WINAPI test_dialog_free(HGLOBAL memory);
BOOL WINAPI test_clipboard_open(HWND window);
BOOL WINAPI test_clipboard_empty(void);
HANDLE WINAPI test_clipboard_set(UINT format, HANDLE memory);
BOOL WINAPI test_clipboard_close(void);
#ifndef CRASH_DIALOG_TEST_DRIVER
#define DialogBoxParamA test_dialog_box
#define SetWindowLongA test_dialog_set_data
#define GetWindowLongA test_dialog_get_data
#define SetDlgItemTextA test_dialog_text
#define GetWindowRect test_dialog_rect
#define GetSystemMetrics test_dialog_metrics
#define MoveWindow test_dialog_move
#define EndDialog test_dialog_end
#define SetFocus test_dialog_focus
#define GetDlgItem test_dialog_item
#define GlobalAlloc test_dialog_alloc
#define GlobalLock test_dialog_lock
#define GlobalFree test_dialog_free
#define OpenClipboard test_clipboard_open
#define EmptyClipboard test_clipboard_empty
#define SetClipboardData test_clipboard_set
#define CloseClipboard test_clipboard_close
#endif
#endif
