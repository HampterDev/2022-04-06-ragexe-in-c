#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "diag/crash_report.h"

#define IDD_CRASH 0x75
#define IDC_CRASH_TEXT 0x3ee
#define IDC_CRASH_COPY 2
#define IDC_CRASH_CLOSE 7

static void copy_report(HWND window, const char *text)
{
    size_t length = strlen(text) + 1;
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, length);
    char *destination;
    if (!memory)
        return;
    destination = GlobalLock(memory);
    if (!destination) {
        GlobalFree(memory);
        return;
    }
    memcpy(destination, text, length);
    GlobalUnlock(memory);
    if (OpenClipboard(window)) {
        if (EmptyClipboard() && SetClipboardData(CF_TEXT, memory))
            memory = NULL;
        CloseClipboard();
    }
    if (memory)
        GlobalFree(memory);
}

static INT_PTR CALLBACK crash_dialog(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_INITDIALOG) {
        RECT rect;
        int width, height;
        SetWindowLongPtrA(window, DWLP_USER, (LONG_PTR)lparam);
        SetDlgItemTextA(window, IDC_CRASH_TEXT, (const char *)lparam);
        if (GetWindowRect(window, &rect)) {
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
            MoveWindow(window, (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
                       (GetSystemMetrics(SM_CYSCREEN) - height) / 2, width, height, TRUE);
        }
        return TRUE;
    }
    if (message == WM_CLOSE) {
        EndDialog(window, IDC_CRASH_CLOSE);
        return TRUE;
    }
    if (message != WM_COMMAND)
        return FALSE;
    switch (LOWORD(wparam)) {
    case IDC_CRASH_COPY: {
        const char *text = (const char *)GetWindowLongPtrA(window, DWLP_USER);
        if (text)
            copy_report(window, text);
        SetFocus(GetDlgItem(window, IDC_CRASH_CLOSE));
        return TRUE;
    }
    case IDC_CRASH_CLOSE:
        EndDialog(window, IDC_CRASH_CLOSE);
        return TRUE;
    default:
        return FALSE;
    }
}

void crash_report_show_dialog(const char *text)
{
    DialogBoxParamA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(IDD_CRASH), NULL,
                   crash_dialog, (LPARAM)text);
}
