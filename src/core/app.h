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

extern char g_clientDir[0x88];

void check_system_message(void);
void app_last_pump(void);
int app_create_window(void);
void app_destroy_singletons(void);

struct InitResult {
    unsigned char ok;
    struct String what;
};

void read_startup_settings(void);
void app_activate(int active);
int app_init_3d(void);
struct InitResult *app_init_managers(struct InitResult *out);

extern int g_loginStartMode;

#endif
