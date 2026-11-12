#include "render/texture_cache.h"
#include "render/render_api.h"
#include "std/list.h"
#include <stdlib.h>

/* The quads drawn into a render target w by h pixels and copied into *out, made when NULL, in the
   interface's composite mode (render/draw_modes.h, DRAW_COMPOSITE). */
char render_quads_to_texture(int w, int h, const struct List *quads, struct Texture **out)
{
    gfx_surface *rt, *old = NULL;
    char ok;

    if (!*out) {
        *out = texture_create(w, h, GFX_FMT_8888);
        if (!*out)
            return 0;
    }
    rt = gfx_create_render_target(w, h);
    if (!rt) {
        ok = 0;
    } else if ((old = gfx_get_render_target()) == NULL) {
        ok = 0;
    } else {
        if (gfx_set_render_target(rt) == 0) {
            gfx_set_mode(DRAW_COMPOSITE);
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
        }
        ok = 1;
    }
    if (old)
        gfx_release(old);
    if (rt)
        gfx_release(rt);
    return ok;
}
