#ifndef CONTENTS_ENCHANT_WND_H
#define CONTENTS_ENCHANT_WND_H

#include "contents/enchant_window_api.h"
#include "contents/enchant_mgr.h"
#include "contents/enchant_ui.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "std/std.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"

enum { PAGE_ENCHANT, PAGE_PERFECT, PAGE_UPGRADE, PAGE_RESET, PAGES, NO_PAGE = PAGES };

struct EnchantPage {
    int page;
    struct UIEnchantInfo *info;
};

struct UIEnchantWnd {
    struct UIFrameWnd frame;
    struct UIEnchantList *targets;
    struct UIEnchantGroup *tabs;
    struct Tree pages;
    struct UITagTextViewer *help;
    const struct EnchantList *table;
    unsigned __int64 table_id;
};

struct UIEnchantInfo **enchant_wnd_page_ref(struct UIEnchantWnd *self, int page);
#define PAGE(self, n) (*enchant_wnd_page_ref((self), (n)))

int enchant_wnd_shown_page(struct UIEnchantWnd *self);
void enchant_wnd_lock(struct UIEnchantWnd *self);
void enchant_wnd_select_target(struct UIEnchantWnd *self, const struct ItemInfo *item);
char enchant_wnd_pick_perfect(struct UIEnchantWnd *self, const struct ItemInfo *item);
char enchant_wnd_pick_upgrade(struct UIEnchantWnd *self, const struct ItemInfo *item);
char enchant_wnd_pick_socket(struct UIEnchantWnd *self, unsigned slot_no, int item_id);

struct UIEnchantWnd *enchant_wnd_construct(struct UIEnchantWnd *self);
struct UIWindow *enchant_wnd_new(void);

void enchant_wnd_delete(struct UIWindow *window);

void enchant_wnd_create(struct UIWindow *window, int width, int height);
void enchant_wnd_draw(struct UIWindow *window);
int enchant_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                    int wparam, int lparam, int extra1, int extra2);

#endif
