#define WIN32_LEAN_AND_MEAN
#include "platform/client_config.h"
#include "platform/settings.h"
#include "platform/win_types.h"
#include <string.h>
#include "render/gfx.h"
#include "core/app.h"

GUID g_driverGuid;

static int guid_attached;

static BOOL match_adapter(GUID *guid, char *desc, char *name, void *ctx, HMONITOR monitor)
{
    (void)desc; (void)name; (void)ctx; (void)monitor;
    if (guid && memcmp(guid, &g_driverGuid, sizeof(GUID)) == 0)
        guid_attached = 1;
    return TRUE;
}

int stale_guid(void)
{
    static const GUID zero;
    if (memcmp(&g_driverGuid, &zero, sizeof(GUID)) != 0) {
        guid_attached = 0;
        gfx_ddraw_enumerate(match_adapter, NULL, 7);
        if (!guid_attached) {
            memset(&g_driverGuid, 0, sizeof(GUID));
            {
                struct PlatformSettingsKey *key;
                if (platform_settings_create(g_registryKeyPath, PLATFORM_SETTINGS_SET, &key)) {
                    platform_settings_write(key, "GUIDDRIVER", PLATFORM_SETTING_BINARY, &g_driverGuid, sizeof(GUID));
                    platform_settings_close(key);
                }
            }
            MessageBoxA(NULL,
                        "The display device this client had saved is no longer available, so it has "
                        "been reset to your primary display.\r\n\r\n"
                        "Choose a display device in the in-game graphics settings.",
                        "Display device reset", MB_ICONEXCLAMATION);
        }
    }
    return g_isVoodoo;
}
