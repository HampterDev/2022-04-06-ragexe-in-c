#include "core/fatal.h"
#include "ui/wndmgr/window_table.h"

/* WM_NO_SLOT and an open row without info are offset 0, which must not be a window. */
typedef char wm_offset_zero_is_windows_by_id[offsetof(struct UIWindowMgr, windows_by_id) == 0 ? 1 : -1];

#define WM_MAX_ROWS 0x300

static const struct WmWindowRow *rows_by_id[WM_WINDOW_IDS];
static const struct WmWindowRow *all_rows[WM_MAX_ROWS];
static size_t all_row_count;

void ui_window_mgr_set_window_table(const struct WmWindowTable *table)
{
    size_t i, j;

    for (i = 0; i < WM_WINDOW_IDS; i++)
        rows_by_id[i] = NULL;
    all_row_count = 0;
    for (i = 0; i < table->count; i++) {
        for (j = 0; j < table->parts[i]->count; j++) {
            const struct WmWindowRow *row = &table->parts[i]->rows[j];

            if (all_row_count == WM_MAX_ROWS)
                fatal_error("window table: more than WM_MAX_ROWS rows");
            all_rows[all_row_count++] = row;
            if (row->slot == WM_NO_SLOT && (row->flags & (WM_GET_SLOT | WM_SLOT_FLAGS)))
                fatal_error("window table: a row with slot flags but no slot");
            if (row->id == WM_NO_ID)
                continue;
            if ((unsigned)row->id >= WM_WINDOW_IDS || rows_by_id[row->id])
                fatal_error("window table: a window ID out of range or with two rows");
            rows_by_id[row->id] = row;
        }
    }
    for (i = 0; i < all_row_count; i++)
        for (j = i + 1; j < all_row_count; j++)
            if (all_rows[i]->slot != WM_NO_SLOT && all_rows[i]->slot == all_rows[j]->slot &&
                (all_rows[i]->flags & WM_SLOT_FLAGS) != (all_rows[j]->flags & WM_SLOT_FLAGS))
                fatal_error("window table: two rows give one slot different slot flags");
}

const struct WmWindowRow *wm_window_row(int window_id)
{
    return (unsigned)window_id < WM_WINDOW_IDS ? rows_by_id[window_id] : NULL;
}

size_t wm_window_row_count(void)
{
    return all_row_count;
}

const struct WmWindowRow *wm_window_row_at(size_t index)
{
    return all_rows[index];
}

struct UIWindow **wm_row_slot(struct UIWindowMgr *self, const struct WmWindowRow *row)
{
    return (struct UIWindow **)((unsigned char *)self + row->slot);
}
