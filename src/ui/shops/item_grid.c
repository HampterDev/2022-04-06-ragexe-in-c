#include "ui/shops/shops.h"

int ui_item_base_wnd_index_at(struct UIItemBaseWnd *self, int x, int y)
{
    struct ListNode *node;
    int column, row, cell, i;

    if (x < ITEM_GRID_X || y < ITEM_GRID_Y || y >= self->rows * ITEM_GRID_CELL + ITEM_GRID_Y)
        return -1;
    column = (x - ITEM_GRID_X) / ITEM_GRID_CELL;
    row = (y - ITEM_GRID_Y) / ITEM_GRID_CELL;
    if (column > self->columns - 1)
        column = self->columns - 1;
    cell = 0;
    for (node = self->items.head->next, i = 0; node != self->items.head; node = node->next, i++) {
        if (i < self->scroll * self->columns || item_grid_hidden(LIST_VALUE(node, struct ItemInfo)))
            continue;
        if (cell == self->columns * row + column)
            return i;
        cell++;
    }
    return -1;
}

struct ItemInfo *ui_item_base_wnd_get_item(struct UIItemBaseWnd *self, struct ItemInfo *out, int index)
{
    struct ListNode *node;
    int i = 0;

    for (node = self->items.head->next; node != self->items.head; node = node->next, i++)
        if (i == index)
            return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
    return item_info_construct_with_arg(out);
}

struct ItemInfo *ui_item_base_wnd_item_at(struct UIItemBaseWnd *self, struct ItemInfo *out, int x, int y)
{
    struct ListNode *node;
    int column, row, cell, i;

    if (x < ITEM_GRID_X || x >= self->frame.base.width - 0x14 || y < ITEM_GRID_Y ||
        y >= self->rows * ITEM_GRID_CELL + ITEM_GRID_Y)
        return item_info_construct_with_arg(out);
    column = (x - ITEM_GRID_X) / ITEM_GRID_CELL;
    row = (y - ITEM_GRID_Y) / ITEM_GRID_CELL;
    if (column > self->columns - 1)
        return item_info_construct_with_arg(out);
    cell = 0;
    for (node = self->items.head->next, i = 0; node != self->items.head; node = node->next, i++) {
        if (i < self->columns * self->scroll || item_grid_hidden(LIST_VALUE(node, struct ItemInfo)))
            continue;
        if (cell == self->columns * row + column)
            return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
        cell++;
    }
    return item_info_construct_with_arg(out);
}
