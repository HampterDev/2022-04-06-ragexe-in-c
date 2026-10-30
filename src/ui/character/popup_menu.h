#ifndef UI_CHARACTER_POPUP_MENU_H
#define UI_CHARACTER_POPUP_MENU_H

struct UIWindow;

#define MENU_OWNER_MODE 0x166
#define MENU_OWNER_BY_KEY 0x12d

int ui_menu_owner_id(const struct UIWindow *menu);
int ui_menu_owner_key(const struct UIWindow *menu);
int ui_menu_take_hold(struct UIWindow *menu);
void ui_menu_set_hold(struct UIWindow *menu);

struct UIMenuWnd *ui_menu_wnd_construct(struct UIMenuWnd *self);
struct UIWindow *ui_menu_wnd_new(void);

#endif
