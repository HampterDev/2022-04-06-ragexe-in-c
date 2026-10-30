#include "skills/skill_info.h"
#include "session/character_name.h"
#include "contents/manner_filter.h"
#include "session/state.h"
#include "contents/connection_api.h"
#ifndef UI_SOCIAL_H
#define UI_SOCIAL_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_std.h"

static __inline struct UIBitmapButton *ui_add_bitmap_button(struct UIWindow *parent, const char *normal,
                                                            const char *hover, const char *pressed, int x, int y,
                                                            int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, normal, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, pressed, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(parent, &button->button.base);
    return button;
}

static __inline struct UIWindow *ui_add_text_viewer(struct UIWindow *parent, int x, int y, int w, int h)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UITextViewer));

    if (text)
        text = ui_text_viewer_construct(text);
    ui_list_box_create_window((struct UIListBox *)text, x, y, w, h, 0);
    UI_TEXT_VIEWER_BACKGROUND(text) = ui_skin_argb(2, 2);
    ui_window_invalidate(text);
    ui_window_add_child(parent, text);
    return text;
}

static __inline void ui_send_packet(const void *packet)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), *(const short *)packet);

    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

#define SKILL_INFO_UPGRADABLE(info) ((info)->upgradable)
#define SKILL_INFO_NAME(info) ((info)->id_name)

struct UIAcceptWnd *ui_ally_guild_accept_wnd_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_ally_guild_accept_wnd_new(void);
struct UIAcceptWnd *ui_baby_accept_wnd2_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_baby_accept_wnd2_new(void);
struct UIAcceptWnd *ui_baby_accept_wnd_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_baby_accept_wnd_new(void);
struct UIAcceptWnd *ui_couple_accept_wnd_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_couple_accept_wnd_new(void);
struct UIAcceptWnd *ui_join_guild_accept_wnd_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_join_guild_accept_wnd_new(void);
struct UIAcceptWnd *ui_join_party_accept_wnd_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_join_party_accept_wnd_new(void);
struct UIStarPlaceAcceptWnd *ui_star_place_accept_wnd_construct(struct UIStarPlaceAcceptWnd *self);
struct UIWindow *ui_star_place_accept_wnd_new(void);

struct UIAcceptWnd *ui_choose_sell_buy_wnd_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_choose_sell_buy_wnd_new(void);
struct UIAcceptWnd *ui_exchange_accept_wnd_construct(struct UIAcceptWnd *self);
struct UIWindow *ui_exchange_accept_wnd_new(void);

struct UIClanInfoManageWnd *ui_clan_info_manage_wnd_construct(struct UIClanInfoManageWnd *self);
struct UIWindow *ui_clan_info_manage_wnd_new(void);

struct UIEmblemWnd *ui_emblem_wnd_new(void);

struct UICreateGuildWnd *ui_create_guild_wnd_construct(struct UICreateGuildWnd *self, int disband);
struct UIWindow *ui_create_guild_wnd_new(int arg);
struct UIGuildHelperWnd *ui_guild_helper_wnd_construct(struct UIGuildHelperWnd *self);
struct UIWindow *ui_guild_helper_wnd_new(void);
struct UIGuildTipWnd *ui_guild_tip_wnd_construct(struct UIGuildTipWnd *self);
struct UIWindow *ui_guild_tip_wnd_new(void);

struct UIGuildBanReasonDescWnd *ui_guild_ban_reason_desc_wnd_construct(struct UIGuildBanReasonDescWnd *self);
struct UIWindow *ui_guild_ban_reason_desc_wnd_new(void);
struct UIGuildLeaveReasonDescWnd *ui_guild_leave_reason_desc_wnd_construct(
    struct UIGuildLeaveReasonDescWnd *self);
struct UIWindow *ui_guild_leave_reason_desc_wnd_new(void);

struct UIGuildSkillWnd *ui_guild_skill_wnd_construct(struct UIGuildSkillWnd *self);
struct UIWindow *ui_guild_skill_wnd_new(void);

struct UIGuild_Storage_Log *ui_guild_storage_log_construct(struct UIGuild_Storage_Log *self);
struct UIWindow *ui_guild_storage_log_new(void);

struct UIMemorialDunWnd *ui_memorial_dun_wnd_construct(struct UIMemorialDunWnd *self);
struct UIWindow *ui_memorial_dun_wnd_new(void);

struct UIPartyBookingHelpWnd *ui_party_booking_help_wnd_construct(struct UIPartyBookingHelpWnd *self);
struct UIWindow *ui_party_booking_help_wnd_new(void);

struct UIPartyInvitationToWnd *ui_party_invitation_to_wnd_construct(struct UIPartyInvitationToWnd *self);
struct UIWindow *ui_party_invitation_to_wnd_new(void);
struct UIPartySettingWnd *ui_party_setting_wnd_construct(struct UIPartySettingWnd *self);
struct UIWindow *ui_party_setting_wnd_new(void);

#endif
