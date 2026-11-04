#include "session/option_pairs.h"
#include "render/device.h"
#include "platform/client_options.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "contents/connection_api.h"
#include "platform/system.h"
#include "platform/win_types.h"
#include "std/std.h"
#include "render/render_api.h"
#include "core/app.h"
#include "core/client_managers.h"
#include "platform/language.h"
#include "world/world_imports.h"
#include "session/session_state.h"
#include "ui/render/graphic_setting.h"
#include "contents/sound.h"
#include "platform/window.h"

PlatformWindow g_hMainWnd;

void app_activate(int active)
{
    g_windowActive = active;
    if (active) {
        platform_set_background_priority(0);
        if (g_session1.mouseExclusiveOption != 1)
            return;
        platform_clip_cursor_to_client(g_hMainWnd, 1);
    } else {
        struct IntInt64Pair *var;
        DWORD packet;
        int key = 2, flag;

        platform_set_background_priority(1);
        var = tree_find(&g_session1.instantVars, &key, tree_less_int);
        flag = var ? (int)var->value : 0;
        if (flag) {
            packet = 0x07f40b11;
            rag_connection_send_packet(rag_connection_instance(), 4, &packet);
            session_set_pair(&g_session1, 2, 0);
        }
    }
}

int app_init_3d(void)
{
    struct SurfaceDesc mode;
    int flags = g_fullscreenMode ? 7 : 6;

    mode.refreshRate = 0;
    mode.width = g_displayWidth;
    mode.height = g_displayHeight;
    mode.bitCount = g_bitsPerPixel;
    if (c3d_device_init(&g_3dDevice, g_hMainWnd, &g_driverGuid, &g_deviceGuid, &mode, flags) < 0) {
        error_msg("Cannot init d3d OR grf file has problem.", 0);
        return 0;
    }
    return 1;
}

static struct InitResult *init_failed(struct InitResult *out, const char *what)
{
    out->ok = 0;
    string_init(&out->what);
    string_assign_cstr(&out->what, what);
    return out;
}

/* Creates the client's managers (g_clientManagers, core/client_managers.c) in their list order.
   A manager whose failure stops startup ends it with its text; the managers created before it
   stay (see manager_table.h). */
struct InitResult *app_init_managers(struct InitResult *out)
{
    const struct ManagerRow *failed = manager_table_create(&g_clientManagers);

    if (failed)
        return init_failed(out, failed->on_failure.text);
    out->ok = 1;
    string_init(&out->what);
    string_assign_cstr(&out->what, "good");
    return out;
}
