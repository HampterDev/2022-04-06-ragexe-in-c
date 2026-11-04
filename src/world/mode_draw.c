#include "render/device.h"
#include "contents/sound.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "world/mode.h"

#include <string.h>
#include "render/screen_draw.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

DWORD g_fadeStart = 0;

void mode_run_fade_out(struct Mode *self, int logo)
{
    DWORD elapsed;

    sound_bgm_fade_out();
    g_fadeStart = platform_ticks_ms();
    renderer_save_frame(g_renderer);
    elapsed = platform_ticks_ms() - g_fadeStart;
    while (elapsed < 0xff) {
        draw_box_screen(0, 0, g_renderer->width, g_renderer->height, elapsed << 24);
        if (logo) {
            void *bmp = res_mgr_get(g_resMgr(), "rag_logo.bmp");

            ui_window_mgr_draw_picture(&g_windowMgr, g_renderer->width - 0xc0, g_renderer->height - 0x7a, bmp);
        }
        if (renderer_draw_scene(g_renderer))
            renderer_flip(g_renderer, 1);
        renderer_restore_frame(g_renderer);
        elapsed = platform_ticks_ms() - g_fadeStart;
    }
}

void mode_set_cursor_action(struct Mode *self, int action)
{
    if (self->cursor_action == action)
        return;
    self->cursor_action = action;
    self->cursor_motion = 0;
    self->cursor_anim_start = platform_ticks_ms();
    ui_window_mgr_send_msg_(&g_windowMgr, WM_MSG_CURSOR_ACTION, self->cursor_action, 0, 0, 0);
}
