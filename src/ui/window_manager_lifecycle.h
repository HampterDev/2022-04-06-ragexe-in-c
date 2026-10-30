#ifndef UI_WINDOW_MANAGER_LIFECYCLE_H
#define UI_WINDOW_MANAGER_LIFECYCLE_H

struct UIWindowMgr;

struct UIWindowMgr *window_mgr_construct(struct UIWindowMgr *self);
void ui_window_mgr_destroy(struct UIWindowMgr *self);
void ui_window_mgr_set_screen_size(struct UIWindowMgr *self, int width, int height);
void ui_window_mgr_init_ui_rect_info_from_reg(struct UIWindowMgr *self);
void ui_window_mgr_write_ui_rect_info_to_reg(struct UIWindowMgr *self);
void ui_window_mgr_close_all_windows(struct UIWindowMgr *self);
void window_mgr_clear_backdrop(struct UIWindowMgr *self);

#endif
