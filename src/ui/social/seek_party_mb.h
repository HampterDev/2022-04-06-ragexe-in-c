#ifndef UI_SOCIAL_SEEK_PARTY_MB_H
#define UI_SOCIAL_SEEK_PARTY_MB_H

struct UIWindow;

const short *ui_seek_party_mb_jobs(const struct UIWindow *wnd);

struct UISeekPartyMBWnd *ui_seek_party_mb_wnd_construct(struct UISeekPartyMBWnd *self);
struct UIWindow *ui_seek_party_mb_wnd_new(void);

#endif
