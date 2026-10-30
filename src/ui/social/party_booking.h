#include "web/service_managers.h"
#include "contents/guild_info.h"
#ifndef UI_SOCIAL_PARTY_BOOKING_H
#define UI_SOCIAL_PARTY_BOOKING_H

#include <string.h>
#include "ui/social/social.h"
#include "web/party_booking_data.h"

#define AGENCY UI_SKIN "UIadventurerAgency\\"

const char *party_purpose_name(int purpose);

static __inline void party_draw_roles(struct UIWindow *wnd, const BYTE *roles, int x, int y)
{
    static const char *const icons[4][2] = {
        { AGENCY "btn_job_def_on.bmp", AGENCY "btn_job_def_off.bmp" },
        { AGENCY "btn_job_heal_on.bmp", AGENCY "btn_job_heal_off.bmp" },
        { AGENCY "btn_job_atk_on.bmp", AGENCY "btn_job_atk_off.bmp" },
        { AGENCY "btn_job_buf_on.bmp", AGENCY "btn_job_buf_off.bmp" },
    };
    int i;

    for (i = 0; i < 4; i++)
        ui_window_draw_bitmap(wnd, x + 0x12 * i, y,
                            res_mgr_get(g_resMgr(), ui_skin_path(icons[i][roles[i] == 1 ? 0 : 1])), 0);
}

void party_booking_send_name(const struct AdvenPartyInfo *ad);

struct UIPartyInfoWnd {
    struct UIBitmapButton button;
    struct String memo;
    struct String levels;
    int purpose;
    int roles[4];
    struct String name;
    struct AdvenPartyInfo ad;
};
struct UIPartyInfoWnd *ui_party_info_wnd_construct(struct UIPartyInfoWnd *self);
int ui_party_info_wnd_set_ad(struct UIPartyInfoWnd *self, const struct AdvenPartyInfo *ad);
void ui_party_info_wnd_reset(struct UIPartyInfoWnd *self);

struct UIApplyForPartyWnd {
    struct UIFrameWnd frame;
    struct AdvenPartyInfo ad;
    struct WindowInfo *info;
};
struct UIApplyForPartyWnd *ui_apply_for_party_wnd_construct(struct UIApplyForPartyWnd *self);
struct UIWindow *ui_apply_for_party_wnd_new(void);
int ui_apply_for_party_wnd_set_ad(struct UIApplyForPartyWnd *self, const struct AdvenPartyInfo *ad);
void ui_apply_for_party_wnd_on_mouse_hover(struct UIWindow *window, int x, int y);

#endif
