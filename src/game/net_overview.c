#include <stdio.h>
#include <stdlib.h>
#include "game/net_overview.h"
#include "game/cells.h"
#include "game/net_clock.h"
#include "render/renderer.h"

static int overview_shown(void)
{
    static int shown = -1;

    if (shown < 0) {
        const char *text = getenv("RAGEXE_NET_OVERVIEW");

        shown = text && text[0] && text[0] != '0';
    }
    return shown;
}

/* White on a black outline, as the serial number of the login screen is drawn. */
static void overview_line(int x, int y, const char *text)
{
    renderer_text_out(g_renderer, x + 1, y, text, 0);
    renderer_text_out(g_renderer, x - 1, y, text, 0);
    renderer_text_out(g_renderer, x, y + 1, text, 0);
    renderer_text_out(g_renderer, x, y - 1, text, 0);
    renderer_text_out(g_renderer, x, y, text, 0xffffff);
}

void game_mode_draw_net_overview(struct GameMode *self)
{
    struct GameActor *own = self->world ? (struct GameActor *)self->world->player : NULL;
    char text[160];
    DWORD tick;
    int x = 0, y = 0;

    if (!overview_shown() || !g_renderer)
        return;
    tick = game_mode_true_time(self);
    snprintf(text, sizeof text, "tick %lu   own +%d   others -%d   round trip %u", (unsigned long)tick,
             (int)(game_mode_own_time(self) - tick), (int)(tick - session_get_server_time(&g_session1)),
             self->clock_sync.round_trip);
    overview_line(g_renderer->width / 2 - 170, 4, text);
    if (own)
        game_mode_world_to_cell(self, own->pos.x, own->pos.z, &x, &y);
    snprintf(text, sizeof text, "request %u   move %u   intent %u   cell %d,%d   state %d", (unsigned)self->input_seq,
             (unsigned)self->move_seq, (unsigned)self->intent_seq, x, y, own ? own->state : -1);
    overview_line(g_renderer->width / 2 - 170, 20, text);
}
