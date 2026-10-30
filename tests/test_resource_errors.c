#include "fileio/resource_errors.h"
#include "ui/window_mgr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct UIWindowMgr g_windowMgr;
static LONG lines, captions;
static int reenter;
static const char *recursive_path;
static char last_lines[3][256];
static int capture_lines;

int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int value, int colour, int c, int d)
{
    const char *text = (const char *)value;
    LONG line = InterlockedIncrement(&lines) - 1;
    assert(self == &g_windowMgr && msg == UIWM_MSG_CHAT_LINE && colour == 0xff && !c && !d);
    assert(text);
    if (strcmp(text, "Resource File Loading fail") == 0)
        InterlockedIncrement(&captions);
    if (capture_lines) {
        assert(strlen(text) < sizeof last_lines[0]);
        strcpy(last_lines[line % 3], text);
    }
    if (reenter) {
        reenter = 0;
        resource_error_report("recursive duplicate", recursive_path);
    }
    return 0;
}

static DWORD WINAPI report_paths(void *unused)
{
    int i;
    char path[64];
    (void)unused;
    for (i = 2047; i >= 0; --i) {
        sprintf(path, "texture\\thread_%04d.bmp", i);
        resource_error_report("parallel", path);
    }
    return 0;
}

int main(void)
{
    HANDLE threads[4];
    char path[64] = "texture\\a.bmp";
    unsigned i;
    resource_errors_init();
    capture_lines = 1;
    recursive_path = path;
    reenter = 1;
    resource_error_report("A.BMP", path);
    assert(lines == 3 && captions == 1 && !reenter);
    assert(strcmp(last_lines[0], "Resource File Loading fail") == 0);
    assert(strcmp(last_lines[1], "A.BMP") == 0);
    assert(strcmp(last_lines[2], path) == 0);
    strcpy(path, "texture\\b.bmp");
    resource_error_report("different name", "texture\\a.bmp");
    assert(lines == 3);
    resource_error_report("b.bmp", path);
    assert(lines == 6);
    resource_error_report("upper", "texture\\A.bmp");
    assert(lines == 9);
    resource_error_report("emblem", "missing.EBM");
    resource_error_report(NULL, path);
    resource_error_report("name", NULL);
    assert(lines == 9);
    capture_lines = 0;
    for (i = 0; i < 4; ++i) {
        threads[i] = CreateThread(NULL, 0, report_paths, NULL, 0, NULL);
        assert(threads[i]);
    }
    assert(WaitForMultipleObjects(4, threads, TRUE, INFINITE) == WAIT_OBJECT_0);
    for (i = 0; i < 4; ++i)
        assert(CloseHandle(threads[i]));
    assert(lines == (2048 + 3) * 3 && captions == 2048 + 3);
    resource_errors_destroy();
    resource_errors_init();
    lines = captions = 0;
    resource_error_report("again", "texture\\a.bmp");
    assert(lines == 3 && captions == 1);
    resource_errors_destroy();
    puts("resource errors: owned paths, deduplication, growth, concurrent reports, reentry and cleanup passed");
    return 0;
}
