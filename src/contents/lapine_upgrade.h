#ifndef CONTENTS_LAPINE_UPGRADE_H
#define CONTENTS_LAPINE_UPGRADE_H

#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "contents/lapine_box.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"

#define LAPINE_UPGRADE_WND_ID           0x12e
#define LAPINE_UPGRADE_MAKE_BUTTON_ID   0x1f5

#define LAPINE_UPGRADE_MSG_SET_BOX      0x17
#define LAPINE_UPGRADE_MSG_SET_TARGET   0x1f

#define PACKET_LAPINE_UPGRADE           0xab6
#define PACKET_LAPINE_UPGRADE_CLOSE     0xab5

struct LapineUpgradeList {
    struct UIScrollWnd scroll;
    struct Vec resources;
    struct Vec rows;
    int min_option_num;
    int min_refine;
    int max_refine;
};

struct LapineUpgradeBox {
    struct UIFrameWnd frame;
    struct UIWindow *cancel_button;
    struct UIWindow *make_button;
    struct UIWindow *move_button;
    struct LapineUpgradeList *list;
    int box_item;
    struct String title;
    struct ItemInfo target;
    int min_option_num;
    struct String need_source;
    struct String notice;
    unsigned notice_time;
    unsigned notice_duration;
    char waiting;
};

struct LapineIngredient *lapine_upgrade_row_construct(struct LapineIngredient *self);
void lapine_upgrade_row_destruct(struct LapineIngredient *self);
struct LapineUpgradeList *lapine_upgrade_list_construct(struct LapineUpgradeList *self,
                                                                   int w, int h);
void lapine_upgrade_list_destruct(struct LapineUpgradeList *self);
void lapine_upgrade_destruct(struct LapineUpgradeBox *self);
struct LapineIngredient *lapine_upgrade_list_selected(struct LapineUpgradeList *self);
char lapine_upgrade_list_add(struct LapineUpgradeList *self, int index);
char lapine_upgrade_list_take(struct LapineUpgradeList *self, int index);
void lapine_upgrade_add_resource(struct LapineUpgradeBox *self, int item_id);
void lapine_upgrade_set_waiting(struct LapineUpgradeBox *self, char on);

struct LapineUpgradeBox *lapine_upgrade_construct(struct LapineUpgradeBox *self);
struct UIWindow *lapine_upgrade_new(void);
void lapine_upgrade_delete(struct UIWindow *window);
void lapine_upgrade_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop);
void lapine_upgrade_on_create(struct UIWindow *self, int w, int h);
void lapine_upgrade_on_frame(struct UIWindow *self);
void lapine_upgrade_on_draw(struct UIWindow *self);
void lapine_upgrade_press(struct UIWindow *self, int x, int y);
void lapine_upgrade_double_click(struct UIWindow *self, int x, int y);
void lapine_upgrade_hover(struct UIWindow *self, int x, int y);
void lapine_upgrade_show_info(struct UIWindow *self, int x, int y);
int lapine_upgrade_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                       int a, int b, int c, int d);

void lapine_upgrade_list_delete(struct UIWindow *window);
void lapine_upgrade_list_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop);
int lapine_upgrade_list_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                            int a, int b, int c, int d);

void lapine_upgrade_row_delete(struct UIWindow *window);
void lapine_upgrade_row_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop);
void lapine_upgrade_row_on_draw(struct UIWindow *self);
void lapine_upgrade_row_press(struct UIWindow *self, int x, int y);
void lapine_upgrade_row_double_click(struct UIWindow *self, int x, int y);
void lapine_upgrade_row_release(struct UIWindow *self, int x, int y);
void lapine_upgrade_row_show_info(struct UIWindow *self, int x, int y);

#endif
