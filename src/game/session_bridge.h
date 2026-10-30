#include "actor/actor_ops.h"
#ifndef GAMEMODE_SESSION_BRIDGE_H
#define GAMEMODE_SESSION_BRIDGE_H

#include "actor/render_object.h"
#include "actor/render_ops.h"
#include "render/vec3.h"
#include "session/state.h"
#include "world/actor_lookup.h"
#include "contents/connection_api.h"

#include "base.h"
#include "game/party_member.h"

struct Session;

struct RagConnection;

struct World;
struct GameActor;
struct RagEffect;

enum {
    ACTOR_MSG_REMOVE_EFFECT = 0x6d,
    ACTOR_MSG_STATE_CHANGE = 0x6e,
};

#include "effects/effect_mgr.h"

#endif
