#include "world/mode_actions.h"
#ifndef WORLD_MODE_H
#define WORLD_MODE_H

#include "world/tooltip.h"
#include "world/world_imports.h"
#include "world/mode_manager.h"
#include "world/mode_dispatch.h"

#define MODE_MSG_QUIT           2
#define MODE_MSG_SCREENSHOT     0x44

#define WM_MSG_CHAT_LINE        1
#define WM_MSG_CURSOR_ACTION    0xf
#define WM_MSG_CURSOR_MOTION    0x10

#define CURSOR_ACT_ONCE         1

#include "world/mode_callbacks.h"

extern DWORD g_fadeStart;

#include "render/sprite_draw.h"

#endif
