#include "game/drag.h"
#include "world/actor_lookup.h"
#include "session/shop_inventory.h"
#include "session/hotkey_bindings.h"
#ifndef CONTENTS_GAME_SESSION_H
#define CONTENTS_GAME_SESSION_H

#include "actor/render_object.h"
#include "render/vec3.h"
#include "session/state.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "game/context.h"

#include "base.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"

struct SessionAchievementState {
    char known;
    char completed;
    int objectives[10];
    int completed_at;
    char rewarded;
    short year, month, day;
};

#define MODE_SEND_MSG(mode, msg, a, b, c, d) \
    mode_send_message((struct Mode *)(mode), (msg), (int)(a), (int)(b), (int)(c), (int)(d))

#include "effects/effect_mgr.h"

#endif
