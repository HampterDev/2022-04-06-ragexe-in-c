#ifndef CONTENTS_REPUTE_WND_H
#define CONTENTS_REPUTE_WND_H

#include "base.h"
#include "std/std.h"
#include "contents/ui_window.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/repute_mgr.h"

#define REPUTE_WND_ID           0x15a
#define REPUTE_SEARCH_ID        0x139
#define REPUTE_PREV_ID          0x134
#define REPUTE_NEXT_ID          0x135
#define REPUTE_CLOSE_ID         0xc1
#define REPUTE_ROWS_PER_PAGE    8
#define REPUTE_POINTS_PER_LIGHT 1000

struct UIReputeWnd {
    struct UIFrameWnd frame;
    struct UIWindow *group_combo;
    struct UIWindow *search_edit;
    struct UIWindow *prev_button;
    struct UIWindow *next_button;
    unsigned rows_per_page;
    unsigned page;
    unsigned page_count;
    unsigned char show_points;
    long long group_points;
    struct Vec entries;
};

struct UIReputeWnd *repute_wnd_construct(struct UIReputeWnd *self);
struct UIWindow *repute_wnd_new(void);
void repute_wnd_destruct(struct UIReputeWnd *self);
void repute_wnd_delete(struct UIWindow *window);
void repute_wnd_on_create(struct UIWindow *self, int width, int height);
void repute_wnd_on_draw(struct UIWindow *self);
struct Vec *repute_wnd_entries_move(struct Vec *self,
                                                     struct Vec *from);

int repute_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                   int a, int b, int c, int d);
void repute_wnd_show_group(struct UIReputeWnd *self, const struct String *group);
void repute_wnd_set_page(struct UIReputeWnd *self, unsigned page);
void repute_wnd_unhighlight_page(struct UIReputeWnd *self);

#endif
