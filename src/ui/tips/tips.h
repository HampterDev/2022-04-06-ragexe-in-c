#include "render/texture.h"
#include "render/texture_cache.h"
#include "resources/animation.h"
#include "platform/client_options.h"
#ifndef UI_TIPS_H
#define UI_TIPS_H

#include "platform/client_config.h"

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/ui_std.h"

#include "ui/tips/tipbox_mgr.h"

void tips_save_show_at_startup(const int *value);

struct TipboxIcon {
    DWORD start;
    DWORD blink_time;
    int number;
    int blinks;
    float x, y;
    int from_x, from_y;
    int to_x, to_y;
    float duration;
    BYTE visible;
};
struct TipboxDot {
    float x, y;
    BYTE visible;
};

struct UITipboxWnd {
    struct UIFrameWnd frame;
    struct UIWindow *search_edit;
    struct UIWindow *results;
    struct UIWindow *text;
    struct UIWindow *titles[2];
    struct UIWindow *page_no;
    struct UIWindow *page_count;
    struct UIBitmapButton *prev;
    struct UIBitmapButton *next;
    struct Vec icons;
    struct Vec dots;
    DWORD anim_time;
    DWORD now;
    BYTE page_dirty;
    int show_at_startup;
};
OFFSET_ASSERT(UITipboxWnd, search_edit, 0x94);
OFFSET_ASSERT(UITipboxWnd, icons, 0xb8);
OFFSET_ASSERT(UITipboxWnd, show_at_startup, 0xdc);
SIZE_ASSERT(UITipboxWnd, 0xe0);
#define TIPBOX_WND_ID 0x13b

void ui_tipbox_wnd_animate_icons(struct UITipboxWnd *self);

struct TexCache;

struct UITipOfTheDayWnd *ui_tip_of_the_day_wnd_construct(struct UITipOfTheDayWnd *self);
struct UIWindow *ui_tip_of_the_day_wnd_new(void);

struct UITipboxWnd *ui_tipbox_wnd_construct(struct UITipboxWnd *self);
struct UIWindow *ui_tipbox_wnd_new(void);
void ui_tipbox_wnd_show_tip(struct UITipboxWnd *self, int id);

#endif
