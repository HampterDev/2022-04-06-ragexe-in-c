#include "session/csession.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "session/session_achievements.h"
#include "game/store_search_items.h"

#include <stdio.h>
#include <string.h>
#include <mmsystem.h>
#include "platform/clock.h"

DWORD session_get_server_time(struct Session *self)
{
    return platform_ticks_ms() + (0xffffffb8u - self->serverTimeOffset);
}

void session_init_at_startup(struct Session *self)
{
    struct PlatformDateTime st;
    DWORD base;
    int i;

    self->averagePing = 0;
    self->chatTarget = 0;
    self->showCellInfo = 1;
    self->ownAid = 0;
    self->effectOn = 1;
    platform_date_time_utc(&st);
    base = (((((DWORD)st.day * 24 + st.hour + 9) * 60 + st.minute) * 60 + st.second) * 1000);
    base += (DWORD)(BYTE)st.year * 0x7bbdb000u;
    base -= (DWORD)st.month * 0x605adc00u;
    base += st.milliseconds;
    base -= platform_ticks_ms();
    self->showGuildEmblems = 1;
    self->timeStampOffset = base;
    self->guildLeaveGid = 0;
    self->game_over = 0;
    self->pvpFirstPlaceTime = 0;
    self->countdownTicks = 0;
    self->showName = 1;
    self->timer_extra_flag = 0;
    self->timerShown = 0;
    for (i = 0; i < 5; i++)
        self->map_rule_flags[i] = 0;
    self->timerKind = 0;
    self->shortenSkillPage = 0;
    self->autoSaveChat = 0;
    self->mapType = 0;
    map_property_mgr_clear(g_mapFlags);
    own_states_clear();
    self->option_extra_flag = 0;
    self->petModified = 0;
    self->petJob = -1;
    self->petLevel = 1;
    self->petFullness = 100;
    self->petFullnessMax = 100;
    self->petIntimacy = 100;
    self->hatchedEggIndex = -1;
    self->petPrevFullness = 100;
    self->petAutoFeed = 0;
    self->pet_pending_index = -1;
    sprintf(self->petName, "");
    session_init_pet_emotions(self);

    sprintf(self->homunName, "");
    for (i = 0; i < 8; i++)
        self->homunStats[i] = 0;
    self->homunJob = -1;
    self->homunLevel = 1;
    self->homunHp = 0;
    self->homunMaxHp = 999999;
    self->homunSp = 0;
    self->homunMaxSp = 999999;
    self->homunExp = 0;
    self->homunMaxExp = 999999;
    self->homunFullness = 100;
    self->homunMaxFullness = 100;
    self->homunIntimacy = 100;
    self->homunRenamed = 0;
    self->homun_ai_flags[0] = 0;
    self->homun_ai_flags[1] = 0;
    self->hoUserAI = 0;
    for (i = 0; i < 3; i++)
        self->homun_ai_state[i] = 0;
    for (i = 0; i < 3; i++)
        self->homun_command[3 + i] = 0;
    for (i = 0; i < 3; i++)
        self->homun_command[i] = 0;
    self->mercJob = -1;
    sprintf(self->mercName, "용병");
    for (i = 0; i < 8; i++)
        self->mercStats[i] = 0;
    self->mercLevel = 1;
    self->mercHp = 0;
    self->mercSp = 0;
    self->mercMaxHp = 999999;
    self->mercMaxSp = 999999;
    for (i = 0; i < 5; i++)
        self->merc_contract_stats[i] = 0;
    self->mercOrder = 0;
    self->merc_command[4] = 0;
    self->autoMessageOn = 0;
    self->npcDialogTick = platform_coarse_ticks_ms();

    self->renameCharId = 0;
    sprintf(self->renameNewName, "");
    self->partyInviteRefused = 0;
    self->mdWndShown = 0;
    self->mdWndX = -1;
    self->mdWndY = -1;
    string_clear(&self->mdName);
    self->mdQueuePlace = 0;
    self->mdDestroyTime = 0;
    self->mdEnterLimit = 0;
    self->mdState = 0;
    self->equipOpenToOthers = 0;
    self->showStateInfo = 1;
    self->simpleSkillWnd = 0;
    self->showSkillDescript = 1;
    for (i = 0; i < 7; i++)
        self->charListCounts[i] = 0;
    chat_tabs_clear(&self->chatWndTabs);
    chat_tabs_clear(&self->subChatWnds);
    self->luaState = NULL;
    self->elementalAid = -1;
    session_load_prohibition_log(self);
    store_search_items_destroy(self->storeSearchResults.first, self->storeSearchResults.last);
    vec_clear(&self->storeSearchResults);
    self->storeSearchesLeft = 0;
    self->storeSearchOpenOnEnter = 0;
    self->storeSearchHasNextPage = 0;
    self->storeSearchItemLimit = 0;
    self->store_search_pos_x = -1;
    self->store_search_pos_y = -1;
    self->storeSearchClickedAid = -1;
    session_clear_transform_jobs(self, 1, 1, 1);
    self->itemDropLock = 0;
    for (i = 0; i < 11; i++)
        list_clear(&self->cashShopGoods[i], item_info_destroy);
    self->cartType = 0;
    self->showMonsterHp = 1;
    self->noPriceCheckWarning = 1;
    self->itemCompare = 1;
    self->senseTargetAid = 0;
    self->guildStorageOpen = 0;
    self->guildStorageCount = 0;
    self->guildStorageInUse = 0;
    self->hotkeyPage0 = 0;
    self->hotkeyPage1 = 0;
    for (i = 0; i < 5; i++)
        item_info_clear(&self->mailItems[i]);
    self->mailWeight = 0;
    self->pcCafeActive = 0;
    self->pcCafePoints = 0;
    self->pcCafeSecondsServed = 0;
    self->pcCafeGrade = 1;
    self->noSellWndSound = 1;
    tree_clear(&self->achievements, NULL);
    self->achTotalPoints = 0;
    self->achRank = 0;
    self->achCurrentPoints = 0;
    self->achNextPoints = 0;
    self->titles.last = self->titles.first;
    self->newTitles.last = self->newTitles.first;
    self->ownTitleId = 0;
    tracked_achievements_clear(&self->trackedAchievements);
    self->ownDressUp = 0;
    self->body = 0;
    self->lastPetEmotionTime = platform_ticks_ms();
    self->showCashShopIcon = 1;
    self->showGoldPcIcon = 1;
    self->screenShake = 1;
    self->skillUseKey = 0;
    self->lastKeyDown = 0;
    self->aid = -1;
    self->homunAid = -1;
    self->mercAid = -1;
    self->minimapButtons = 1;
    self->guildMemberSort = 1;
    self->blockPrivateTab = 0;
    self->favouritesSorted = 0;
    memset(self->itemSort, 0, sizeof self->itemSort);
    self->showMenuIcon = 1;
    self->homunAutoFeed = 0;
    self->homun_pending_index = -1;
    self->weightPenaltyPercent = 0x32;
    self->canBackToLogin = 0;
    self->rankingScheduleId = -1;
}
