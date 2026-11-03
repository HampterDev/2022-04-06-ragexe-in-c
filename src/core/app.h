#include "platform/client_options.h"
#include "core/window_state.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#ifndef CORE_APP_H
#define CORE_APP_H

#include "platform/client_config.h"

#include "platform/win_types.h"
#include "platform/window_handle.h"
#include "std/std.h"
#include "core/client.h"

extern HINSTANCE g_hInstance;
extern int g_quitRequested;
extern PlatformWindow g_hMainWnd;
extern char g_clientDir[0x88];
extern int g_windowActive;
extern int g_fullscreenMode;
extern unsigned char g_windowDragging;
extern RECT g_windowDragRect;
extern POINT g_windowDragCursor;
extern HWND g_windowDragHwnd;

extern DWORD g_isVoodoo;
extern char g_deviceName[0x28];
extern GUID g_deviceGuid;

void check_system_message(void);
void app_last_pump(void);
int app_create_window(HINSTANCE instance);
void app_destroy_singletons(void);

struct InitResult {
    unsigned char ok;
    struct String what;
};

void read_startup_settings(void);
void app_activate(int active);
int app_init_3d(void);
struct InitResult *app_init_managers(struct InitResult *out);

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

extern int g_activatedNow;
extern int g_loginStartMode;

#endif
