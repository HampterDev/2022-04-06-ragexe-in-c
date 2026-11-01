#include "ui/window_messages.h"
#include "platform/client_config.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <windows.h>
#include <string.h>
#include "base.h"
#include "core/app.h"
#include "core/msg_str.h"

int g_limiterBudget;
int g_limiterOvershoot;
int g_perfFrequencyRead;

struct Scsk {
    int loaded;
    int started;
    HMODULE dll;
    int (*load_scsk)(void);
    int (*start_scsk)(int arg);
    int (*stop_scsk)(void);
    int (*get_real_ascii)(void);
    int service;
};

struct Scsk *g_scsk;

typedef void *(__stdcall *QueryIMEHelperFn)(DWORD id, const char *version);

void *g_secureGamePack;
