#ifndef UI_CHAT_SUB_CHAT_H
#define UI_CHAT_SUB_CHAT_H

#include "ui/chat/chat.h"

struct UIChatHisBox;
struct UIResizer;
struct UIBitmapButton;
struct WindowInfo;

struct UISubChatHisWnd {
    struct UIFrameWnd frame;
    struct UIChatHisBox *box;
    struct WindowInfo *info;
    struct UIResizer *resizer;
    struct UIBackdropBox backdrop;
    struct UIBitmapButton *stick;
    struct UIBitmapButton *battle_option;
    struct UIBitmapButton *close;
    int tab_height;
    int tab_width;
    int bottom;
    int key;
    BYTE shown[CHAT_TAB_FLAG_COUNT];
    struct String name;
    struct UIWindow *rename;
};
OFFSET_ASSERT(UISubChatHisWnd, backdrop, 0xa0);
OFFSET_ASSERT(UISubChatHisWnd, stick, 0xbc);
OFFSET_ASSERT(UISubChatHisWnd, key, 0xd4);
OFFSET_ASSERT(UISubChatHisWnd, name, 0xf0);

struct UISubChatHisWnd *ui_sub_chat_his_wnd_construct(struct UISubChatHisWnd *self);
struct UIWindow *ui_sub_chat_his_wnd_new(void);
struct UISubChatMiniWnd *ui_sub_chat_mini_wnd_construct(struct UISubChatMiniWnd *self);
struct UIWindow *ui_sub_chat_mini_wnd_new(void);

#endif
