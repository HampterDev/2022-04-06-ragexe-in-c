#include "ui/window_messages.h"
#include "ui/window_manager_state.h"
#ifndef GAMEMODE_UI_BRIDGE_H
#define GAMEMODE_UI_BRIDGE_H

#include "ui/window_access.h"
#include "base.h"
#include "ui/window_types.h"
#include "game/item_info.h"
#include "ui/window_mgr.h"

struct UIWindowMgr;

enum {
    UIM_PUSH_INTO_CHAT = 1,
    UIM_CLOSE_DIALOG = 3,
};

#define CHAT_COLOR_CYAN 0x00ffff00

#endif
