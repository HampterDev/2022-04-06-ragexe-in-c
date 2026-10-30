#include "ui/items/item_tags.h"
#include "platform/service.h"
#include "contents/manner_filter.h"
#include "session/state.h"
#include "session/session_talk.h"
#include "ui/window_manager_state.h"
#ifndef UI_CHAT_H
#define UI_CHAT_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_std.h"

#define WND_ID_COMBO_BOX 0x1a
#define WND_ID_CHAT_ROOM_MAKE 0x1b
#define WND_ID_CHAT_ROOM_CHANGE 0x1e
#define WND_ID_JOIN_FRIEND_ACCEPT 0x6d
#define WND_ID_CHAT_MACRO 0x56
#define WND_ID_EMOTION_LIST 0x57
#define WND_ID_NOTICE 0x5d
#define WND_ID_CHAT 1
#define WND_ID_BATTLE_MSG_OPTION 0x84

#define CHAT_ROOM_PRIVATE 0
#define CHAT_ROOM_NON_CLICKABLE 3
#include "game/chat_room.h"

#include "ui/chat/sub_chat_map.h"

void ui_window_mgr_close_sub_chat(struct UIWindowMgr *self, int key);

#define CHAT_BACK_ARGB 0x66000000
#define CHAT_COLOUR_KEY 0xffff00ff
#define CHAT_MSG_STICK_TAB 0x76
#define BATTLE_OPTION_FOR_TAB 0xb9

struct UISubChatHisWnd;
void ui_sub_chat_his_wnd_place_buttons(struct UISubChatHisWnd *self);

#define CHAT_TABS_EDITABLE (g_session1.canDragWnd != 0)

#include "ui/chat/edit_history.h"

struct UIListBox;

#define MENU_ENTRIES(v) ((struct UIMenuEntry *)(v)->first)
#define MENU_ENTRY_COUNT(v) ((unsigned)VEC_COUNT(v, struct UIMenuEntry))

void ui_window_mgr_forget_whisper_wnd(struct UIWindowMgr *self, struct String name);

void ui_window_mgr_set_whisper_name(struct UIWindowMgr *self, const char *name);
void ui_window_mgr_add_whisper_name(struct UIWindowMgr *self, const char *name);

struct UIBattleMsgOptionWnd *ui_battle_msg_option_wnd_construct(struct UIBattleMsgOptionWnd *self);
struct UIWindow *ui_battle_msg_option_wnd_new(void);

struct UIChatRoomMakeWnd *ui_chat_room_change_wnd_construct(struct UIChatRoomMakeWnd *self);
struct UIWindow *ui_chat_room_change_wnd_new(void);
struct UIChatRoomMakeWnd *ui_chat_room_make_wnd_construct(struct UIChatRoomMakeWnd *self);
struct UIWindow *ui_chat_room_make_wnd_new(void);

struct UIEmotionListWnd *ui_emotion_list_wnd_construct(struct UIEmotionListWnd *self);
struct UIWindow *ui_emotion_list_wnd_new(void);
struct UIWindow *ui_emotion_wnd_new(void);

struct UIFriendOptionWnd *ui_friend_option_wnd_construct(struct UIFriendOptionWnd *self);
struct UIWindow *ui_friend_option_wnd_new(void);

struct UIJoinFriendAcceptWnd *ui_join_friend_accept_wnd_construct(struct UIJoinFriendAcceptWnd *self);
struct UIWindow *ui_join_friend_accept_wnd_new(void);

struct UIGuildNoticeWnd *ui_guild_notice_wnd_construct(struct UIGuildNoticeWnd *self);
struct UIWindow *ui_guild_notice_wnd_new(void);

void macro_reporter_request_players_in_range(WORD x, WORD y, BYTE radius);
struct UIMacroReporterWnd;
void ui_macro_reporter_wnd_add_player(struct UIMacroReporterWnd *self, int aid);
struct UIMacroReporterWnd *ui_macro_reporter_wnd_construct(struct UIMacroReporterWnd *self);
struct UIWindow *ui_macro_reporter_wnd_new(void);
struct UIMacroUserInfoWnd *ui_macro_user_info_wnd_construct(struct UIMacroUserInfoWnd *self);
struct UIWindow *ui_macro_user_info_wnd_new(void);

struct UINoticeConfirmWnd *ui_notice_confirm_wnd_construct(struct UINoticeConfirmWnd *self);
struct UIWindow *ui_notice_confirm_wnd_new(void);

struct UINoticeWnd *ui_notice_wnd_construct(struct UINoticeWnd *self);
struct UIWindow *ui_notice_wnd_new(void);

struct UIProhibitListWnd *ui_prohibit_list_wnd_construct(struct UIProhibitListWnd *self);
struct UIWindow *ui_prohibit_list_wnd_new(void);

struct UISkillTextWnd *ui_graffi_strbox_wnd_construct(struct UISkillTextWnd *self);
struct UIWindow *ui_graffi_strbox_wnd_new(void);
struct UISkillTextWnd *ui_talkbox_trap_input_wnd_construct(struct UISkillTextWnd *self);
struct UIWindow *ui_talkbox_trap_input_wnd_new(void);

#endif
