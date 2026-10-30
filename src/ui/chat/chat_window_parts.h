#ifndef UI_CHAT_CHAT_WINDOW_PARTS_H
#define UI_CHAT_CHAT_WINDOW_PARTS_H

struct UIWindow;

struct UIWindow *ui_chat_window_edit(struct UIWindow *chat);
struct UIWindow *ui_chat_window_whisper_edit(struct UIWindow *chat);
struct UIWindow *ui_chat_window_focus_edit(struct UIWindow *chat);
struct UIWindow *ui_chat_window_tabs(struct UIWindow *chat);

#endif
