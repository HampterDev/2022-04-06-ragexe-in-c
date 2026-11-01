/* Exercise the production screenshot request handler; replace only its I/O. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include "ui/window_mgr.h"
#include "core/fatal.h"
#include "core/app.h"
#include "world/mode.h"
#include "session/state.h"
#include "render/frame_capture.h"
#include "platform/language.h"

static int screenshot_access(const char *path, int mode);
static int screenshot_mkdir(const char *path);
#define _access screenshot_access
#define _mkdir screenshot_mkdir
#include "screenshot_save.inc"
#undef _access
#undef _mkdir

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
char g_clientDir[0x88] = "C:\\screenshot-test";
int g_serviceType = 1;
static const char directory[] = "C:\\screenshot-test\\ScreenShot";
static int directory_exists, mkdir_ok, capture_ok, collisions;
static int accesses, mkdirs, captures, notices, errors;
static char captured_path[512], notice[640], error[640];

__declspec(noreturn) void fatal_error(const char *message)
{
    fprintf(stderr, "Unexpected fatal error: %s\n", message);
    abort();
}

static int screenshot_access(const char *path, int mode)
{
    assert(mode == 0);
    ++accesses;
    if (!strcmp(path, directory)) return directory_exists ? 0 : -1;
    assert(strstr(path, directory) == path);
    if (collisions) { --collisions; return 0; }
    return -1;
}

static int screenshot_mkdir(const char *path)
{
    assert(!strcmp(path, directory));
    ++mkdirs;
    return mkdir_ok ? 0 : -1;
}

char renderer_screenshot(struct Renderer *self, struct String filename)
{
    assert(self == g_renderer);
    ++captures;
    strcpy(captured_path, string_cstr(&filename));
    string_free(&filename);
    return (char)capture_ok;
}

int renderer_text_out(struct Renderer *self, int x, int y, const char *text, COLORREF colour)
{
    /* Foundation still draws its timestamp before capturing. */
    assert(self == g_renderer && text);
    return 1;
}

int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int a, int b, int c, int d)
{
    assert(self == &g_windowMgr && msg == WM_MSG_CHAT_LINE);
    ++notices;
    strcpy(notice, (const char *)a);
    return 0;
}

int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type,
                             int a, int b, int c, const char *title, int w, int h, int d)
{
    assert(self == &g_windowMgr && type == UI_MSGBOX_OK);
    ++errors;
    strcpy(error, text);
    return 0;
}

static void reset(struct Mode *mode)
{
    memset(mode, 0, sizeof *mode);
    mode->screenshot_pending = 1;
    directory_exists = mkdir_ok = capture_ok = 1;
    collisions = accesses = mkdirs = captures = notices = errors = 0;
    captured_path[0] = notice[0] = error[0] = 0;
    g_screenShotCounter = 0;
}

static void assert_consumed(struct Mode *mode)
{
    int calls = accesses + mkdirs + captures + notices + errors;
    assert(!mode->screenshot_pending);
    mode_screen_shot(mode);
    assert(calls == accesses + mkdirs + captures + notices + errors);
}

static void check_directory_failure(void)
{
    struct Mode mode;
    reset(&mode);
    directory_exists = mkdir_ok = 0;
    mode_screen_shot(&mode);
    assert(mkdirs == 1 && !captures && !notices && errors == 1);
    assert(strstr(error, directory));
    assert_consumed(&mode);
}

static void check_capture_failure(void)
{
    struct Mode mode;
    reset(&mode);
    capture_ok = 0;
    mode_screen_shot(&mode);
    assert(captures == 1 && !notices && errors == 1);
    assert(strstr(error, captured_path));
    assert_consumed(&mode);
    /* A subsequent manual request must still work after a failed save. */
    mode.screenshot_pending = capture_ok = 1;
    mode_screen_shot(&mode);
    assert(captures == 2 && notices == 1 && errors == 1);
    assert(!strcmp(captured_path, "C:\\screenshot-test\\ScreenShot\\screenTest_Realm001.jpg"));
    assert_consumed(&mode);
}

static void check_success(void)
{
    struct Mode mode;
    reset(&mode);
    directory_exists = 0;
    collisions = 2;
    mode_screen_shot(&mode);
    assert(mkdirs == 1 && captures == 1 && notices == 1 && !errors);
    assert(!strcmp(captured_path, "C:\\screenshot-test\\ScreenShot\\screenTest_Realm002.jpg"));
    assert(strstr(notice, captured_path) && strstr(notice, " is Saved."));
    assert_consumed(&mode);
    reset(&mode);
    mode.screenshot_pending = 0;
    mode_screen_shot(&mode);
    assert(!accesses && !mkdirs && !captures && !notices && !errors);
}

int main(int argc, char **argv)
{
    string_init(&g_session1.serverName);
    string_assign_cstr(&g_session1.serverName, "Test/Realm");
    renderer.height = 480;
    if (argc > 1 && !strcmp(argv[1], "directory")) {
        check_directory_failure();
    } else {
        check_capture_failure();
        check_directory_failure();
        check_success();
    }
    string_free(&g_session1.serverName);
    puts("Screenshot requests: honest notifications, directory failures, one-shot cleanup, naming and collisions passed");
    return 0;
}
