#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "ui/social/party_windows.h"
#include "ui/wndmgr/wndmgr.h"

int g_miniPartyCellHeight = 55;

int g_miniPartyCellWidth = 177;

struct MiniPartyWndAreaMgr *g_miniPartyAreaMgr;

void mini_party_area_mgr_delete(struct MiniPartyWndAreaMgr *self)
{
    vec_free(&self->free);
    vec_free(&self->taken);
    mem_free(self);
}

static __inline void cell_pos(POINT *out, int cell, int rows, int left, int top)
{
    out->x = cell / rows * g_miniPartyCellWidth + left;
    out->y = cell % rows * g_miniPartyCellHeight + top;
}

int mini_party_area_mgr_find_free_cell(struct MiniPartyWndAreaMgr *self)
{
    RECT tmp, area;
    struct MiniPartySortArea sort;
    struct List cells;
    struct ListNode *node;
    int *it;
    int i, result;

    area = *ui_window_mgr_mini_party_area(&g_windowMgr, &tmp);

    list_init(&sort.cells);
    SetRect(&sort.area, 0, 0, g_windowMgr.screen_width, g_windowMgr.screen_height);
    sort.cols = (sort.area.right - sort.area.left) / g_miniPartyCellWidth;
    sort.rows = (sort.area.bottom - sort.area.top) / g_miniPartyCellHeight;
    for (i = 0; i < sort.cols * sort.rows; i++) {
        POINT p;

        cell_pos(&p, i, sort.rows, sort.area.left, sort.area.top);
        if (area.left <= p.x && p.x + g_miniPartyCellWidth <= area.right && area.top <= p.y &&
            p.y + g_miniPartyCellHeight <= area.bottom)
            list_push_back(&sort.cells, &i, sizeof i);
    }

    mini_party_sort_area_get_cells(&sort, &cells);
    result = -1;
    for (it = (int *)self->free.first; it != (int *)self->free.last; it++) {
        for (node = cells.head->next; node != cells.head; node = node->next)
            if (*LIST_VALUE(node, int) == *it)
                break;
        if (node == cells.head)
            continue;
        if (!ui_window_mgr_find_by_key(&g_windowMgr, *it)) {
            result = *it;
            break;
        }
    }
    list_free(&cells, NULL);
    list_free(&sort.cells, NULL);
    return result;
}

POINT *mini_party_area_mgr_cell_pos(struct MiniPartyWndAreaMgr *self, POINT *out, int cell)
{
    cell_pos(out, cell, self->rows, self->area.left, self->area.top);
    return out;
}

POINT *mini_party_sort_area_cell_pos(struct MiniPartySortArea *self, POINT *out, int cell)
{
    cell_pos(out, cell, self->rows, self->area.left, self->area.top);
    return out;
}

static __inline int nearest_cell(int offset, int size, int count)
{
    int index = offset / size, last = count - 1, t;

    if ((double)(offset % size) > (double)size * 0.5)
        index++;
    t = index < last ? index : last;
    return t < 0 ? 0 : t;
}

int mini_party_area_mgr_cell_at(struct MiniPartyWndAreaMgr *self, int x, int y)
{
    int col = nearest_cell(x - self->area.left, g_miniPartyCellWidth, self->cols);
    int row = nearest_cell(y - self->area.top, g_miniPartyCellHeight, self->rows);
    int cell = self->rows * col + row;
    int *it;

    for (it = (int *)self->free.first; it != (int *)self->free.last; it++)
        if (*it == cell)
            break;
    return it != (int *)self->free.last ? cell : -1;
}

void mini_party_area_mgr_rebuild(struct MiniPartyWndAreaMgr *self)
{
    int i;

    self->cols = (self->area.right - self->area.left) / g_miniPartyCellWidth;
    self->rows = (self->area.bottom - self->area.top) / g_miniPartyCellHeight;
    for (i = 0; i < self->cols * self->rows; i++) {
        RECT *r;
        POINT p;

        cell_pos(&p, i, self->rows, self->area.left, self->area.top);
        for (r = (RECT *)self->taken.first; r != (RECT *)self->taken.last; r++)
            if (p.x < r->right && p.x + g_miniPartyCellWidth > r->left && p.y < r->bottom &&
                p.y + g_miniPartyCellHeight > r->top)
                break;
        if (r != (RECT *)self->taken.last)
            continue;
        vec_push(&self->free, &i, sizeof i);
    }
}

struct MiniPartyWndAreaMgr *mini_party_area_mgr_instance(void)
{
    if (!g_miniPartyAreaMgr) {
        struct MiniPartyWndAreaMgr *p = mem_try_alloc(sizeof(*p));

        g_miniPartyAreaMgr = p ? mini_party_area_mgr_construct(p) : NULL;
    }
    return g_miniPartyAreaMgr;
}
