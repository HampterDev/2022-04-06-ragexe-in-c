#include "game/drag.h"
#include "ui/controls/buttons.h"
#include "iteminfo/item_api.h"
#ifndef UI_ITEMS_REFINING_INTERNAL_H
#define UI_ITEMS_REFINING_INTERNAL_H

#include "ui/items/items.h"
#include "ui/ui_std.h"

#define WID_REFINE 0x127
#define WID_ITEM_DESCRIBE 0xc
#define MSG_ITEM_DROPPED 0x17
#define MSG_DESCRIBE_ITEM 0x18
#define BLACKSMITH_BLESSING 0x19eb

#define ID_REFINE 0xb0
#define ID_CLOSE 0xb1
#define ID_RETURN 0x1ff
#define ID_AGAIN 0x200

#define PACKET_REFINING_SELECT_ITEM 0xaa1
#define PACKET_REQ_REFINING 0xaa3
#define PACKET_REFINING_CLOSE 0xaa4

#define MSG_CLOSE 0xda
#define MSG_TITLE 0xb96
#define MSG_BLESSING_ON 0xb97
#define MSG_NO_ZENY 0xb98
#define MSG_NO_BLESSINGS 0xb99
#define MSG_CANNOT_REFINE 0xb9a
#define MSG_SUCCESS 0xb9b
#define MSG_FAILURE 0xb9c
#define MSG_CHANCE 0xb9e
#define MSG_HINT_ORE 0xbac
#define MSG_HINT_ENRICHED 0xbad
#define MSG_HINT_HD 0xbae
#define MSG_MATERIALS_CHANGED 0xcaa
#define MSG_MATERIAL_GONE 0xcab
#define MSG_AGAIN_NO_ZENY 0xcac
#define MSG_AGAIN_NO_BLESSINGS 0xcad
#define MSG_DESTROYED 0xcae
#define MSG_NAME_OPTIONS 0xa28
#define MSG_CHAT_SUCCESS 0x1f2
#define MSG_CHAT_FAILURE 0x1f3
#define MSG_CHAT_DOWNGRADE 0x601

#define COLOUR_NAME 0x462117
#define COLOUR_CHANCE 0x422b1f
#define COLOUR_PERCENT 0xde612a
#define COLOUR_FRAME 0xc2c2c2
#define COLOUR_CHAT 0xffff00
#define NOTICE_TIME 0x1388
#define NOTICE_TIME_SHORT 0xbb8

#define REFINE_DIR UI_SKIN "refining_renewal\\"

enum {
    REFINE_WAIT,
    REFINE_READY,
    REFINE_PROCESS,
    REFINE_SUCCESS,
    REFINE_FAILURE,
    REFINE_DOWNGRADE,
    REFINE_FAILURE_KEPT,
};
#define REFINE_NO_RESULT 7

#pragma pack(push, 1)
struct RefineMaterial {
    int item_id;
    BYTE chance;
    int zeny;
};
struct PacketRefiningSelectItem {
    WORD id;
    WORD index;
};
struct PacketReqRefining {
    WORD id;
    WORD index;
    int material;
    BYTE blessing;
};
#pragma pack(pop)
SIZE_ASSERT(RefineMaterial, 9);
SIZE_ASSERT(PacketReqRefining, 9);

struct UIRefining_BtnRefining {
    struct UIBitmapButton button;
    int price;
};
struct UIRefining_BtnAgain {
    struct UIBitmapButton button;
    int price;
    BYTE highlight;
    BYTE count;
};
struct UIRefining_BtnReturn {
    struct UIBitmapButton button;
    BYTE highlight;
};
struct UIRefining_Notice {
    struct UIStaticText base;
    struct String next_text;
    BYTE next_red;
    DWORD due;
    BYTE hide_when_due;
    BYTE red;
};

struct UIRefiningWnd {
    struct UIFrameWnd frame;
    struct UIRefining_BtnRefining *btn_refine;
    struct UIRefining_BtnReturn *btn_return;
    struct UIRefining_BtnAgain *btn_again;
    struct UIRefining_Notice *notice;
    int state;
    int result;
    int frame_time;
    DWORD frame_start;
    int anim_frame;
    int selected;
    struct Vec materials;
    BYTE blessing;
    int blessing_item;
    int blessing_count;
    struct ItemInfo item;
    BYTE busy;
    struct Vec next_materials;
    int next_blessing_count;
    struct ItemInfo next_item;
};
OFFSET_ASSERT(UIRefiningWnd, state, 0xa4);
OFFSET_ASSERT(UIRefiningWnd, materials, 0xbc);
OFFSET_ASSERT(UIRefiningWnd, item, 0xd4);

void ui_refining_wnd_reset(struct UIRefiningWnd *self);
void ui_refining_wnd_set_state(struct UIRefiningWnd *self, int state, int result);
int ui_refining_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4);
void ui_refining_wnd_refresh(struct UIRefiningWnd *self);
void ui_refining_btn_again_set_bitmaps(struct UIRefining_BtnAgain *self);
void ui_refining_btn_return_set_bitmaps(struct UIRefining_BtnReturn *self);
void ui_refining_notice_set_text(struct UIRefining_Notice *self, const char *text, int delay, BYTE red);

#define WND(self) (&(self)->frame.base)
#define MATERIAL(v, i) ((struct RefineMaterial *)(v)->first + (i))
#define MATERIAL_COUNT(v) ((unsigned)((struct RefineMaterial *)(v)->last - (struct RefineMaterial *)(v)->first))

int refining_selected_zeny(struct UIRefiningWnd *self);
BYTE refining_selected_chance(struct UIRefiningWnd *self);
int refining_carried(int item_id);
void ui_refining_btn_refining_set_bitmaps(struct UIRefining_BtnRefining *self);

void ui_refining_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);

void ui_refining_wnd_on_draw(struct UIWindow *window);
void ui_refining_wnd_on_l_btn_down(struct UIWindow *window, int x, int y);
void ui_refining_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y);
void ui_refining_wnd_on_mouse_hover(struct UIWindow *window, int x, int y);
void ui_refining_wnd_on_l_btn_up(struct UIWindow *window, int x, int y);
void ui_refining_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_refining_wnd_on_r_btn_up(struct UIWindow *window, int x, int y);

struct UIRefiningWnd *ui_refining_wnd_construct(struct UIRefiningWnd *self);
struct UIWindow *ui_refining_wnd_new(void);
void ui_refining_wnd_set_item(struct UIRefiningWnd *self, int index, int count, const void *materials,
                           BYTE blessing_count);

#endif
