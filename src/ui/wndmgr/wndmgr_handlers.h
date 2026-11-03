#ifndef UI_WNDMGR_WNDMGR_HANDLERS_H
#define UI_WNDMGR_WNDMGR_HANDLERS_H

struct Session;
struct UIChatRoomWnd;
struct UINewChatWnd;
struct UIWindow;

/* Window-specific steps the window manager (ui/wndmgr) hands to the module that owns
   the window. There is one instance, g_windowMgrHandlers (ui/wndmgr/wndmgr_handlers.c);
   client_install_hooks() in core/client_init.c fills it in once at startup, before any
   window exists. Each member says who provides it and what the window manager does
   while it is NULL. */
struct WindowMgrHandlers {
    /* The chat window's (new_chat_wnd) input line and its whisper-name line.
       ui_chat_window_edit and ui_chat_window_whisper_edit (ui/chat/chat_window.c).
       NULL: the window manager takes the chat window to have no such line: moving the
       focus to the chat window does nothing, battle mode clears nothing, and no focused
       edit counts as the chat window's. */
    struct UIWindow *(*chat_edit)(struct UIWindow *chat);
    struct UIWindow *(*chat_whisper_edit)(struct UIWindow *chat);
    /* UIWM_MSG_SAVE_CHAT: writes the chat window's log to a file.
       ui_chat_wnd_write_chat_to_file (ui/chat/chat_window_log.c). NULL: nothing is
       written. */
    void (*save_chat_log)(struct UINewChatWnd *chat);
    /* UIWM_MSG_SAVE_CHAT_ROOM: writes the open chat room's talk to a file.
       ui_chat_room_wnd_save_talk (ui/chat/chat_room.c). NULL: nothing is written. */
    void (*save_chat_room_talk)(struct UIChatRoomWnd *room);
    /* The slot of the skill hotkey in the first hotkey list that key `code` triggers,
       or -1. session_skill_hot_key_slot (session/session_hotkeys.c). NULL: -1, no skill
       hotkey. */
    int (*skill_hot_key_slot)(struct Session *session, int code);
};

extern struct WindowMgrHandlers g_windowMgrHandlers;

#endif
