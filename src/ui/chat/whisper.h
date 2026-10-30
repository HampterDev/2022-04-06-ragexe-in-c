#ifndef UI_CHAT_WHISPER_H
#define UI_CHAT_WHISPER_H

#include "ui/window.h"

struct UIChatEditCtrl;
struct UIChatHisBox2;
struct WindowInfo;

struct UIWhisperWnd {
    struct UIFrameWnd frame;
    struct String name;
    struct String title_name;
    struct UIWindow *resizer;
    struct UIChatEditCtrl *edit;
    struct UIChatHisBox2 *box;
    struct UIWindow *close;
    struct Vec menu;
    struct WindowInfo *info;
    struct String code;
};

struct UIWhisperWnd *ui_whisper_wnd_construct(struct UIWhisperWnd *self);
int ui_whisper_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4);

#endif
