#include "game/drag.h"
#include "game/mode_util.h"
#include "ui/items/item_tags.h"
#include "iteminfo/item_list.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_MAIL_RODEX_UI_H
#define UI_MAIL_RODEX_UI_H

#include "ui/items/items.h"
#include "ui/shops/shops.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "ui/hotkeys/hotkeys.h"
#include "ui/chat/markup_links.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"

#define RODEX_DIR UI_SKIN "basic_interface\\rodexsystem\\renewal\\"

#define WID_RODEX_INBOX 0x107
#define WID_RODEX_COMPOSE 0x108
#define WID_RODEX_CONTENTS 0x109
#define WID_ITEM_COMPARE 0xea

#define RODEX_CHAT_COLOUR 0x64ffff
static __inline void rodex_chat(int msg)
{
    ui_chat_line(msg_str(msg), RODEX_CHAT_COLOUR);
}

struct UIEditCtrl3 *ui_rodex_edit_ctrl3_construct(struct UIEditCtrl3 *self);
#define RODEX_SCROLL_BUTTON 5

#define RODEX_TAG_COLOUR 0xd630a9

#define UIM_RODEX_REFRESH 0x17
#define RODEX_ITEM_SLOTS 5

extern int g_rodexBodyMaxChars;

static __inline void rodex_expand_msg_tags(struct String *s)
{
    struct String out = {0};

    navi_links_expand_msg_tags(&out, string_view(s));
    string_assign(s, string_cstr(&out), out.size);
    string_free(&out);
}

#define UIM_RODEX_INBOX_REFRESH 0x3c

#pragma pack(push, 1)
struct PacketRodexTake {
    WORD id;
    long long mail_id;
    BYTE open_type;
};
struct PacketRodexReturn {
    WORD id;
    int mail_id;
};
#pragma pack(pop)
SIZE_ASSERT(PacketRodexTake, 11);

struct Rodex;
void rodex_purge_deleted(struct Rodex *self);

struct UIRodexContentsWnd;
void rodex_contents_wnd_items_taken(struct UIRodexContentsWnd *self);
void rodex_contents_wnd_set_items_flag(struct UIRodexContentsWnd *self, BYTE disabled);
void rodex_contents_wnd_set_zeny_flag(struct UIRodexContentsWnd *self, BYTE disabled);
void rodex_contents_wnd_zeny_taken(struct UIRodexContentsWnd *self);
struct UIRodexContentsWnd *ui_rodex_contents_wnd_construct(struct UIRodexContentsWnd *self);
struct UIWindow *ui_rodex_contents_wnd_new(void);

struct UIRodexInboxWnd;
void rodex_inbox_wnd_remove(struct UIRodexInboxWnd *self, int mail_id);
struct UIRodexInboxWnd *ui_rodex_inbox_wnd_construct(struct UIRodexInboxWnd *self);
struct UIWindow *ui_rodex_inbox_wnd_new(void);
void ui_rodex_inbox_wnd_next_page(struct UIRodexInboxWnd *self);
void ui_rodex_inbox_wnd_rebuild_lists(struct UIRodexInboxWnd *self);
void ui_rodex_inbox_wnd_reset_to_first_page(struct UIRodexInboxWnd *self);
void ui_rodex_inbox_wnd_set_buttons_disabled(struct UIRodexInboxWnd *self, BYTE disabled);

#endif
