#include "ui/window_manager_lifecycle.h"
#include "ui/window_messages.h"
#include "ui/misc/snap_mgr.h"
#include "ui/window_manager_state.h"
#ifndef UI_WINDOW_MGR_H
#define UI_WINDOW_MGR_H

#include "ui/window.h"
#include "ui/window_access.h"

struct BitmapRes;

struct UIWindow *ui_window_mgr_find_by_key(struct UIWindowMgr *self, int key);
void ui_window_mgr_touch_window_id(struct UIWindowMgr *self, int window_id);
void ui_window_mgr_remove_from_name_waiting_list(struct UIWindowMgr *self,
                                                      struct UIWindow *wnd);
void ui_window_mgr_forget_window(struct UIWindowMgr *self, const struct UIWindow *wnd);
int ui_window_mgr_is_friend(struct UIWindowMgr *self, const char *name);
void ui_window_mgr_set_backdrop(struct UIWindowMgr *self,
                                        struct BitmapRes *res);

int ui_window_mgr_get_window_flag(struct UIWindowMgr *self, int kind,
                                         int window_id);
#define UI_WNDFLAG_COMMANDS 3
#define UI_WNDFLAG_SEND_EDITS 6

static __inline struct UIWindow *ui_make_window(int window_id)
{
    return ui_window_open(window_id);
}
static __inline struct UIWindow *ui_get_window(int window_id)
{
    return ui_window_find(window_id);
}
static __inline BYTE ui_close_window(int window_id)
{
    return ui_window_mgr_post_quit1(&g_windowMgr, window_id);
}

static __inline void ui_chat_line(const char *text, COLORREF colour)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)text, (int)colour, 0, 0);
}

void ui_window_mgr_set_focus_edit(struct UIWindowMgr *self,
                                          struct UIWindow *edit);
struct UIWindow *ui_window_mgr_get_capture(struct UIWindowMgr *self);
void ui_window_mgr_set_capture(struct UIWindowMgr *self,
                                       struct UIWindow *wnd);
void ui_window_mgr_release_capture(struct UIWindowMgr *self);

#define UI_MSGBOX_OK 0
#define UI_MSGBOX_QUESTION 2
#define UI_MSGBOX_ANSWER_YES 0xb3
static __inline int ui_message_box(const char *text)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_OK, 1, 0, 0, "Message",
                                  0x118, 0x78, 0);
}

struct UIWindow *ui_normal_mb_new(void);
void ui_normal_mb_create(struct UIWindow *self, struct UIWindow *owner, const char *text,
                                  int type, int arg3, int arg4, int width, int height, int arg7);
void ui_normal_mb_set_answer(struct UIWindow *window, int id, void (*run)(void));

void ui_window_mgr_get_snap_delta(struct UIWindowMgr *self, struct UIWindow *wnd,
                                         int *dx, int *dy, int flag);
void ui_window_mgr_update_snap_rect(struct UIWindowMgr *self,
                                           struct UIWindow *wnd);
struct SnapMgr;

int ui_window_mgr_run_email_address_box(struct UIWindowMgr *self);

#endif
