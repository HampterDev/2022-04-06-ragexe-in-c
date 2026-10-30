#ifndef UI_SHOPS_ITEM_STORE_COMBO_H
#define UI_SHOPS_ITEM_STORE_COMBO_H

#include "ui/window.h"
#include "ui/controls/combo_box.h"

struct UIItemStoreComboBoxWnd {
    struct UIComboBoxDefWnd def;
    BYTE pink;
};

struct UIItemStoreComboBoxWnd *ui_item_store_combo_box_wnd_construct(struct UIItemStoreComboBoxWnd *self);

#endif
