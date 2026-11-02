#include "ui/controls/listbox.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "session/session_items.h"

#define LIST_MSG_SELECT_PREV 0x12
#define LIST_MSG_SELECT_NEXT 0x13
#define LIST_MSG_SELECT 0x28
#define LIST_MSG_ITEM_LINK 0x62
#define WND_ITEM_COLLECTION 0xc
#define WND_ITEM_COLLECTION_CMP 0xea
#define UI_MSG_COLLECTION_ITEM 0x18
#define H_PAGE_STEP 0xf

#define LINE_COUNT(lb) ((int)VEC_COUNT(&(lb)->items, struct String))

static void redraw_after_scroll(struct UIListBox *self, struct UIScrollBar *bar)
{
    ui_window_invalidate_base(&bar->base);
    ui_window_invalidate_base(&self->base);
    if (self->transparent && self->base.parent)
        ui_window_invalidate(self->base.parent);
}

static void scroll_lines(struct UIListBox *self, int lines)
{
    ui_window_send_msg(&self->base, NULL, UI_MSG_SCROLL_V, lines, 0, 0, 0);
}

static int selection_shown(struct UIListBox *self, int last_in_view)
{
    if (self->selected > last_in_view)
        scroll_lines(self, 1);
    ui_window_invalidate_base(&self->base);
    ui_list_box_refresh_scroll(self);
    return 0;
}

intptr_t ui_list_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                 intptr_t val3, intptr_t val4)
{
    struct UIListBox *self = (struct UIListBox *)window;
    int lines_in_view, last;

    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN: {
        int old = self->top, top, max;

        lines_in_view = self->base.height / self->line_height;
        if (msg == UI_MSG_PAGE_UP)
            top = old - lines_in_view + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            top = old - 1 + lines_in_view;
        else
            top = old + val1;
        if (top < 0)
            top = 0;
        max = LINE_COUNT(self) - lines_in_view + self->extra_lines;
        if ((unsigned)top > (unsigned)max)
            top = max;
        self->top = top;
        self->vbar->pos = top;
        redraw_after_scroll(self, self->vbar);
        return self->top - old;
    }
    case UI_MSG_SCROLL_H:
    case UI_MSG_PAGE_LEFT:
    case UI_MSG_PAGE_RIGHT: {
        int old = self->scroll_x, x, max;

        if (msg == UI_MSG_PAGE_LEFT)
            x = old - H_PAGE_STEP;
        else if (msg == UI_MSG_PAGE_RIGHT)
            x = old + H_PAGE_STEP;
        else
            x = old + val1;
        if (x < 0)
            x = 0;
        max = self->widest + (self->has_vscroll << 4) - self->base.width;
        if (x > max)
            x = max;
        self->scroll_x = x;
        self->hbar->pos = x;
        redraw_after_scroll(self, self->hbar);
        return self->scroll_x - old;
    }
    case LIST_MSG_SELECT_PREV:
        if (--self->selected < 0)
            self->selected = 0;
        if (self->selected < self->top)
            scroll_lines(self, -1);
        ui_window_invalidate_base(&self->base);
        ui_list_box_refresh_scroll(self);
        return 0;
    case LIST_MSG_SELECT_NEXT:
        self->selected++;
        last = LINE_COUNT(self) - 1;
        if ((unsigned)self->selected > (unsigned)last)
            self->selected = last;
        return selection_shown(self, self->top - 1 + self->base.height / self->line_height);
    case LIST_MSG_SELECT:
        self->selected = val1 < 0 ? 0 : val1;
        last = LINE_COUNT(self) - 1;
        if ((unsigned)self->selected > (unsigned)last)
            self->selected = last;
        if (self->selected < self->top)
            scroll_lines(self, -1);
        return selection_shown(self, self->base.height / self->line_height - 1 + self->top);
    case LIST_MSG_ITEM_LINK:
        if (val3) {
            struct UIWindow *wnd;
            struct ItemInfo *worn;

            wnd = ui_window_open(WND_ITEM_COLLECTION);
            ui_window_send(wnd, NULL, UI_MSG_COLLECTION_ITEM, val3, 0, 0, 0);
            if (g_windowMgr.item_collection_wnd) {
                worn = session_get_worn_counterpart(&g_session1, (const struct ItemInfo *)val3);
                if (g_session1.itemCompare && worn && !val4) {
                    wnd = ui_window_open(WND_ITEM_COLLECTION_CMP);
                    if (wnd) {
                        ui_window_send(wnd, NULL, UI_MSG_COLLECTION_ITEM, (intptr_t)worn, 1, 0, 0);
                        return 0;
                    }
                    ui_window_mgr_post_quit1(&g_windowMgr, WND_ITEM_COLLECTION_CMP);
                }
            }
        }
        return 0;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}
