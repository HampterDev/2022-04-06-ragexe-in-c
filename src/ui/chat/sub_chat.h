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

struct UISubChatHisWnd *ui_sub_chat_his_wnd_construct(struct UISubChatHisWnd *self);

/* Shows sub-chat window `key`, creating it at (x, y) and size (width, height) clamped
   to the screen when it does not exist yet (x == y == -1: cascade by key). A UIWindowMgr
   method in the original client, kept under its name in ui/chat/sub_chat.c. */
struct UIWindow *ui_window_mgr_open_sub_chat(struct UIWindowMgr *self, int key, int x, int y,
                                                    int width, int height);

#endif
