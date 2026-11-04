#ifndef UI_ITEM_DRAW_H
#define UI_ITEM_DRAW_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "platform/win_types.h"
#include "ui/items/item_name_runs.h"

struct UIWindow;
struct BitmapRes;
struct ItemInfo;

void ui_window_draw_item_win_mid(struct UIWindow *self, int x, int y);

void ui_window_draw_item_icon(struct UIWindow *self, int x, int y,
                                      const struct ItemInfo *info, BYTE force_identified);

void ui_window_draw_item_icon_count(struct UIWindow *self, int x, int y,
                                           const struct ItemInfo *info, BYTE force_identified, BYTE plain_count);

void ui_window_draw_item_icon_count_grey(struct UIWindow *self, int grey_arg, int x, int y,
                                               const struct ItemInfo *info, BYTE identified, BYTE plain_count);

void ui_window_draw_item_icon_by_name(struct UIWindow *self, int x, int y,
                                            const char *id_text, BYTE identified);

int ui_window_draw_item_name(struct UIWindow *self, int x, int y,
                                     const struct ItemInfo *info, COLORREF colour,
                                     int arg7);

int ui_window_draw_item_name_short(struct UIWindow *self, int x, int y, int max_width,
                                          const struct ItemInfo *info, int line_height);

#endif
