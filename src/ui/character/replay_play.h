#ifndef UI_CHARACTER_REPLAY_PLAY_H
#define UI_CHARACTER_REPLAY_PLAY_H

struct UIWindow;
struct UIListBox;

struct UIListBox *ui_select_replay_list(struct UIWindow *wnd);

struct UIReplayControlWnd *ui_replay_control_wnd_construct(struct UIReplayControlWnd *self);
struct UIWindow *ui_replay_control_wnd_new(void);
struct UISelectReplayDataWnd *ui_select_replay_data_wnd_construct(struct UISelectReplayDataWnd *self);
struct UIWindow *ui_select_replay_data_wnd_new(void);

#endif
