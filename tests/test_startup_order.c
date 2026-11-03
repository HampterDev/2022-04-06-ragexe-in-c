/* Characterization of the order in which the client creates and destroys its managers, before
   startup and shutdown become one table.

   app_init_managers (core/startup.c) and app_destroy_singletons (core/app.c) are extracted
   (extract_startup_order.py) and run against stubs that record their names. Recorded:
   - app_init_managers when every step succeeds;
   - app_init_managers when each checked create fails in turn: the steps up to the failing
     one, and the failure text it returns (nothing created before it is destroyed);
   - app_init_managers when the two loads that only warn (town info, champion monsters) fail;
   - app_destroy_singletons.
   The expected sequences below are the current order; a change to either function that moves,
   adds or drops a step fails the test. */
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/app.h"
#include "std/string.h"
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
#include "core/map_sky.h"
#include "effects/effect_mgr.h"
#include "effects/effect_registry.h"
#include "effects/ez2str_effect_mgr.h"
#include "effects/particle.h"
#include "platform/win_utf8.h"
#include "resources/state_icon_sheet.h"
#include "skills/skill_action_registry.h"
#include "ui/mail/rodex_box.h"
#include "ui/social/party_board.h"

#define MAX_STEPS 64

static const char *steps[MAX_STEPS];
static int step_count;
static const char *failing;

static void step(const char *name)
{
    assert(step_count < MAX_STEPS);
    steps[step_count++] = name;
}

/* A step that can fail: it fails when the scenario names it. */
static int result(const char *name)
{
    step(name);
    return failing == NULL || strcmp(failing, name) != 0;
}

void *mem_alloc(size_t bytes)
{
    void *p = malloc(bytes ? bytes : 1);
    assert(p);
    return p;
}
void mem_free(void *p) { free(p); }
__declspec(noreturn) void fatal_error(const char *message)
{
    printf("fatal_error: %s\n", message);
    exit(1);
}

const struct EffectTypeSources g_effectTypes;
static struct Ez2StrEffectMgr *const ez2str = (struct Ez2StrEffectMgr *)(uintptr_t)0x1000;
static struct MapSkyMgr *const map_sky = (struct MapSkyMgr *)(uintptr_t)0x2000;

struct Ez2StrEffectMgr *ez2str_effect_mgr_instance(void) { step("ez2str_effect_mgr_instance"); return ez2str; }
unsigned char ez2_str_effect_mgr_load(struct Ez2StrEffectMgr *self)
{
    assert(self == ez2str);
    return (unsigned char)result("ez2_str_effect_mgr_load");
}
unsigned char effect_mgr_create(const struct EffectTypeSources *types)
{
    assert(types == &g_effectTypes);
    return (unsigned char)result("effect_mgr_create");
}
unsigned char effect_memory_create(void) { return (unsigned char)result("effect_memory_create"); }
char time_scheduler_create(void) { return (char)result("time_scheduler_create"); }
char town_info_create(void) { return (char)result("town_info_create"); }
char town_info_load(void) { return (char)result("town_info_load"); }
unsigned char icon_sheet_create(void) { return (unsigned char)result("icon_sheet_create"); }
char renew_quest_create(void) { return (char)result("renew_quest_create"); }
void renew_quest_load(void) { step("renew_quest_load"); }
char champion_monster_create(void) { return (char)result("champion_monster_create"); }
char champion_monster_load(void) { return (char)result("champion_monster_load"); }
unsigned char map_property_mgr_create(void) { return (unsigned char)result("map_property_mgr_create"); }
unsigned char rodex_create_instance(void) { return (unsigned char)result("rodex_create_instance"); }
int account_user_data_create(void) { return result("account_user_data_create"); }
int char_user_data_create(void) { return result("char_user_data_create"); }
int whisper_block_create(void) { return result("whisper_block_create"); }
int player_names_create(void) { return result("player_names_create"); }
char guild_mgr_create(void) { return (char)result("guild_mgr_create"); }
int adventurer_agency_create(void) { return result("adventurer_agency_create"); }
char ui_request_join_party_mgr_create(void) { return (char)result("ui_request_join_party_mgr_create"); }
struct MapSkyMgr *map_sky_mgr_instance(void) { step("map_sky_mgr_instance"); return map_sky; }
unsigned char map_sky_mgr_load(struct MapSkyMgr *self)
{
    assert(self == map_sky);
    return (unsigned char)result("map_sky_mgr_load");
}
int WINAPI win_utf8_message_box(HWND owner, LPCSTR text, LPCSTR caption, UINT type)
{
    assert(owner == NULL && !strcmp(caption, "Error") && type == 0);
    if (!strcmp(text, "TownInfo file Init"))
        step("message box: TownInfo file Init");
    else if (!strcmp(text, "ChampionMonster file Init"))
        step("message box: ChampionMonster file Init");
    else
        assert(!"an unexpected message box");
    return 0;
}

void skill_action_mgr_destroy_instance(void) { step("skill_action_mgr_destroy_instance"); }
void ez2str_effect_mgr_destroy(void) { step("ez2str_effect_mgr_destroy"); }
void map_sky_mgr_destroy(void) { step("map_sky_mgr_destroy"); }
void guild_mgr_destroy(void) { step("guild_mgr_destroy"); }
void player_names_destroy(void) { step("player_names_destroy"); }
void whisper_block_destroy(void) { step("whisper_block_destroy"); }
void account_user_data_destroy(void) { step("account_user_data_destroy"); }
void char_user_data_destroy(void) { step("char_user_data_destroy"); }
void rodex_destroy_instance(void) { step("rodex_destroy_instance"); }
void map_property_mgr_release(void) { step("map_property_mgr_release"); }
void champion_monster_destroy(void) { step("champion_monster_destroy"); }
void renew_quest_destroy(void) { step("renew_quest_destroy"); }
void icon_sheet_destroy(void) { step("icon_sheet_destroy"); }
void town_info_destroy(void) { step("town_info_destroy"); }
void time_scheduler_destroy(void) { step("time_scheduler_destroy"); }
void effect_memory_destroy(void) { step("effect_memory_destroy"); }
void effect_mgr_destroy_instance(void) { step("effect_mgr_destroy_instance"); }
void adventurer_agency_destroy(void) { step("adventurer_agency_destroy"); }
void ui_request_join_party_mgr_delete(void) { step("ui_request_join_party_mgr_delete"); }

#include "startup_order.inc"

/* app_init_managers when every step succeeds. */
static const char *const create_order[] = {
    "ez2str_effect_mgr_instance", "ez2_str_effect_mgr_load",
    "effect_mgr_create",
    "effect_memory_create",
    "time_scheduler_create",
    "town_info_create", "town_info_load",
    "icon_sheet_create",
    "renew_quest_create", "renew_quest_load",
    "champion_monster_create", "champion_monster_load",
    "map_property_mgr_create",
    "rodex_create_instance",
    "account_user_data_create",
    "char_user_data_create",
    "whisper_block_create",
    "player_names_create",
    "guild_mgr_create",
    "adventurer_agency_create",
    "ui_request_join_party_mgr_create",
    "map_sky_mgr_instance", "map_sky_mgr_load",
};

/* The creates whose failure stops app_init_managers, and the text it returns. The other steps'
   results are ignored, or (the two loads) only shown in a message box. */
static const struct { const char *create, *what; } checked[] = {
    { "effect_mgr_create", "CEffectMgr::CreateInstance()" },
    { "effect_memory_create", "Particle pools" },
    { "time_scheduler_create", "CTimeScheduler::CreateInstance()" },
    { "icon_sheet_create", "CCooltimerMgr::CreateInstance()" },
    { "renew_quest_create", "CRenewQuestMgr::CreateInstance()" },
    { "map_property_mgr_create", "CMapPropertyMgr::CreateInstance()" },
    { "rodex_create_instance", "CRodexSystemMgr::CreateInstance()" },
    { "account_user_data_create", "CAccountLinkedUserDataMgr::CreateInstance()" },
    { "char_user_data_create", "CCharacterLinkedUserDataMgr::CreateInstance()" },
    { "whisper_block_create", "CWhisperBlockMgr::CreateInstance()" },
    { "player_names_create", "CPlayerNameDictionary::CreateInstance()" },
    { "guild_mgr_create", "CGuildMgr::CreateInstance()" },
    { "adventurer_agency_create", "CAdventurerAgencyMgr::CreateInstance()" },
    { "ui_request_join_party_mgr_create", "UIRequestJoinPartyMgr::CreateInstance()" },
};
static const char *const ignored[] = {
    "ez2_str_effect_mgr_load", "town_info_create", "champion_monster_create", "map_sky_mgr_load",
};

/* app_destroy_singletons. Not the reverse of create_order: the skill-action manager is
   destroyed here but created by client_init (skill_actions_register); the EZ2STR effect
   manager, set up first, goes first; account data goes before character data; and the
   adventurer agency and the join-party manager, created last, go last. */
static const char *const destroy_order[] = {
    "skill_action_mgr_destroy_instance",
    "ez2str_effect_mgr_destroy",
    "map_sky_mgr_destroy",
    "guild_mgr_destroy",
    "player_names_destroy",
    "whisper_block_destroy",
    "account_user_data_destroy",
    "char_user_data_destroy",
    "rodex_destroy_instance",
    "map_property_mgr_release",
    "champion_monster_destroy",
    "renew_quest_destroy",
    "icon_sheet_destroy",
    "town_info_destroy",
    "time_scheduler_destroy",
    "effect_memory_destroy",
    "effect_mgr_destroy_instance",
    "adventurer_agency_destroy",
    "ui_request_join_party_mgr_delete",
};

#define COUNT(a) (int)(sizeof(a) / sizeof((a)[0]))

static void expect_steps(const char *const *expected, int count, const char *what)
{
    int i;

    if (step_count != count) {
        printf("%s: %d steps, expected %d\n", what, step_count, count);
        exit(1);
    }
    for (i = 0; i < count; i++)
        if (strcmp(steps[i], expected[i])) {
            printf("%s: step %d is %s, expected %s\n", what, i, steps[i], expected[i]);
            exit(1);
        }
}

static void run_init(const char *fail, struct InitResult *init)
{
    step_count = 0;
    failing = fail;
    assert(app_init_managers(init) == init);
}

int main(void)
{
    struct InitResult init;
    const char *with_warnings[COUNT(create_order) + 2];
    int i, j, n;

    /* Every step succeeds. */
    run_init(NULL, &init);
    expect_steps(create_order, COUNT(create_order), "all succeed");
    assert(init.ok == 1 && !strcmp(string_cstr(&init.what), "good"));
    string_free(&init.what);

    /* A checked create fails: the steps before it, then the failure; nothing is undone. */
    for (i = 0; i < COUNT(checked); i++) {
        run_init(checked[i].create, &init);
        for (n = 0; n < COUNT(create_order) && strcmp(create_order[n], checked[i].create); n++)
            ;
        assert(n < COUNT(create_order));
        expect_steps(create_order, n + 1, checked[i].create);
        assert(init.ok == 0 && !strcmp(string_cstr(&init.what), checked[i].what));
        string_free(&init.what);
    }

    /* A step whose result is ignored fails: the sequence is unchanged. */
    for (i = 0; i < COUNT(ignored); i++) {
        run_init(ignored[i], &init);
        expect_steps(create_order, COUNT(create_order), ignored[i]);
        assert(init.ok == 1);
        string_free(&init.what);
    }

    /* A load that only warns fails: its message box follows it. */
    for (i = 0; i < 2; i++) {
        const char *load = i == 0 ? "town_info_load" : "champion_monster_load";
        const char *box = i == 0 ? "message box: TownInfo file Init" : "message box: ChampionMonster file Init";

        for (j = n = 0; j < COUNT(create_order); j++) {
            with_warnings[n++] = create_order[j];
            if (!strcmp(create_order[j], load))
                with_warnings[n++] = box;
        }
        run_init(load, &init);
        expect_steps(with_warnings, n, load);
        assert(init.ok == 1);
        string_free(&init.what);
    }

    /* Shutdown. */
    step_count = 0;
    failing = NULL;
    app_destroy_singletons();
    expect_steps(destroy_order, COUNT(destroy_order), "destroy");

    printf("Startup order: %d startup steps (%d checked creates, %d ignored results, 2 warning loads), "
           "%d shutdown steps; passed.\n", COUNT(create_order), COUNT(checked), COUNT(ignored), COUNT(destroy_order));
    return 0;
}
