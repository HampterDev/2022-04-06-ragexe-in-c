#ifndef GAMEMODE_STATE_CHANGE_HELPERS_H
#define GAMEMODE_STATE_CHANGE_HELPERS_H

#include "game/mode_util.h"
#include "actor/actor_ops.h"
#include "actor/render_ops.h"
#include "skills/skill_action_registry.h"
#include "session/state.h"
#include <string.h>
#include "game/packet_handlers.h"
#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "game/own_states.h"
#include "actor/actor_common.h"
#include "core/msg_str.h"

enum {
    ACTOR_MSG_SLEEP_MARK_OFF = 0x46,
    ACTOR_MSG_FORM_OFF = 0x6a,
    ACTOR_MSG_TINT_OFF = 0x80,
};

void game_mode_zc_msg_state_change(struct GameMode *mode, int aid, int state,
                                               int index, int remain_ms, int val1, int val2, int val3,
                                               int from_entry, int total_ms);

enum { EFST_EQUIPMENT_EFFECT = 0x37f };

static void actor_msg(void *actor, int msg, int par1)
{
    render_object_send(actor, NULL, msg, par1, 0, 0, 0);
}

static void remove_all_effects(struct GameActor *actor, int effect_id)
{
    if (able_to_make_effect_is_effect_exist(actor, effect_id)) {
        do
            actor_msg(actor, ACTOR_MSG_REMOVE_EFFECT, effect_id);
        while (able_to_make_effect_is_effect_exist(actor, effect_id));
    }
}

enum {
    MSG_PARTY_MEMBER_PREFIX = 0x4fb,
    STATE_COLOR_BEGIN = 0x00ffff00,
    STATE_COLOR_END = 0x000000ff,
    CHAT_CHANNEL_OWN_STATE = 7,
    CHAT_CHANNEL_MEMBER_STATE = 9,
};

static void say(char *text, int msg_id)
{
    sprintf(text, msg_str(msg_id));
}

static void no_line(char *text)
{
    sprintf(text, "");
}

#endif
