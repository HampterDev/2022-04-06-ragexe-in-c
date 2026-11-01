#include "ui/items/item_tags.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_CHAT_WINDOW_H
#define UI_CHAT_WINDOW_H

#include "ui/chat/chat.h"
#include "ui/chat/chat_history.h"

#define OFF_SCREEN (-100)
#define BASIC_INTERFACE UI_SKIN "basic_interface\\"

struct UINewChatWnd {
    struct UIFrameWnd frame;
    struct UIWindow *focus_edit;
    struct UIResizer *resizer;
    struct UIChatEditCtrl *edit;
    struct UIWhisperEditCtrl *whisper;
    struct UIBackdropBox backdrop;
    struct UIBitmapButton *bar_buttons[3];
    int combo_kind;
    struct Vec boxes;
    struct UIBitmapButton *tab_option;
    struct UIBitmapButton *mini;
    int edit_icon_x;
    int bar_icon_x;
    int max_tabs;
    int tab;
    int saved_step;
    int tab_height;
    int tab_width;
    int bar_height;
    struct UIBitmapButton *stick;
    struct UIBitmapButton *battle_option;
    struct UIWindow *tabs;
};
OFFSET_ASSERT(UINewChatWnd, backdrop, 0xa4);

#define SIZE_STEP(self) ((self)->frame.base.state)
#define CHAT_BOX(self, i) (((struct UIWindow **)(self)->boxes.first)[i])
#define CHAT_BOX_COUNT(self) ((int)VEC_COUNT(&(self)->boxes, struct UIWindow *))
#define CHAT_TABS(self) ((struct UITabControl *)(self)->tabs)

#define CHAT_ID_BOX 0x1e5
#define CHAT_ID_STICK 0x15e
#define CHAT_ID_BATTLE_OPTION 0xb9
#define CHAT_ID_TAB_OPTION 0x16e
#define CHAT_ID_MINI 0xc2
#define CHAT_ID_BAR_DIALOG 0xd9
#define CHAT_ID_BAR_SYS_1 0xcc
#define CHAT_ID_BAR_SYS_2 0xe6
#define MINI_ID_PLACE 0xbd
#define CHAT_MAX_TABS 10

#define CHAT_LINES_SHOWN (g_windowMgr.chat_wnd_stick_on != 0)

void ui_new_chat_wnd_set_size_step(struct UINewChatWnd *self, int step);
void ui_new_chat_wnd_place_title_buttons(struct UINewChatWnd *self);
void ui_new_chat_wnd_place_bar(struct UINewChatWnd *self);
void ui_new_chat_wnd_show_tab(struct UINewChatWnd *self, BYTE place_boxes);

void ui_window_mgr_no_op(struct UIWindowMgr *self);
struct UIItemTagCtrl;
int ui_chat_edit_ctrl_has_item_link(struct UIChatEditCtrl *self, const struct String *text);
int ui_chat_edit_ctrl_has_navi_link(struct UIChatEditCtrl *self, const struct String *text);

int ui_new_chat_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4);

struct UINewChatWnd *ui_new_chat_wnd_construct(struct UINewChatWnd *self);
struct UIWindow *ui_new_chat_wnd_new(void);
void ui_new_chat_wnd_on_enter(struct UINewChatWnd *self);
void ui_new_chat_wnd_move(struct UIWindow *window, int x, int y);

void ui_chat_wnd_write_chat_to_file(struct UINewChatWnd *self);

#endif
