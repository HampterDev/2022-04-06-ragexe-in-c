#define CRASH_DIALOG_TEST_DRIVER
#include "crash_dialog_fixture.h"
#include "diag/crash_report.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char message[] = "A bounded crash report\r\n";
static LONG dialog_data;
static int failure, freed, transferred, opened, closed, ended, centered, displayed;
static HGLOBAL allocation;
static HWND window = (HWND)1;

LONG WINAPI test_dialog_set_data(HWND hwnd, int index, LONG value)
{
    assert(hwnd == window && index == DWLP_USER);
    dialog_data = value;
    return 0;
}
LONG WINAPI test_dialog_get_data(HWND hwnd, int index)
{
    assert(hwnd == window && index == DWLP_USER);
    return dialog_data;
}
BOOL WINAPI test_dialog_text(HWND hwnd, int control, const char *text)
{
    assert(hwnd == window && control == 0x3ee && !strcmp(text, message));
    ++displayed;
    return TRUE;
}
BOOL WINAPI test_dialog_rect(HWND hwnd, RECT *rect)
{
    assert(hwnd == window);
    rect->left = rect->top = 0;
    rect->right = 300;
    rect->bottom = 200;
    return TRUE;
}
int WINAPI test_dialog_metrics(int index) { return index == SM_CXSCREEN ? 800 : 600; }
BOOL WINAPI test_dialog_move(HWND hwnd, int x, int y, int width, int height, BOOL repaint)
{
    assert(hwnd == window && x == 250 && y == 200 && width == 300 && height == 200 && repaint);
    ++centered;
    return TRUE;
}
BOOL WINAPI test_dialog_end(HWND hwnd, INT_PTR result)
{
    assert(hwnd == window && result == 7);
    ++ended;
    return TRUE;
}
HWND WINAPI test_dialog_focus(HWND hwnd) { assert(hwnd == window); return NULL; }
HWND WINAPI test_dialog_item(HWND hwnd, int control) { assert(hwnd == window && control == 7); return window; }
HGLOBAL WINAPI test_dialog_alloc(UINT flags, SIZE_T size)
{
    assert(flags == GMEM_MOVEABLE && size == sizeof message);
    if (failure == 1) return NULL;
    allocation = GlobalAlloc(flags, size);
    assert(allocation);
    return allocation;
}
void *WINAPI test_dialog_lock(HGLOBAL memory)
{
    assert(memory == allocation);
    return failure == 2 ? NULL : GlobalLock(memory);
}
HGLOBAL WINAPI test_dialog_free(HGLOBAL memory)
{
    assert(memory == allocation && !transferred && !freed);
    ++freed;
    return GlobalFree(memory);
}
BOOL WINAPI test_clipboard_open(HWND hwnd) { assert(hwnd == window); ++opened; return failure != 3; }
BOOL WINAPI test_clipboard_empty(void) { return failure != 4; }
HANDLE WINAPI test_clipboard_set(UINT format, HANDLE memory)
{
    const char *text;
    assert(format == CF_TEXT && memory == allocation);
    text = GlobalLock(memory);
    assert(text && !strcmp(text, message));
    GlobalUnlock(memory);
    if (failure == 5) return NULL;
    ++transferred;
    return memory;
}
BOOL WINAPI test_clipboard_close(void) { ++closed; return TRUE; }

INT_PTR WINAPI test_dialog_box(HINSTANCE instance, const char *resource, HWND parent, DLGPROC proc, LPARAM parameter)
{
    assert(instance == GetModuleHandleA(NULL) && resource == MAKEINTRESOURCEA(0x75) && !parent);
    assert(proc(window, WM_INITDIALOG, 0, parameter));
    assert(displayed == 1 && centered == 1);
    assert(!proc(window, WM_PAINT, 0, 0));
    assert(!proc(window, WM_COMMAND, 999, 0));
    assert(proc(window, WM_COMMAND, 2, 0));
    assert(proc(window, WM_COMMAND, 7, 0));
    assert(proc(window, WM_CLOSE, 0, 0));
    assert(ended == 2);
    return 7;
}

int main(void)
{
    for (failure = 0; failure <= 5; ++failure) {
        freed = transferred = opened = closed = ended = centered = displayed = 0;
        allocation = NULL;
        crash_report_show_dialog(message);
        if (!failure) {
            assert(transferred == 1 && !freed && opened == 1 && closed == 1);
            assert(!GlobalFree(allocation));
        } else {
            assert(!transferred && freed == (failure != 1));
            assert(opened == (failure >= 3) && closed == (failure >= 4));
        }
    }
    puts("PASS crash dialog: presentation, close, clipboard transfer and every failure path");
    return 0;
}
