#include "render/texture_cache.h"
#include "render/display_devices.h"
#include "render/render_api.h"
#include "std/list.h"
#include <stdlib.h>
#include <string.h>
#include "platform/window.h"

RECT g_monitorRect;

static BOOL ddraw_enum_adapter_callback(GUID *guid, char *desc, char *name, void *ctx, HMONITOR monitor)
{
    (void)name;
    (void)ctx;
    if (guid && monitor && memcmp(guid, &g_driverGuid, sizeof(GUID)) == 0) {
        platform_monitor_rect(monitor, &g_monitorRect);
    }
    gfx_enum_adapter(guid, desc, monitor);
    return 1;
}

int render_enum_adapters(void)
{
    display_devices_reset();
    gfx_ddraw_enumerate(ddraw_enum_adapter_callback, NULL, 7);
    return display_devices_result();
}

char render_quads_to_texture(int w, int h, const struct List *quads, struct Texture **out)
{
    gfx_surface *rt, *old = NULL;
    DWORD oldZ = 0, oldFog = 0;
    char ok;

    if (!*out) {
        *out = texture_create(w, h, GFX_FMT_8888);
        if (!*out)
            return 0;
    }
    rt = gfx_create_render_target(w, h);
    if (!rt) {
        ok = 0;
    } else if ((old = gfx_get_render_target()) == NULL
               || gfx_get_render_state(GFX_RS_ZENABLE, &oldZ) != 0
               || gfx_get_render_state(GFX_RS_FOGENABLE, &oldFog) != 0) {
        ok = 0;
    } else {
        if (gfx_set_render_target(rt) == 0) {
            gfx_set_render_state(GFX_RS_ZENABLE, 0);
            gfx_set_render_state(GFX_RS_FOGENABLE, 0);
            gfx_clear(GFX_CLEAR_TARGET, 0, 0.0f, NULL);
            if (gfx_begin_scene() == 0) {
                struct ListNode *n;
                RECT rc;
                for (n = quads->head->next; n != quads->head; n = n->next) {
                    struct RPQuad *q = *LIST_VALUE(n, struct RPQuad *);
                    gfx_set_texture(0, q->texture ? q->texture->base.surface : NULL);
                    rp_quad_draw(q);
                }
                gfx_end_scene();
                rc.left = 0;
                rc.top = 0;
                rc.right = w;
                rc.bottom = h;
                texture_copy_rect_from(*out, rt, &rc);
            }
            gfx_set_render_target(old);
            gfx_set_render_state(GFX_RS_ZENABLE, oldZ);
            gfx_set_render_state(GFX_RS_FOGENABLE, oldFog);
        }
        ok = 1;
    }
    if (old)
        gfx_release(old);
    if (rt)
        gfx_release(rt);
    return ok;
}
