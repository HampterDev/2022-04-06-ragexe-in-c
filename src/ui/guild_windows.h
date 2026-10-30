#ifndef UI_GUILD_WINDOWS_H
#define UI_GUILD_WINDOWS_H
struct UIWindow;
struct UIWindowMgr;
struct UIWindow *window_mgr_guild_window(struct UIWindowMgr *manager);
void window_mgr_invalidate_guild_wnd(struct UIWindowMgr *manager);
#endif
