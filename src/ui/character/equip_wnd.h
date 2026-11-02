#ifndef UI_CHARACTER_EQUIP_WND_H
#define UI_CHARACTER_EQUIP_WND_H

struct UIWindow;

void ui_equip_wnd_set_viewing_other(struct UIWindow *wnd, int other);

struct UIEquipWnd *ui_equip_wnd_construct(struct UIEquipWnd *self);
struct UIWindow *ui_equip_wnd_new(void);

#endif
