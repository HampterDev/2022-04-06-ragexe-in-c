#include "platform/client_options.h"
#include "core/window_state.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#ifndef CORE_APP_H
#define CORE_APP_H

#include "platform/client_config.h"

#include <windows.h>
#include "std/std.h"
#include "core/client.h"

extern HINSTANCE g_hInstance;
extern int g_quitRequested;
extern HWND g_hMainWnd;
extern char g_clientDir[0x88];
extern int g_windowActive;
extern int g_fullscreenMode;
extern unsigned char g_windowDragging;
extern RECT g_windowDragRect;
extern POINT g_windowDragCursor;
extern HWND g_windowDragHwnd;

extern const char *g_setupRegistryKeyPath;
extern DWORD g_isVoodoo;
extern char g_deviceName[0x28];
extern GUID g_deviceGuid;

void check_system_message(void);
void app_last_pump(void);
int app_create_window(HINSTANCE instance);
unsigned char app_is_win9x(void);
HANDLE app_run(const char *cmd);
void app_swap_ai_folder(void);
void app_destroy_singletons(void);
void app_finish_async_work(void);

struct InitResult {
    unsigned char ok;
    struct String what;
};

int read_registry(void);
char create_depends_dll_msg(void);
char i_process_construct(void);
void app_activate(int active);
int app_init_3d(void);
struct InitResult *app_init_managers(struct InitResult *out);

unsigned char select_package_create(void);
void select_package_free(void);

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

extern unsigned char g_openLauncherPageAtExit;
extern int g_activatedNow;
extern int g_loginStartMode;

#endif
