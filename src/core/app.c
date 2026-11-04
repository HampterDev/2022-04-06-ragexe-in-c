#include "contents/adventurer_agency.h"
#include "contents/character_settings.h"
#include "contents/account_settings.h"
#include "actor/map_type_table.h"
#include "contents/town_info.h"
#include "contents/champion_monster.h"
#include "core/map_sky.h"
#include "effects/effect_mgr.h"
#include "effects/ez2str_effect_mgr.h"
#include "effects/particle.h"
#include "contents/player_names.h"
#include "contents/schedule.h"
#include "contents/whisper_block.h"
#include "resources/state_icon_sheet.h"
#include "skills/skill_action_registry.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "platform/win_types.h"
#include <string.h>
#include "std/std.h"
#include "core/app.h"
#include "contents/guild_mgr.h"
#include "contents/quest.h"
#include "core/text_util.h"
#include "ui/mail/rodex_box.h"
#include "ui/social/party_board.h"

HINSTANCE g_hInstance;
int g_quitRequested;
char g_clientDir[0x88];
int g_fullscreenMode = 0;
int g_savedWidth, g_savedHeight;
int g_screenWidth = 1024;
int g_screenHeight = 768;

void app_destroy_singletons(void)
{
    skill_action_mgr_destroy_instance();

    ez2str_effect_mgr_destroy();

    map_sky_mgr_destroy();

    guild_mgr_destroy();
    player_names_destroy();
    whisper_block_destroy();
    account_user_data_destroy();
    char_user_data_destroy();
    rodex_destroy_instance();
    map_property_mgr_release();
    champion_monster_destroy();

    renew_quest_destroy();
    icon_sheet_destroy();
    town_info_destroy();
    time_scheduler_destroy();
    effect_memory_destroy();
    effect_mgr_destroy_instance();
    adventurer_agency_destroy();
    ui_request_join_party_mgr_delete();
}
