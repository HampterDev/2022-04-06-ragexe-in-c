#include "game/mode_misc.h"
#include "session/item_predicates.h"
#include "platform/code_page.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_ITEMS_H
#define UI_ITEMS_H

#include <stdio.h>
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "iteminfo/item_tables.h"
#include "ui/character/character_draw.h"
#include <mmsystem.h>

static __inline struct UIBitmapButton *ui_add_skin_button(struct UIWindow *parent, const char *name, int x, int y,
                                                          int id)
{
    static const char *const suffix[3] = { "", "_a", "_b" };
    struct UIBitmapButton *btn = mem_alloc(sizeof(struct UIBitmapButton));
    char path[MAX_PATH];
    int state;

    if (btn)
        btn = ui_bitmap_button_construct(btn);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++) {
        _snprintf(path, sizeof path, UI_SKIN "%s%s.bmp", name, suffix[state]);
        ui_bitmap_button_set_bitmap(btn, path, state, 0);
    }
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_window_move_base(&btn->button.base, x, y);
    btn->button.base.id = id;
    ui_window_add_child(parent, &btn->button.base);
    return btn;
}

#define UI_WND_CARD_ILLUST 0x2f
#define UI_MSG_SHOW_CARD 0x3e

struct UITextButton;
#define ui_button_set_tooltip(btn, text) ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)(btn), (text))

void ui_item_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);

void ui_item_wnd_store_info(struct UIWindow *window);
void ui_item_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y);

struct UICollectionSystemWnd *ui_collection_system_wnd_construct(struct UICollectionSystemWnd *self);
struct UIWindow *ui_collection_system_wnd_new(void);

struct UIItemDropCntWnd {
    struct UIFrameWnd frame;
    struct UIWindow *edit;
    struct UIStaticText *name;
};

struct UIItemDropCntWnd *ui_item_drop_cnt_wnd_construct(struct UIItemDropCntWnd *self);
struct UIWindow *ui_item_drop_cnt_wnd_new(void);
void ui_item_drop_cnt_wnd_destruct(struct UIItemDropCntWnd *self);

struct UICardItemIllustWnd *ui_card_item_illust_wnd_construct(struct UICardItemIllustWnd *self);
struct UIWindow *ui_card_item_illust_wnd_new(void);
struct UIIllustWnd *ui_illust_wnd_construct(struct UIIllustWnd *self);
struct UIWindow *ui_illust_wnd_new(void);

struct UIItemBaseWnd *ui_item_base_wnd_construct(struct UIItemBaseWnd *self);
void ui_item_base_wnd_destruct(struct UIItemBaseWnd *self);

struct UIMixAcceptWnd *ui_mix_accept_wnd_construct(struct UIMixAcceptWnd *self);
struct UIWindow *ui_mix_accept_wnd_new(void);
struct UIYourItemWnd *ui_your_item_wnd_construct(struct UIYourItemWnd *self);
struct UIWindow *ui_your_item_wnd_new(void);

struct UINotifyItemObtainWnd *ui_notify_item_obtain_wnd_construct(struct UINotifyItemObtainWnd *self);
struct UIWindow *ui_notify_item_obtain_wnd_new(void);

struct UIItemWnd;
void ui_item_wnd_clear_new_marks(struct UIItemWnd *self, BYTE keep);
struct UIItemWnd *ui_item_wnd_construct(struct UIItemWnd *self);
struct UIWindow *ui_item_wnd_new(void);
void ui_item_wnd_destruct(struct UIItemWnd *self);
void ui_slot_no_op(struct UIWindow *self);

struct MergeItemWnd *merge_item_wnd_construct(struct MergeItemWnd *self);
struct UIWindow *merge_item_wnd_new(void);

struct UIRoundBoxTooltipWnd *ui_round_box_tooltip_wnd_construct(struct UIRoundBoxTooltipWnd *self);
struct UIWindow *ui_round_box_tooltip_wnd_new(void);

struct UIPetInfoWnd *ui_pet_info_wnd_construct(struct UIPetInfoWnd *self);
struct UIWindow *ui_pet_info_wnd_new(void);

#pragma pack(push, 1)
struct RepairItem {
    short index;
    int item_id;
    int cards[ITEM_CARD_SLOTS];
    BYTE refine;
    BYTE grade;
};
#pragma pack(pop)

struct UIItemRepairWnd *ui_item_repair_wnd_construct(struct UIItemRepairWnd *self);
struct UIWindow *ui_item_repair_wnd_new(void);
struct UIWeaponRefineWnd *ui_weapon_refine_wnd_construct(struct UIWeaponRefineWnd *self);
struct UIWindow *ui_weapon_refine_wnd_new(void);

struct UISwapBitmapButton;

struct UISwapEquipmentWnd {
    struct UIFrameWnd frame;
    struct UISwapBitmapButton *swap;
    int kind;
    DWORD made_at;
    int accessory_bottom;
    int accessory_top;
    int accessory_mid;
    int robe;
    int hover[3];
    struct UIAnimation animation;
};

struct UISwapEquipmentWnd *ui_swap_equipment_wnd_construct(struct UISwapEquipmentWnd *self);
unsigned long long ui_swap_equipment_wnd_key(struct UISwapEquipmentWnd *self, unsigned long long key);
int ui_swap_equipment_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                          int val2, int val3, int val4);

#endif
