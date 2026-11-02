#include "ui/window_messages.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef GAMEMODE_PACKETS_H
#define GAMEMODE_PACKETS_H

#include "ui/window_access.h"
#include "world/actor_lookup.h"
#include "game/gamemode.h"

#include "actor/render_ops.h"
#include "core/msg_str.h"

static __inline void actor_send(struct GameActor *actor, int msg, RenderMsgArg par1, RenderMsgArg par2)
{
    render_object_send(actor, NULL, msg, par1, par2, 0, 0);
}

static __inline intptr_t wnd_send(void *wnd, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    return ui_window_send(wnd, NULL, msg, a, b, c, d);
}

static __inline void message_box(int id)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(id), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

struct Session;

#endif
