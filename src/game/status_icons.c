#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/packet_handlers.h"
#include "game/client.h"
#include "game/magic_static.h"
#include "game/session_bridge.h"
#include "game/packet_data.h"
#include "game/state_change_helpers.h"
#include "session/session_items.h"
#include "ui/ui_std.h"

struct Tree g_healthStateIcons;
struct Once g_healthStateIconsGuard;
struct Tree g_bodyStateIcons;
struct Once g_bodyStateIconsGuard;
void health_state_icons_atexit(void);
void body_state_icons_atexit(void);

#define NO_DURATION 0x270f
#define ICON_BLIND_FIRST 0x377
#define ICON_BLIND_LAST 0x379

static void set_icon(struct GameMode *self, int aid, int on, int icon)
{
    game_mode_zc_msg_state_change(self, aid, on, icon, NO_DURATION, 0, 0, 0, 0, NO_DURATION);
}

void game_mode_show_player_states(struct GameMode *self, int body_state, int health_state)
{
    const struct IntIntPair *found;
    struct TreeNode *it;
    int aid;

    MAGIC_STATIC(g_healthStateIconsGuard, state_ids_by_flag, g_healthStateIcons, health_state_icons_atexit);
    MAGIC_STATIC(g_bodyStateIconsGuard, state_ids_by_value, g_bodyStateIcons, body_state_icons_atexit);

    aid = g_session1.ownAid;
    for (it = tree_first(&g_healthStateIcons); it != g_healthStateIcons.head; it = tree_next(it)) {
        const struct IntIntPair *icon = TREE_VALUE(it, const struct IntIntPair);

        if (icon->value < ICON_BLIND_FIRST || icon->value > ICON_BLIND_LAST)
            set_icon(self, aid, (health_state & icon->key) != 0, icon->value);
    }

    found = tree_find(&g_bodyStateIcons, &g_session1.bodyState, tree_less_int);
    if (found)
        set_icon(self, aid, 0, found->value);
    if (body_state != 0) {
        found = tree_find(&g_bodyStateIcons, &body_state, tree_less_int);
        if (found)
            set_icon(self, aid, 1, found->value);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_STATE_CHANGE3 {
    unsigned short id;
    unsigned aid;
    short body_state;
    short health_state;
    int effect_state;
    unsigned char pk_mode;
};
struct PACKET_ZC_STATE_CHANGE {
    unsigned short id;
    unsigned aid;
    short body_state;
    short health_state;
    short effect_state;
    unsigned char pk_mode;
};
#pragma pack(pop)

enum { ACTOR_MSG_STATES = 0x2b };

enum { UIM_EQUIP_STATES = 0x3c };
#define WINDOW_CART 0x28

static void own_states(const struct ActorStates *states)
{
    g_session1.bodyState = states->body_state;
    g_session1.healthState = states->health_state;
    g_session1.effectState = states->effect_state;
    if (g_windowMgr.equip_wnd != 0)
        UIWINDOW_SENDMSG(g_windowMgr.equip_wnd, UIM_EQUIP_STATES, 0, 0, 0, 0);
    if (g_session1.cartType <= 0) {
        session_clear_cart(&g_session1);
        ui_window_mgr_post_quit1(&g_windowMgr, WINDOW_CART);
    }
}

static void tell_actor(struct GameActor *actor, struct ActorStates *states)
{
    if (actor != 0)
        render_object_send(actor, NULL, ACTOR_MSG_STATES, MSG_ARG_PTR(states), 0, 0, 0);
}

void game_mode_zc_state_change3(struct GameMode *self, const struct PACKET_ZC_STATE_CHANGE3 *packet)
{
    struct ActorStates states;
    struct GameActor *actor;

    states.effect_state = packet->effect_state;
    states.body_state = packet->body_state;
    states.health_state = packet->health_state;
    states.pk_mode = packet->pk_mode;
    if (packet->aid == (unsigned)g_session1.ownAid) {
        actor = (struct GameActor *)self->world->player;
        game_mode_show_player_states(self, states.body_state, packet->health_state);
        own_states(&states);
    } else {
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
    }
    tell_actor(actor, &states);
}

void game_mode_zc_state_change(struct GameMode *self, const struct PACKET_ZC_STATE_CHANGE *packet)
{
    struct ActorStates states;
    struct GameActor *actor;

    states.health_state = packet->health_state;
    states.body_state = packet->body_state;
    states.effect_state = packet->effect_state;
    states.pk_mode = packet->pk_mode;
    if (packet->aid == (unsigned)g_session1.ownAid) {
        actor = (struct GameActor *)self->world->player;
        own_states(&states);
    } else {
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
    }
    tell_actor(actor, &states);
}

void health_state_icons_atexit(void)
{
    tree_free(&g_healthStateIcons, NULL);
}

void body_state_icons_atexit(void)
{
    tree_free(&g_bodyStateIcons, NULL);
}
