#include "game/drag.h"
#include "skills/skill_bundle.h"
#include "session/skill_queries.h"
#include "skills/skill_info.h"
#include "ui/cooldown.h"
#include "resources/state_icon_sheet.h"
#include "replay/replay.h"
#include "session/state.h"
#ifndef UI_HOTKEY_BARS_H
#define UI_HOTKEY_BARS_H

#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "ui/hotkeys/hotkeys.h"
#include <mmsystem.h>

#include "session/hotkey_types.h"

#define HOTKEY_SLOTS_PER_ROW 9
#define HOTKEY_BAR_SLOTS 36
#define HOTKEY_QUICK_SLOTS 2

#include "session/option_pairs.h"
#define SESSION_OPT_BAR_GROUP 0xa
#define SESSION_OPT_NO_HOTKEYS 5
#define SESSION_OPT_ROLLING_CUTTER_ONE_SHOT 4

#define SKILL_SHARED_DELAY 0x270f
#define SKILL_POST_DELAY 0x241
#define SKILL_GC_ROLLINGCUTTER 0x7f4

#define GAME_MSG_DROP_DONE 0x12
#define GAME_MSG_EQUIP 0x13
#define GAME_MSG_TAKEOFF_EQUIP 0x14
#define GAME_MSG_USE_ITEM 0x1b
#define GAME_MSG_EQUIP_AMMO 0x57
#define GAME_MSG_USE_SKILL 0x71
#define WEAR_AMMO 0x8000
#define ITEM_ID_ASK_FIRST 0x3202
#define MSG_CONFIRM_OPEN_BOUND_BOX 0x945
#define MSG_ASK_USE_ITEM 0x4ca
#define UI_MSGBOX_ANSWER_NO 0xb4

#define DRAG_FROM_SHORTCUT 7
#define DRAG_FROM_QUICK_SLOT 0x10

#define WND_SKILL_DESCRIBE 0x2e
#define WND_ITEM_DESCRIBE 0xc
#define SKILL_DESCRIBE_MSG_SHOW 0x3d
#define ITEM_DESCRIBE_MSG_SHOW 0x18

void ui_window_text_out_outlined_right(struct UIWindow *self, int x, int y,
                                              const char *text, int len, COLORREF colour, COLORREF outline,
                                              int font_face, int font_height, int font_arg);

void hotkey_cooldowns_draw(struct UIWindow *wnd, struct CooldownSlotList *slots, int quick, int sheet_arg);
unsigned long long hotkey_slot_key(unsigned long long key, const struct HotkeySlot *slot);
struct SkillCooldownNode *hotkey_cooldown_of(int skill_id);
void hotkey_place_describe(struct UIWindow *wnd, struct UIWindow *describe, int x, int y);

#define WND_SHORTCUT 0x24
#define SHORTCUT_ROW_HEIGHT 0x21
#define SHORTCUT_SLOT_WIDTH 0x1d
#define SHORTCUT_MAX_ROWS 4

struct UIShortCutWnd {
    struct UIFrameWnd frame;
    struct UIWindow *group_check;
    struct WndInfoShow *info;
    struct UIBitmapButton *minus;
    struct UIBitmapButton *plus;
    struct HotkeySlot *slots[HOTKEY_BAR_SLOTS];
    int hover_column;
    int hover_row;
    int rows;
    unsigned long long drawn_key;
};
OFFSET_ASSERT(UIShortCutWnd, slots, 0xa4);
OFFSET_ASSERT(UIShortCutWnd, hover_column, 0x134);
SIZE_ASSERT(UIShortCutWnd, 0x148);

int quik_slot_wnd_info_is_valid(struct WindowInfo *self);
void quik_slot_wnd_info_make_default(struct WindowInfo *self);

struct UIQuikSlotWnd {
    struct UIFrameWnd frame;
    int hover;
    struct WindowInfo *info;
    struct HotkeySlot slots[HOTKEY_QUICK_SLOTS];
    unsigned long long drawn_key;
};
OFFSET_ASSERT(UIQuikSlotWnd, slots, 0x9c);
SIZE_ASSERT(UIQuikSlotWnd, 0xb8);

#define HOTKEY_MSG_RELAYOUT 0xe
#define HOTKEY_MSG_REFRESH 0x17
#define HOTKEY_MSG_USE 0x29
#define HOTKEY_MSG_SCROLL 0x8d
#define SHORTCUT_ID_PLUS 0xd0
#define SHORTCUT_ID_MINUS 0xd1
#define SHORTCUT_ID_GROUP 0x21d
#define MSG_SHORTCUT_GROUP_TIP 0xe0c
#define MSG_SKILL_SLOT 0xb8
#define MSG_ITEM_SLOT 0xb7

struct UIQuikSlotWnd *ui_quik_slot_wnd_construct(struct UIQuikSlotWnd *self);
struct UIWindow *ui_quik_slot_wnd_new(void);

struct UIShortCutWnd *ui_short_cut_wnd_construct(struct UIShortCutWnd *self);
struct UIWindow *ui_short_cut_wnd_new(void);

#endif
