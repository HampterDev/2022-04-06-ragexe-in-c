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

struct UIWindowMgr;

/* g_windowMgrHandlers.open_whisper for UIWM_MSG_WHISPER: opens the whisper window for
   `name`, titled "name  (title)  *^_^*" when `title` is given and carrying `code`, unless
   one is open. Returns 1 when it opened one. */
int ui_whisper_wnd_open(struct UIWindowMgr *mgr, const char *name, const char *title, const char *code);

/* Shows an incoming whisper `line` from `name` (character `cid`), opening that sender's
   whisper window first when the whisper-window options ask for it. Returns 1 when the
   line went to a whisper window. A UIWindowMgr method in the original client, kept under
   its name. */
int ui_window_mgr_add_whisper_chat_to_whisper_wnd(struct UIWindowMgr *self, const char *name, const char *line,
                                                       unsigned colour, int cid);
intptr_t ui_whisper_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                    intptr_t val3, intptr_t val4);

#endif
