#include "world/tooltip.h"
#include "world/mode_types.h"
#include "core/memory.h"
#include "session/session_names.h"
#include "render/renderer.h"
#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "ui/window_access.h"
#include "ui/window_manager_state.h"
#include <mmsystem.h>
#include <string.h>

void mode_show_tooltip(struct Mode *self, const char *text, int x, int y,
                                      DWORD colour, BYTE flag)
{
    char buf[0x400];
    struct UIBalloonText *balloon;
    DWORD blue = colour & 0xff;
    DWORD red = (colour >> 16) & 0xff;
    int maxX, maxY;

    if (strlen(text) == 0)
        return;
    session_remove_underbar(text, buf);
    self->help_balloon_tick = timeGetTime();
    if (!self->help_balloon) {
        void *p = mem_alloc(sizeof(struct UITransBalloonText));
        self->help_balloon = p ? ui_trans_balloon_text_construct(p) : NULL;
        if (!self->help_balloon)
            return;
        ui_window_create(self->help_balloon, 8, 8);
        ui_window_mgr_add_window(&g_windowMgr, self->help_balloon);
    } else {
        ui_window_mgr_raise_window(&g_windowMgr, self->help_balloon);
    }
    balloon = (struct UIBalloonText *)self->help_balloon;
    balloon->colour_codes = flag;
    ui_balloon_text_set_text(balloon, buf, 0x23);
    balloon = (struct UIBalloonText *)self->help_balloon;
    balloon->colour = (colour & 0xff00) | (blue << 16) | red;
    balloon->outline = 0;
    ((struct UITransBalloonText *)self->help_balloon)->backdrop.argb = 0x88000000;

    balloon = (struct UIBalloonText *)self->help_balloon;
    if (x < -3)
        x = -3;
    if (y < -3)
        y = -3;
    maxX = g_renderer->width - balloon->base.width + 3;
    maxY = g_renderer->height - balloon->base.height + 3;
    ui_window_move(&balloon->base, x <= maxX ? x : maxX, y <= maxY ? y : maxY);
}

void mode_hide_help_balloon(struct Mode *self)
{
    if (self->help_balloon) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->help_balloon);
        self->help_balloon = NULL;
        self->help_balloon_tick = timeGetTime();
    }
}

void mode_process_help_balloon(struct Mode *self)
{
    if (self->help_balloon && timeGetTime() >= self->help_balloon_tick + 100) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->help_balloon);
        self->help_balloon = NULL;
    }
}
