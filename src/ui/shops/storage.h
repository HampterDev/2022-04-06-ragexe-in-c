#ifndef UI_SHOPS_STORAGE_H
#define UI_SHOPS_STORAGE_H

struct UIWindow;

void ui_storage_set_title(struct UIWindow *wnd, const char *name);
void ui_storage_set_list_received(struct UIWindow *wnd);

struct UIItemStoreWnd *ui_item_store_wnd_construct(struct UIItemStoreWnd *self);
struct UIWindow *ui_item_store_wnd_new(void);
void ui_item_store_wnd_set_page_check(struct UIItemStoreWnd *self, int page, BYTE checked);

#endif
