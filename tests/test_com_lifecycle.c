/* Exact production control flow, with controlled COM results and subsystem
 * failures. The extractor leaves the startup tail and app_last_pump intact. */
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <string.h>

enum Failure { NO_FAILURE, SOUND, WINDOW, MANAGERS, DEVICE, RENDERER, CONNECTION };
struct InitResult { int ok, what; };
struct FakeRenderer { int width, height; };
static struct FakeRenderer renderer = {1024, 768}, *g_renderer;
static struct { int fullscreen; } g_3dDevice;
static struct { const char *launchArg3; } g_session1;
static int g_replayLaunch, g_loginStartMode, g_openLauncherPageAtExit, g_serverType;
static HWND g_hMainWnd;
static HRESULT com_status;
static enum Failure failure;
static int init_calls, uninit_calls, balance, frees, shell_calls, failures;
static const char *case_name;

static void check(int ok, const char *message)
{
    if (!ok) {
        printf("FAIL %s / HRESULT %08lx / stage %d: %s\n",
               case_name, (unsigned long)com_status, failure, message);
        ++failures;
    }
}

static void check_com_alive(void)
{
    check(balance == (SUCCEEDED(com_status) ? 1 : 0),
          "COM ownership changed before consumers finished");
}

static HRESULT fake_initialize(void *reserved)
{
    check(reserved == NULL, "unexpected COM initialization argument");
    ++init_calls;
    if (SUCCEEDED(com_status)) ++balance;
    return com_status;
}

static void fake_uninitialize(void)
{
    ++uninit_calls;
    --balance;
}

static int fake_window(HINSTANCE instance)
{
    (void)instance;
    return failure != WINDOW;
}

static void fake_managers(struct InitResult *init)
{
    init->ok = failure != MANAGERS;
    init->what = 123;
}

static void fake_free(int *what)
{
    check(*what == 123, "freed an uninitialized InitResult string");
    ++frees;
}

static void fake_shell(void)
{
    check_com_alive();
    ++shell_calls;
}

/* Keep these operations observable; the other subsystems are inert. */
#define platform_desktop_start() SUCCEEDED(fake_initialize(NULL))
#define platform_desktop_stop fake_uninitialize
#define sound_mgr_create() (failure != SOUND)
#define app_create_window fake_window
#define app_init_managers fake_managers
#define app_init_3d() (failure != DEVICE)
#define c3d_device_create_renderer(...) (failure == RENDERER ? NULL : &renderer)
#define connection_startup() (failure != CONNECTION)
#define mode_mgr_run(...) check_com_alive()
#define pump_messages() check_com_alive()
#define window_drag_follow() ((void)0)
#define DestroyWindow(...) check_com_alive()
#define app_destroy_singletons() check_com_alive()
#define sound_mgr_destroy() check_com_alive()
#define async_work_mgr_destroy() check_com_alive()
#define ShellExecuteA(...) fake_shell()
#define string_free fake_free
#define string_cstr(...) "startup failure"
#define msg_str(...) "https://example.com"
#define error_msg(...) ((void)0)
#include "platform/window.h"
#define MessageBoxA(...) ((void)0)
#define platform_message_box(...) ((void)0)
#define frame_timer_reset() ((void)0)
#define frame_timer_start(...) ((void)0)
#define session_startup(...) ((void)0)
#define res_mgr_read_res_name_table(...) ((void)0)
#define SetFocus(...) ((void)0)
#define ShowCursor(...) ((void)0)
#define ui_window_mgr_set_screen_size(...) ((void)0)
#define ui_window_mgr_init_ui_rect_info_from_reg(...) ((void)0)
#define granny_animation_cache_init(...) ((void)0)
#define granny_animation_cache_load(...) ((void)0)
#define skin_mgr_scan(...) ((void)0)
#define math_init_degree_tables() ((void)0)
#define window_mgr_clear_backdrop(...) ((void)0)
#define connection_disconnect(...) ((void)0)
#define connection_shutdown() ((void)0)
#define ui_window_mgr_close_all_windows(...) ((void)0)
#define ui_window_mgr_write_ui_rect_info_to_reg(...) ((void)0)
#define session_save_options(...) ((void)0)
#define map_info_destroy() ((void)0)
#define res_mgr_delete_all(...) ((void)0)
#define texcache_clear(...) ((void)0)
#define c3d_device_delete_renderer(...) ((void)0)
#define c3d_device_release_all(...) ((void)0)
#define platform_timer_period_end(...) ((void)0)
#define crash_report_uninstall() ((void)0)
#define app_is_win9x() 0
#define create_depends_dll_msg() ((void)0)
#define i_process_construct() ((void)0)
#define gameguard_set_window(...) ((void)0)
#define choose_login_start() ((void)0)
#define FindFirstFileA(...) INVALID_HANDLE_VALUE
#define CreateDirectoryA(...) ((void)0)
#define FindClose(...) ((void)0)
#define app_finish_async_work() ((void)0)
#define delete_hack_files() ((void)0)

#include "com_lifecycle_functions.inc"

static void run_case(HRESULT status, enum Failure fail, int exit_page)
{
    int result;
    com_status = status;
    failure = fail;
    init_calls = uninit_calls = balance = frees = shell_calls = 0;
    g_openLauncherPageAtExit = exit_page != 0;
    g_session1.launchArg3 = exit_page == 1 ? "Dev" : "";
    case_name = exit_page ? "foundation exit page" : "startup/shutdown";
    result = run_startup();
    check(result == (fail != NO_FAILURE && fail != SOUND), "changed process result");
    check(init_calls == 1, "expected one initialization attempt");
    check(uninit_calls == (SUCCEEDED(status) ? 1 : 0), "unbalanced COM cleanup count");
    check(balance == 0, "COM ownership leaked or over-released");
    check(frees == (fail != SOUND && fail != WINDOW), "incorrect string cleanup count");
    check(shell_calls == (TEST_LEGACY_EXIT && exit_page != 0), "exit page path was skipped");
}

int main(void)
{
    static const HRESULT statuses[] = {S_OK, S_FALSE, RPC_E_CHANGED_MODE, E_FAIL};
    unsigned i;
    int stage, count = 0;
    for (i = 0; i < sizeof statuses / sizeof statuses[0]; ++i) {
        for (stage = NO_FAILURE; stage <= CONNECTION; ++stage) {
            run_case(statuses[i], (enum Failure)stage, 0);
            ++count;
        }
        if (TEST_LEGACY_EXIT) {
            run_case(statuses[i], NO_FAILURE, 1);
            run_case(statuses[i], NO_FAILURE, 2);
            count += 2;
        }
    }
    printf("COM lifecycle: %d cases, %d failures\n", count, failures);
    return failures ? 1 : 0;
}
