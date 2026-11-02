#ifndef CONTENTS_LAPINE_BOX_H
#define CONTENTS_LAPINE_BOX_H

#include "base.h"
#include "std/std.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"

#define LAPINE_BMP(file) "유저인터페이스\\lapine\\" file

#define LAPINE_BOX_WND_ID       0x122
#define LAPINE_CANCEL_BUTTON_ID 0xc1
#define LAPINE_MAKE_BUTTON_ID   0x1ef
#define LAPINE_MOVE_BUTTON_ID   0x1f1
#define LAPINE_MAX_CHOSEN       10

#define LAPINE_MSG_FILL         0x17
#define LAPINE_MSG_LAYOUT       0x57
#define LAPINE_MSG_REMOVE       0x68
#define LAPINE_MSG_NOTICE       0x6f

#define LAPINE_DRAG_CHOSEN      0x1a
#define LAPINE_DRAG_CANDIDATE   0x1b
#define MODE_MSG_DROPPED        0x12

#define PACKET_LAPINE_MAKE      0xa4f
#define PACKET_LAPINE_CLOSE     0xa70

struct LapineScrollBar {
    struct UIScrollBar bar;
};

struct LapineIngredient {
    struct UIFrameWnd frame;
    struct ItemInfo item;
    char selected;
    char unusable;
    struct String name;
    int count;
};

struct LapineIngredientList {
    struct UIScrollWnd scroll;
    struct Vec resources;
    struct Vec rows;
    int min_refine;
    int max_refine;
};

struct LapineBox {
    struct UIFrameWnd frame;
    struct UIWindow *cancel_button;
    struct UIWindow *make_button;
    struct UIWindow *move_button;
    struct LapineIngredientList *list;
    int box_item;
    struct String title;
    struct Vec chosen;
    int need_count;
    struct String need_source;
    struct String notice;
    unsigned notice_time;
    unsigned notice_duration;
    char waiting;
};

void lapine_add_buttons(struct UIWindow *wnd, struct UIWindow **cancel, struct UIWindow **make,
                        struct UIWindow **move, unsigned make_id);

struct LapineBox *lapine_box_construct(struct LapineBox *self);
struct UIWindow *lapine_box_new(void);
void lapine_box_set_item(struct LapineBox *self, int box_item);
void lapine_box_set_waiting(struct LapineBox *self, char on);
char lapine_box_add(struct LapineBox *self, int index, int count);
void lapine_box_add_ingredient(struct LapineBox *self, int item_id, int count);
void lapine_box_destruct(struct LapineBox *self);
void lapine_box_delete(struct UIWindow *window);
void lapine_box_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop);
void lapine_box_on_create(struct UIWindow *self, int w, int h);
void lapine_box_on_frame(struct UIWindow *self);
void lapine_box_on_draw(struct UIWindow *self);
void lapine_box_press(struct UIWindow *self, int x, int y);
void lapine_box_double_click(struct UIWindow *self, int x, int y);
void lapine_box_hover(struct UIWindow *self, int x, int y);
void lapine_box_show_info(struct UIWindow *self, int x, int y);
intptr_t lapine_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                   intptr_t a, intptr_t b, intptr_t c, intptr_t d);

struct LapineIngredientList *lapine_list_new(void);
struct LapineIngredientList *lapine_list_construct(struct LapineIngredientList *self, int w, int h);
void lapine_list_destruct(struct LapineIngredientList *self);
struct LapineIngredient *lapine_row_construct(struct LapineIngredient *self);
void lapine_row_destruct(struct LapineIngredient *self);
char lapine_list_add(struct LapineIngredientList *self, int index);
void lapine_list_layout(struct UIScrollWnd *scroll, struct Vec *rows);
void lapine_list_delete(struct UIWindow *window);
void lapine_list_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop);
void lapine_list_on_create(struct UIWindow *self, int w, int h);
void lapine_list_on_draw(struct UIWindow *self);
void lapine_list_wheel(struct UIWindow *window, int delta);
intptr_t lapine_list_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    intptr_t a, intptr_t b, intptr_t c, intptr_t d);

void lapine_row_delete(struct UIWindow *window);
void lapine_row_move(struct UIWindow *self, int x, int y);
void lapine_row_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop);
void lapine_row_on_draw(struct UIWindow *self);
void lapine_row_press(struct UIWindow *self, int x, int y);
void lapine_row_double_click(struct UIWindow *self, int x, int y);
void lapine_row_hover(struct UIWindow *self, int x, int y);
void lapine_row_release(struct UIWindow *self, int x, int y);
void lapine_row_show_info(struct UIWindow *self, int x, int y);
void lapine_row_wheel(struct UIWindow *self, int delta);

struct LapineScrollBar *lapine_scroll_bar_construct(struct LapineScrollBar *self);
void lapine_scroll_bar_delete(struct UIWindow *window);
void lapine_scroll_bar_on_draw(struct UIWindow *self);

#endif
