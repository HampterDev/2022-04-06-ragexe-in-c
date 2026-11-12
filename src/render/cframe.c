#include "render/render_api.h"
#include "render/screen_wobble.h"
#include "session/session_names.h"
#include <stdlib.h>

int   g_wobbleFramesLeft;
BYTE  g_wobbleMode;
short g_wobbleAmplitude;

/* the wobble's random numbers: the C runtime's, which the rest of the game draws from too */
static int wobble_rand(void *context)
{
    (void)context;
    return rand();
}

struct WobbleRows {
    struct WobbleRow *rows;
    int height;
};

/* One frame of the wobble over the renderer's rows, advancing its globals: called by the device
   only once it can draw the wobble, so that a frame it cannot draw it on draws no random numbers
   and advances nothing, as the original's frame that did not lock. */
static void wobble_rows(void *context)
{
    struct WobbleRows *w = context;
    struct Wobble wobble;
    wobble.framesLeft = g_wobbleFramesLeft;
    wobble.amplitude = g_wobbleAmplitude;
    wobble_frame(&wobble, w->height, g_wobbleMode, wobble_rand, NULL, w->rows);
    g_wobbleFramesLeft = wobble.framesLeft;
    g_wobbleAmplitude = wobble.amplitude;
}

void renderer_draw_frame_extras(struct Renderer *self)
{
    if (g_session1.showTimeFont != 0) {
        int unit = self->height / 700;
        void *act = res_mgr_get(g_resMgr(), "이팩트\\TimeFont.act");
        void *spr = res_mgr_get(g_resMgr(), "이팩트\\TimeFont.spr");
        screen_sprite_frame(self, 100, 100, act, spr, 0, 0, (float)unit * 0.5f, 0, -1, 0x201);
        screen_sprite_frame(self, 100, 200, act, spr, 1, 0, (float)unit, 0, -1, 0x201);
    }
    if (session_get_battlefield_state(&g_session1) == 1)
        renderer_draw_battle_field_icon(self);
    if (g_session1.effectOn != 0 && g_wobbleFramesLeft > 0 && g_wobbleMode != 2) {
        /* custom: the frame drawn so far is drawn again by the device with its rows moved
           (render/screen_wobble.h), where the original locked the frame and moved them on the CPU */
        struct WobbleRows w;
        w.height = self->height;
        w.rows = self->height > 0 ? malloc((size_t)self->height * sizeof *w.rows) : NULL;
        if (w.rows) {
            gfx_wobble(w.rows, w.height, wobble_rows, &w);
            free(w.rows);
        }
    }
}
