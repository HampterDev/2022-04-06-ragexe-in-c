#include "actor/map_type_table.h"
#include "contents/account_settings.h"
#include "contents/adventurer_agency.h"
#include "contents/champion_monster.h"
#include "contents/character_settings.h"
#include "contents/guild_mgr.h"
#include "contents/player_names.h"
#include "contents/quest.h"
#include "contents/schedule.h"
#include "contents/town_info.h"
#include "contents/whisper_block.h"
#include "core/client_managers.h"
#include "core/map_sky.h"
#include "effects/effect_mgr.h"
#include "effects/effect_registry.h"
#include "effects/ez2str_effect_mgr.h"
#include "effects/particle.h"
#include "resources/state_icon_sheet.h"
#include "skills/skill_action_registry.h"
#include "ui/mail/rodex_box.h"
#include "ui/social/party_board.h"

/* The creates in the form a row calls them: no arguments, nonzero on success. The managers
   whose create already has that form are in the rows directly. */
static int load_ez2str_effects(void) { return ez2_str_effect_mgr_load(ez2str_effect_mgr_instance()); }
static int create_effect_mgr(void) { return effect_mgr_create(&g_effectTypes); }
static int create_particle_pools(void) { return effect_memory_create(); }
static int create_time_scheduler(void) { return time_scheduler_create(); }
/* town_info_create's result is not looked at; the row warns when the file does not load. */
static int create_town_info(void)
{
    town_info_create();
    return town_info_load();
}
static int create_icon_sheet(void) { return icon_sheet_create(); }
/* The quests load only after the create succeeded; the load has no result. */
static int create_renew_quests(void)
{
    if (!renew_quest_create())
        return 0;
    renew_quest_load();
    return 1;
}
/* champion_monster_create's result is not looked at; the row warns when the file does not
   load. */
static int create_champion_monsters(void)
{
    champion_monster_create();
    return champion_monster_load();
}
static int create_map_properties(void) { return map_property_mgr_create(); }
static int create_rodex(void) { return rodex_create_instance(); }
static int create_guild_mgr(void) { return guild_mgr_create(); }
static int create_join_party_mgr(void) { return ui_request_join_party_mgr_create(); }
static int load_map_sky(void) { return map_sky_mgr_load(map_sky_mgr_instance()); }

/* The composition point: every manager that startup creates and shutdown destroys. The list
   order is the create order; destroy_rank is the destroy order, which is not the reverse.
   manager_table.h says what each column and a missing row mean. */
static const struct ManagerRow k_client_manager_rows[] = {
    /* { name, create, destroy, destroy rank, on failure } */
    /* Created by client_init (skill_actions_register), before the others. */
    { "skill-action manager", NULL, skill_action_mgr_destroy_instance, 1,
      MANAGER_IGNORE },
    { "EZ2STR effect manager", load_ez2str_effects, ez2str_effect_mgr_destroy, 2,
      MANAGER_IGNORE },
    { "effect manager", create_effect_mgr, effect_mgr_destroy_instance, 17,
      MANAGER_STOP("CEffectMgr::CreateInstance()") },
    { "particle pools", create_particle_pools, effect_memory_destroy, 16,
      MANAGER_STOP("Particle pools") },
    { "time scheduler", create_time_scheduler, time_scheduler_destroy, 15,
      MANAGER_STOP("CTimeScheduler::CreateInstance()") },
    { "town info", create_town_info, town_info_destroy, 14,
      MANAGER_WARN("TownInfo file Init") },
    { "icon sheet", create_icon_sheet, icon_sheet_destroy, 13,
      MANAGER_STOP("CCooltimerMgr::CreateInstance()") },
    { "renewed quests", create_renew_quests, renew_quest_destroy, 12,
      MANAGER_STOP("CRenewQuestMgr::CreateInstance()") },
    { "champion monsters", create_champion_monsters, champion_monster_destroy, 11,
      MANAGER_WARN("ChampionMonster file Init") },
    { "map properties", create_map_properties, map_property_mgr_release, 10,
      MANAGER_STOP("CMapPropertyMgr::CreateInstance()") },
    { "Rodex", create_rodex, rodex_destroy_instance, 9,
      MANAGER_STOP("CRodexSystemMgr::CreateInstance()") },
    { "account data", account_user_data_create, account_user_data_destroy, 7,
      MANAGER_STOP("CAccountLinkedUserDataMgr::CreateInstance()") },
    { "character data", char_user_data_create, char_user_data_destroy, 8,
      MANAGER_STOP("CCharacterLinkedUserDataMgr::CreateInstance()") },
    { "whisper block", whisper_block_create, whisper_block_destroy, 6,
      MANAGER_STOP("CWhisperBlockMgr::CreateInstance()") },
    { "player names", player_names_create, player_names_destroy, 5,
      MANAGER_STOP("CPlayerNameDictionary::CreateInstance()") },
    { "guild manager", create_guild_mgr, guild_mgr_destroy, 4,
      MANAGER_STOP("CGuildMgr::CreateInstance()") },
    { "adventurer agency", adventurer_agency_create, adventurer_agency_destroy, 18,
      MANAGER_STOP("CAdventurerAgencyMgr::CreateInstance()") },
    { "join-party manager", create_join_party_mgr, ui_request_join_party_mgr_delete, 19,
      MANAGER_STOP("UIRequestJoinPartyMgr::CreateInstance()") },
    { "map sky", load_map_sky, map_sky_mgr_destroy, 3,
      MANAGER_IGNORE },
};

const struct ManagerTable g_clientManagers = MANAGER_TABLE(k_client_manager_rows);
