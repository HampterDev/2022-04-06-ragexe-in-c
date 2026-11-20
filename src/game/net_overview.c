#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/net_overview.h"
#include "game/cells.h"
#include "game/net_clock.h"
#include "render/cdc.h"
#include "render/renderer.h"
#include "render/textdc.h"

/* The overview's bitmap: wide and tall enough for its two lines in the 12-pixel font. */
#define OVERVIEW_WIDTH 560
#define OVERVIEW_HEIGHT 34
#define OVERVIEW_LINE 16

static int overview_shown(void)
{
    static int shown = -1;

    if (shown < 0) {
        const char *text = getenv("RAGEXE_NET_OVERVIEW");

        shown = text && text[0] && text[0] != '0';
    }
    return shown;
}

/* White on a black outline, as the login screen's serial number is drawn. */
static void overview_line(struct TextDC *dc, int y, const char *text)
{
    static const int outline[4][2] = { { 2, 1 }, { 0, 1 }, { 1, 2 }, { 1, 0 } };
    int length = (int)strlen(text), i;

    text_dc_set_text_color(dc, 0);
    for (i = 0; i < 4; i++)
        text_dc_text_out_plain(dc, outline[i][0], y + outline[i][1], text, length);
    text_dc_set_text_color(dc, 0xffffff);
    text_dc_text_out_plain(dc, 1, y + 1, text, length);
}

/* custom: the lines are written into a bitmap each frame and shown as a quad over everything the
   frame holds (dc_bitmap_draw_on_top), as the login screen's serial number is; the renderer draws
   no text into the frame. A bitmap that could not be made is not tried again. */
void game_mode_draw_net_overview(struct GameMode *self)
{
    static struct DcBitmap *bitmap;
    static int failed;
    struct GameActor *own = self->world ? (struct GameActor *)self->world->player : NULL;
    struct TextDC dc;
    char text[160];
    DWORD tick;
    int x = 0, y = 0;

    if (!overview_shown() || !g_renderer || failed)
        return;
    if (!bitmap) {
        bitmap = dc_bitmap_create(OVERVIEW_WIDTH, OVERVIEW_HEIGHT);
        if (!bitmap || bitmap->width != OVERVIEW_WIDTH || bitmap->height != OVERVIEW_HEIGHT) {
            if (bitmap)
                dc_bitmap_destroy(bitmap);
            bitmap = NULL;
            failed = 1;
            return;
        }
    }
    dc_bitmap_fill(bitmap, NULL, CDC_COLOR_KEY);
    text_dc_construct(&dc, bitmap);
    text_dc_select_font(&dc, 0, 12, 0, 0);
    tick = game_mode_true_time(self);
    snprintf(text, sizeof text, "tick %lu   own +%d   others -%d   round trip %u", (unsigned long)tick,
             (int)(game_mode_own_time(self) - tick), (int)(tick - session_get_server_time(&g_session1)),
             self->clock_sync.round_trip);
    overview_line(&dc, 0, text);
    if (own)
        game_mode_world_to_cell(self, own->pos.x, own->pos.z, &x, &y);
    snprintf(text, sizeof text, "request %u   move %u   intent %u   cell %d,%d   state %d", (unsigned)self->input_seq,
             (unsigned)self->move_seq, (unsigned)self->intent_seq, x, y, own ? own->state : -1);
    overview_line(&dc, OVERVIEW_LINE, text);
    text_dc_destruct(&dc);
    dc_bitmap_draw_on_top(bitmap, g_renderer->width / 2 - 170 - 1, 4 - 1, bitmap->width, bitmap->height,
                          0xffffffff);
}
