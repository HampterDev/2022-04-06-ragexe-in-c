#include "render/texture_cache.h"
#include "render/render_api.h"
#include "std/list.h"
#include <stdlib.h>

/* The quads drawn into *out, a window's texture whose pixels are a render target of w by h pixels
   (texture_create_target), made when NULL or of another size, in the interface's composite mode
   (render/draw_modes.h, DRAW_COMPOSITE). The window's quad then reads that target as it reads any
   texture; nothing comes back to the CPU. Where no target of that size can be made, *out stays the
   texture it was and nothing is drawn (0). */
char render_quads_to_texture(int w, int h, const struct List *quads, struct Texture **out)
{
    gfx_surface *old;

    if (w <= 0 || h <= 0)
        return 0;
    if (!*out || (*out)->base.width != w || (*out)->base.height != h) {
        struct Texture *made = texture_create_target(w, h);
        if (!made)
            return 0;
        if (*out)
            texture_destroy(*out);
        *out = made;
    }
    old = gfx_get_render_target();
    if (!old)
        return 0;
    if (gfx_set_render_target((*out)->base.surface) == 0) {
        gfx_set_mode(DRAW_COMPOSITE);
        gfx_clear(GFX_CLEAR_TARGET, 0, 0.0f, NULL);
        if (gfx_begin_scene() == 0) {
            struct ListNode *n;
            for (n = quads->head->next; n != quads->head; n = n->next) {
                struct RPQuad *q = *LIST_VALUE(n, struct RPQuad *);
                gfx_set_texture(0, q->texture ? q->texture->base.surface : NULL);
                rp_quad_draw(q);
            }
            gfx_end_scene();
        }
        gfx_set_render_target(old);
    }
    gfx_release(old);
    return 1;
}
