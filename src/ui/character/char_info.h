#include "session/character_name.h"
#include "session/map_names.h"
#include "session/state.h"
#include "world/mode_manager.h"
#ifndef UI_CHARACTER_CHAR_INFO_H
#define UI_CHARACTER_CHAR_INFO_H

#include "ui/game.h"
#include "ui/character/char_preview.h"
#include "login/char_info.h"

#define MODE_MSG_CHAR_INFO 8
#define CHAR_SEX_ACCOUNT 0x63
static __inline struct CharInfo *ui_char_info(int slot)
{
    return (struct CharInfo *)mode_send_message(client_current_mode(), MODE_MSG_CHAR_INFO, slot, 0, 0, 0);
}

struct LoginMode;

#endif
