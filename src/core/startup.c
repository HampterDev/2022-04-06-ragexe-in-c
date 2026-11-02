#include "contents/adventurer_agency.h"
#include "contents/character_settings.h"
#include "contents/account_settings.h"
#include "actor/map_type_table.h"
#include "contents/town_info.h"
#include "contents/champion_monster.h"
#include "core/map_sky.h"
#include "effects/effect_mgr.h"
#include "effects/ez2str_effect_mgr.h"
#include "session/option_pairs.h"
#include "effects/particle.h"
#include "contents/player_names.h"
#include "render/device.h"
#include "contents/schedule.h"
#include "contents/whisper_block.h"
#include "resources/state_icon_sheet.h"
#include "contents/quest.h"
#include "platform/client_options.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include <shellapi.h>
#include "std/std.h"
#include "render/render_api.h"
#include "core/app.h"
#include "platform/language.h"
#include "world/world_imports.h"
#include "contents/guild_mgr.h"
#include "session/session_state.h"
#include "ui/mail/rodex_box.h"
#include "ui/social/party_board.h"
#include "ui/render/graphic_setting.h"
#include "contents/sound.h"
#include "platform/window.h"

HWND g_hMainWnd;

void app_activate(int active)
{
    g_windowActive = active;
    if (active) {
        RECT client, clip;
        POINT top_left, bottom_right;

        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
        if (g_session1.mouseExclusiveOption != 1)
            return;
        GetClientRect(g_hMainWnd, &client);
        top_left.x = client.left;
        top_left.y = client.top;
        bottom_right.x = client.right;
        bottom_right.y = client.bottom;
        ClientToScreen(g_hMainWnd, &top_left);
        ClientToScreen(g_hMainWnd, &bottom_right);
        clip.left = top_left.x;
        clip.top = top_left.y;
        clip.right = bottom_right.x;
        clip.bottom = bottom_right.y;
        ClipCursor(&clip);
    } else {
        struct IntInt64Pair *var;
        DWORD packet;
        int key = 2, flag;

        SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
        var = tree_find(&g_session1.instantVars, &key, tree_less_int);
        flag = var ? (int)var->value : 0;
        if (flag) {
            packet = 0x07f40b11;
            rag_connection_send_packet(rag_connection_instance(), 4, &packet);
            session_set_pair(&g_session1, 2, 0);
        }
    }
}

int app_init_3d(void)
{
    struct SurfaceDesc mode;
    int flags = g_fullscreenMode ? 7 : 6;

    mode.refreshRate = 0;
    mode.width = g_displayWidth;
    mode.height = g_displayHeight;
    mode.bitCount = g_bitsPerPixel;
    if (c3d_device_init(&g_3dDevice, g_hMainWnd, &g_driverGuid, &g_deviceGuid, &mode, flags) < 0) {
        error_msg("Cannot init d3d OR grf file has problem.", 0);
        return 0;
    }
    return 1;
}

static struct InitResult *init_failed(struct InitResult *out, const char *what)
{
    out->ok = 0;
    string_init(&out->what);
    string_assign_cstr(&out->what, what);
    return out;
}

struct InitResult *app_init_managers(struct InitResult *out)
{
    ez2_str_effect_mgr_load(ez2str_effect_mgr_instance());
    if (!effect_mgr_create())
        return init_failed(out, "CEffectMgr::CreateInstance()");
    if (!effect_memory_create())
        return init_failed(out, "Particle pools");
    if (!time_scheduler_create())
        return init_failed(out, "CTimeScheduler::CreateInstance()");
    town_info_create();
    if (!town_info_load())
        platform_message_box(0, "TownInfo file Init", "Error", 0);
    if (!icon_sheet_create())
        return init_failed(out, "CCooltimerMgr::CreateInstance()");
    if (!renew_quest_create())
        return init_failed(out, "CRenewQuestMgr::CreateInstance()");
    renew_quest_load();
    champion_monster_create();
    if (!champion_monster_load())
        platform_message_box(0, "ChampionMonster file Init", "Error", 0);
    if (!map_property_mgr_create())
        return init_failed(out, "CMapPropertyMgr::CreateInstance()");
    if (!rodex_create_instance())
        return init_failed(out, "CRodexSystemMgr::CreateInstance()");
    if (!account_user_data_create())
        return init_failed(out, "CAccountLinkedUserDataMgr::CreateInstance()");
    if (!char_user_data_create())
        return init_failed(out, "CCharacterLinkedUserDataMgr::CreateInstance()");
    if (!whisper_block_create())
        return init_failed(out, "CWhisperBlockMgr::CreateInstance()");
    if (!player_names_create())
        return init_failed(out, "CPlayerNameDictionary::CreateInstance()");
    if (!guild_mgr_create())
        return init_failed(out, "CGuildMgr::CreateInstance()");
    if (!adventurer_agency_create())
        return init_failed(out, "CAdventurerAgencyMgr::CreateInstance()");
    if (!ui_request_join_party_mgr_create())
        return init_failed(out, "UIRequestJoinPartyMgr::CreateInstance()");
    map_sky_mgr_load(map_sky_mgr_instance());
    out->ok = 1;
    string_init(&out->what);
    string_assign_cstr(&out->what, "good");
    return out;
}
