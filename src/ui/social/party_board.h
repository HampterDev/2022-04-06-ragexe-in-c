#include "contents/adventurer_agency.h"
#ifndef UI_SOCIAL_PARTY_BOARD_H
#define UI_SOCIAL_PARTY_BOARD_H

#include "ui/social/party_booking.h"

#define PARTY_BOARD_WND_ID 0x144
#define PARTY_REGISTER_WND_ID 0x143
#define PARTY_APPLY_WND_ID 0x159

#define BOARD_ID_NEXT_PAGE 0xd2
#define BOARD_ID_PREV_PAGE 0xee
#define BOARD_ID_RESET 0x163
#define BOARD_ID_SEARCH 0x165
#define BOARD_ID_ROW 0x173
#define BOARD_ID_BOOKING_OFF 0x192
#define BOARD_ID_REFRESH 0x197
#define BOARD_ID_OWN_AD 0x22c
#define BOARD_ID_RECRUIT 0x22d

#define BOARD_MSG_REFRESH 0x3c
#define BOARD_MSG_PAGE_BUTTONS 0xa5
#define UI_MSG_COMBO_CHOSE 0x27

#define PARTY_LEVEL_RANGE 5

#define MSG_PURPOSE_ANY 0xd31
#define MSG_PURPOSE_FIRST 0xd20
#define PURPOSE_COUNT 7

struct UIJobSlotButton {
    struct UIBitmapButton bmp;
    struct String on_bitmap;
    struct String off_bitmap;
    BYTE checked;
};
void ui_job_slot_button_set_checked(struct UIJobSlotButton *self, BYTE checked);
void party_make_job_slots(struct UIWindow *parent, struct UIJobSlotButton **slots, const int at[4][2], BYTE checked);
struct UIWindow *party_make_purpose_combo(struct UIWindow *parent, int x, int y, int width);
void party_level_text(struct UIWindow *edit, int level);
void party_button_caption(struct UIBmpTextButton *button, const char *text);

struct UIAdvenPartyBoardWnd {
    struct UIFrameWnd frame;
    struct UIWindow *page_text;
    struct UIWindow *refresh;
    struct UIWindow *search_text;
    struct UIPartyInfoWnd *rows[10];
    struct UIPartyInfoWnd *own_row;
    struct UIBmpTextButton *recruit;
    struct UIWindow *booking_off;
    struct UIBitmapButton *prev_page;
    struct UIBitmapButton *next_page;
    int selected_row;
    int hover_row;
    int page;
    int pages;
    BYTE first_open;
    int ad_count;
    struct WindowInfo *info;
    struct UIWindow *min_level;
    struct UIWindow *max_level;
    struct UIJobSlotButton *roles[4];
    struct UIWindow *purpose_combo;
    struct Vec menu;
    int purpose;
    BYTE searching;
};

struct UIRegisterPartyWnd {
    struct UIFrameWnd frame;
    struct UIWindow *purpose_combo;
    struct UIWindow *min_level;
    struct UIWindow *max_level;
    struct UIWindow *memo;
    struct UIJobSlotButton *roles[4];
    int purpose;
    struct UIWindow *name_text;
    struct UIWindow *recruit;
    struct UIWindow *change;
    struct WindowInfo *info;
};
void ui_register_party_wnd_set_ad(struct UIRegisterPartyWnd *self, const struct AdvenPartyInfo *ad);
BYTE party_is_number(struct String text);

void ui_adven_party_board_wnd_set_own_ad(struct UIAdvenPartyBoardWnd *self, const struct AdvenPartyInfo *ad);
void ui_adven_party_board_wnd_set_has_ad(struct UIAdvenPartyBoardWnd *self, BYTE has_ad);

struct UIAdvenPartyBoardWnd *ui_adven_party_board_wnd_construct(struct UIAdvenPartyBoardWnd *self);
struct UIWindow *ui_adven_party_board_wnd_new(void);

struct UIRegisterPartyWnd *ui_register_party_wnd_construct(struct UIRegisterPartyWnd *self);
struct UIWindow *ui_register_party_wnd_new(void);

struct UIRequestJoinPartyMgr;
void ui_request_join_party_mgr_add_request(struct UIRequestJoinPartyMgr *self, int char_id, int aid, const char *name, int level,
                           int job);
char ui_request_join_party_mgr_create(void);
void ui_request_join_party_mgr_delete(void);
struct UIRequestJoinPartyWnd *ui_request_join_party_wnd_construct(struct UIRequestJoinPartyWnd *self);
struct UIWindow *ui_request_join_party_wnd_new(void);

extern struct UIRequestJoinPartyMgr *g_requestJoinPartyMgr;

#endif
