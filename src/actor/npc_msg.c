#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "ui/window_access.h"
#include "render/renderer.h"
#include "ui/character/gauge.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "game/actor_entry.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "ui/window.h"
#include "ui/wndmgr/wndmgr.h"

struct UIWindow;

static void npc_build_player_gage(struct Npc *self, struct UIWindow *g)
{
    switch (self->object_type) {
    case 7:
        ui_player_gauge_set(g, PLAYER_GAUGE_FOOD, gauge_value_from_int(&g_session1.petFullness), gauge_value_from_int(&g_session1.petFullnessMax));
        ui_window_create(g, 0x3c, 5);
        ui_player_gauge_set_layout(g, 7);
        break;
    case 8:
        ui_player_gauge_set(g, PLAYER_GAUGE_HP, gauge_value_from_int(&g_session1.homunHp), gauge_value_from_int(&g_session1.homunMaxHp));
        ui_player_gauge_set(g, PLAYER_GAUGE_SP, gauge_value_from_int(&g_session1.homunSp), gauge_value_from_int(&g_session1.homunMaxSp));
        ui_player_gauge_set(g, PLAYER_GAUGE_FOOD, gauge_value_from_int(&g_session1.homunFullness), gauge_value_from_int(&g_session1.homunMaxFullness));
        ui_window_create(g, 0x3c, 0xd);
        ui_player_gauge_set_layout(g, 8);
        break;
    case 9:
        ui_player_gauge_set(g, PLAYER_GAUGE_HP, gauge_value_from_int(&g_session1.mercHp), gauge_value_from_int(&g_session1.mercMaxHp));
        ui_player_gauge_set(g, PLAYER_GAUGE_SP, gauge_value_from_int(&g_session1.mercSp), gauge_value_from_int(&g_session1.mercMaxSp));
        ui_window_create(g, 0x3c, 9);
        break;
    case 10:
        ui_player_gauge_set(g, PLAYER_GAUGE_HP, gauge_value_from_int(&g_session1.elementalHp), gauge_value_from_int(&g_session1.elementalMaxHp));
        ui_player_gauge_set(g, PLAYER_GAUGE_SP, gauge_value_from_int(&g_session1.elementalSp), gauge_value_from_int(&g_session1.elementalMaxSp));
        ui_window_create(g, 0x3c, 9);
        break;
    }
}

void npc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Npc *self = (struct Npc *)object;

    if (msg == 4) {
        const struct ActorEntryInfo *info = MSG_PTR(par1);

        game_actor_send_msg(object, source, 4, par1, par2, par3, par4);
        switch (self->object_type) {
        case 8: case 9: case 10:
            self->owner_aid = info->char_id;
            break;
        }
        self->visible = session_is_visible_npc(info->job);
        game_actor_set_guild((struct GameActor *)self, info->guild_id, info->emblem_version);
        return;
    }
    if (msg == 0x22) {
        struct UIWindow *g;

        if (session_is_own_unit_aid(&g_session1, self->aid)) {
            g = self->player_gage;
            if (!g) {
                g = ui_player_gauge_create();
                self->player_gage = g;
                npc_build_player_gage(self, g);
                ui_window_mgr_add_to_window_list(&g_windowMgr, self->player_gage);
                g = self->player_gage;
                if (!g)
                    return;
            }
            ui_window_move(g, self->screen_x - 30, g_renderer->height * 12 / 480 + self->screen_y);
            return;
        }
        g = self->pc_gage;
        if (!g) {
            g = (struct UIWindow *)ui_pc_gage_new();
            self->pc_gage = g;
            ui_window_create(g, 0x3c, 5);
            ui_window_mgr_add_to_window_list(&g_windowMgr, self->pc_gage);
            if (!self->pc_gage)
                return;
        }
        ui_pc_gage_set_hp((struct UIPcGage *)self->pc_gage, (int)par1, (int)par2);
        ui_pc_gage_set_sp((struct UIPcGage *)self->pc_gage, 0, 0);
        g = self->pc_gage;
        ui_window_move(g, self->screen_x - 30, scale_y_from480(12) + self->screen_y);
        return;
    }
    game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
}

void hallu_plant_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct HalluPlant *self = (struct HalluPlant *)object;

    if (msg == 0) {
        render_object_set_argb((struct RenderObject *)self, 0xb4, (int)par1, (int)par2, (int)par3);
        return;
    }
    if (msg == 1) {
        self->argb &= ((unsigned)(int)par1 << 24) + 0xffffff;
        return;
    }
    if (msg == 3) {
        self->motion_held = 1;
        return;
    }
    npc_send_msg(object, source, msg, par1, par2, par3, par4);
}
