#include "ui/window_types.h"
#include "resources/state_icon_sheet.h"
#include "diag/diag.h"
#include "ui/window_access.h"
#include "ui/wndmgr/wndmgr.h"
#include "render/screen_draw.h"
#include "ui/character/character.h"
#include "ui/misc/roulette_state.h"
#include "ui/worldmap/romap.h"
#include "ui/shops/roulette.h"

#define CHECKPOINT(line) \
    (g_renderCheckpointLine = (line), g_renderCheckpointFile = "c:\\repositories\\client\\kr\\kr.ro1.live\\source\\ragnarokclient\\uiwindowmgr.cpp")

#include "render/cdc.h"
#include "render/surface.h"

#define WND_ID_SHOP_TITLE 0xbd
#define WND_ID_BASIC_BUTTONS 0xb6

struct UIRoMapWnd;
struct UIButtonsOfBasicWnd;

void ui_window_mgr_render_windows(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *w = *LIST_VALUE(node, struct UIWindow *);
        struct UIBackdropBox box;

        if (!w->visible)
            continue;
        CHECKPOINT(0xd54);
        if (w->id == WND_ID_SHOP_TITLE) {
            ui_soft_next(1);
            ui_window_invalidate(w);
            ui_soft_next(0);
        }
        CHECKPOINT(0xd70);
        ui_window_render(w, 0);
        CHECKPOINT(0xd74);
        if (ui_window_get_backdrop_box(w, &box) == 1) {
            draw_box_screen(box.x, box.y, box.width, box.height, box.argb);
            CHECKPOINT(0xd7c);
        } else if (w->ops->has_custom_backdrop) {
            ui_window_draw_backdrop(w);
            CHECKPOINT(0xd82);
        }

        if (w == self->kafra_move_map_service_info_wnd)
            ui_ro_map_wnd_render_overlay((struct UIRoMapWnd *)w);
        CHECKPOINT(0xd88);
        if (w == ui_window_mgr_get_window(self, WND_ID_BASIC_BUTTONS)) {
            struct UIWindow *buttons = ui_window_mgr_get_window(self, WND_ID_BASIC_BUTTONS);

            if (buttons)
                ui_buttons_of_basic_wnd_render_buttons((struct UIButtonsOfBasicWnd *)buttons);
        } else if (w->composites == 1) {
            if (w->texture)
                texture_draw_surface(w->texture, w->x, w->y, w->width, w->height, ui_window_get_color(w));
            CHECKPOINT(0xd9b);
        } else {
            if (w->canvas)
                dc_bitmap_draw(w->canvas, w->x, w->y, w->width, w->height, ui_window_get_color(w));
            CHECKPOINT(0xda1);
        }

        ui_window_flush_quads(w);
        CHECKPOINT(0xdab);
        icon_sheet_flush_bars(g_stateIconSheet);
        CHECKPOINT(0xdb1);
        if (w == self->roullette_wnd)
            roulette_render(g_roulette);
        CHECKPOINT(0xdb8);
        ui_window_render_overlay(w);
        CHECKPOINT(0xdbc);
    }
    CHECKPOINT(0xdc5);
}
