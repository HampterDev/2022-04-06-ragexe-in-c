#include "ui/window_manager_state.h"
#include "ui/social/party_windows.h"
#include "ui/social/mini_party_area.h"
#include "ui/wndmgr/wndmgr.h"

struct MiniPartyWndAreaMgr *mini_party_area_mgr_construct(struct MiniPartyWndAreaMgr *self)
{
    self->taken.first = self->taken.last = self->taken.end = NULL;
    self->free.first = self->free.last = self->free.end = NULL;
    SetRect(&self->area, 0, 0, g_windowMgr.screen_width, g_windowMgr.screen_height);
    return self;
}

struct MiniPartySortArea *mini_party_sort_area_construct(struct MiniPartySortArea *self,
                                                                 const RECT *in)
{
    RECT bounds;
    int i;

    list_init(&self->cells);
    SetRect(&self->area, 0, 0, g_windowMgr.screen_width, g_windowMgr.screen_height);
    self->cols = (self->area.right - self->area.left) / g_miniPartyCellWidth;
    self->rows = (self->area.bottom - self->area.top) / g_miniPartyCellHeight;
    bounds = *in;
    for (i = 0; i < self->cols * self->rows; i++) {
        int x = self->area.left + i / self->rows * g_miniPartyCellWidth;
        int y = self->area.top + i % self->rows * g_miniPartyCellHeight;

        if (bounds.left <= x && x + g_miniPartyCellWidth <= bounds.right && bounds.top <= y &&
            y + g_miniPartyCellHeight <= bounds.bottom)
            list_push_back(&self->cells, &i, sizeof i);
    }
    return self;
}
