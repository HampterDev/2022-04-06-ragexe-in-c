#ifndef UI_SHOPS_ROULETTE_H
#define UI_SHOPS_ROULETTE_H

struct UIWindow;

void ui_roulette_allow_spin(struct UIWindow *wnd, int allow);

struct UIOpenRoulletteWnd *ui_open_roullette_wnd_construct(struct UIOpenRoulletteWnd *self);
struct UIWindow *ui_open_roullette_wnd_new(void);
struct UIRoulletteWnd *ui_roullette_wnd_construct(struct UIRoulletteWnd *self);
struct UIWindow *ui_roullette_wnd_new(void);

struct Roulette;
extern struct Roulette *g_roulette;

#endif
