#ifndef UI_CHARACTER_SEL_CHAR_USERVER_H
#define UI_CHARACTER_SEL_CHAR_USERVER_H

#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "ui/character/character.h"

#include "ui/character/char_preview.h"
#include "ui/character/character_draw.h"

#include "ui/character/char_info.h"
#define CMODE_GET_CHAR_INFO 8

void ui_window_draw_robe(struct UIWindow *self, int job, int sex, int robe, int x, int y,
                                  BYTE scaled, int action, int motion);

#include "ui/chat/balloon.h"

#pragma pack(push, 1)
struct PACKET_CZ_SELECT_CHARS {
    WORD id;
    int aid;
    int count;
    int gids[9];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_CZ_SELECT_CHARS, 0x2e);

#define SEL_CHAR_MAX 0x12
struct UISelCharForUServerWnd {
    struct UIFrameWnd frame;
    struct CharPreview looks[SEL_CHAR_MAX];
    int hidden[SEL_CHAR_MAX];
    short headtop[SEL_CHAR_MAX];
    DWORD start_time;
    struct UIWindow *checks[SEL_CHAR_MAX];
    struct UIWindow *ok;
    struct UIWindow *balloon;
    struct Vec lines;
    int checked;
    int columns;
    struct UIAnimation animation;
    unsigned long long drawn_key;
};

struct UISelCharForUServerWnd *ui_sel_char_for_u_server_wnd_construct(struct UISelCharForUServerWnd *self);
struct UIWindow *ui_sel_char_for_u_server_wnd_new(void);

#endif
