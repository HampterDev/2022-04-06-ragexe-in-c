#include "ui/window_messages.h"
#include "world/object_lookup.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "ui/window_access.h"
#include "world/actor_lookup.h"
#include "contents/connection_api.h"
#ifndef GAMEMODE_OBJECTS_H
#define GAMEMODE_OBJECTS_H

#include <windows.h>
#include <mmsystem.h>

#include "game/gamemode.h"
#include "render/renderer.h"
#include "base.h"
#include "ui/window_types.h"

struct UIWindowMgr;
struct UIWindow;

enum {
    UIM_PUSH_INTO_CHAT = 1,
};
#define CHAT_COLOR_YELLOW   0x00ffff
#define CHAT_COLOR_CYAN     0xffff00
#define CHAT_COLOR_RED  0xff

struct ModeMgr;

struct GameActor;

#include "actor/render_ops.h"

struct Renderer;

#include "effects/effect_mgr.h"

struct Session;

struct RagConnection;

#endif
