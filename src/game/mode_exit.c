#include "contents/adventurer_agency.h"
#include "game/gamemode.h"
#include "world/view_api.h"
#include "world/lifecycle.h"
#include "render/rp_pool.h"
#include "render/renderer.h"
#include "render/sprite_atlas.h"
#include "render/texture_cache.h"
#include "contents/schedule.h"
#include "fileio/resource_api.h"
#include "contents/sound.h"
#include "world/pick_node.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "game/mode_callbacks.h"
#include <windows.h>
#include <string.h>
#include "game/camera.h"
#include "game/client.h"
#include "effects/effect.h"
#include "render/cache_lock.h"
#include "effects/effect_mgr.h"
#include "game/chat.h"
#include "session/session_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/win_utf8.h"

void game_mode_on_exit(struct Mode *mode)
{
    struct GameMode *self = (struct GameMode *)mode;
    char path[0x200];
    HKEY key;
    DWORD disposition;
    struct Schedule *schedule;

    adventurer_agency_reset(g_adventurerAgency);
    schedule = time_scheduler_find(g_timeScheduler, g_session1.gameModeScheduleId);
    if (schedule != 0)
        schedule->alive = 0;

    sound_end_samples();
    if (self->world != 0) {
        world_clear_map(self->world);
        if (self->world != 0)
            world_delete(self->world);
        self->world = NULL;
    }
    if (self->view != 0) {
        view_delete(self->view);
        self->view = NULL;
    }
    actor_pick_node_clear(&g_actorPickNode);
    res_mgr_purge_by_extension(g_resMgr(), "rsm");
    res_mgr_purge_by_extension(g_resMgr(), "rsw");
    res_mgr_purge_by_extension(g_resMgr(), "gat");
    cache_purge(g_resMgr());
    texcache_evict(&g_textureCache);
    renderer_destroy_all_rp_list(g_renderer);
    sprite_atlas_clear(g_renderer);
    ui_window_mgr_write_shortcut_item_info_to_reg(&g_windowMgr);

    if (self->preferences_loaded) {
        strcpy(path, g_registryKeyPath);
        strcat(path, "\\NameBaloon");
        strcat(path, "\\");
        strcat(path, string_cstr(&g_session1.serverName));
        disposition = 0;
        if (win_utf8_reg_create_key(HKEY_CURRENT_USER, path, 0, NULL, 0, 0xf003f, NULL, &key, &disposition) == 0) {
            win_utf8_reg_set_value(key, "POS", 0, REG_DWORD, (const BYTE *)&self->name_balloon_pos, 4);
            RegCloseKey(key);
        }

        if (self->camera_free != 0) {
            g_freeViewPitch = g_viewPitch;
            g_freeViewDistance = g_viewDistance;
        } else {
            g_fixedViewPitch = g_viewPitch;
            g_fixedViewDistance = g_viewDistance;
        }
    }
    if (g_session1.autoSaveChat != 0)
        game_mode_close_chat_log();
    effect_mgr_reset(g_effectMgr);
    session_save_mini_party_info(&g_session1);
}
