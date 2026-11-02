#include "contents/client_lua.h"
#include <stdlib.h>
#include "platform/embedded_fonts.h"
#include "render/display_devices.h"
#include "session/state.h"
#include "core/memory.h"
#include "session/csession.h"

#include "session/session_items.h"
#include "session/session_state.h"

#include <string.h>
#include "std/std.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "std/vec.h"
#include "game/store_search_items.h"
#include "game/guild_storage.h"
#include "game/party_member.h"
#include "session/session_achievements.h"
#include "platform/window.h"

struct Session g_session1;

static void session_inventory_lists(struct Session *self, struct List *lists[12])
{
    lists[0] = &self->itemList;
    lists[1] = &self->shopItems;
    lists[2] = &self->purchaseItems;
    lists[3] = &self->sellItems;
    lists[4] = &self->storageItems;
    lists[5] = &self->cartItems;
    lists[6] = &self->otherStoreItems;
    lists[7] = &self->storeCandidateItems;
    lists[8] = &self->ownStoreItems;
    lists[9] = &self->storePurchaseItems;
    lists[10] = &self->storeSetupItems;
    lists[11] = &self->storePurchaseSent;
}

static void destroy_party_member(void *value)
{
    party_member_info_destroy(value);
}

struct Session *session_construct(struct Session *self)
{
    int i;

    list_init(&self->guildSkills);
    player_skill_bundle_construct(&self->skills);
    list_init(&self->homunSkills);
    list_init(&self->merSkills);
    self->cooldownSlotsBar = (struct CooldownSlotList){0};
    self->cooldownSlotsQuick = (struct CooldownSlotList){0};
    for (i = 0; i < 0x24; i++)
        string_init(&self->shortenSkillNames[i]);
    tree_init(&self->skillHitEffectIds);
    tree_init(&self->skillEffectIds);
    vec_init(&self->guildSkillUseLevels);
    vec_init(&self->homunSkillUseLevels);
    vec_init(&self->merSkillUseLevels);
    list_init(&self->plateRecords);
    self->chatWndTabs = (struct ChatTabs){0};
    self->subChatWnds = (struct ChatTabs){0};
    vec_init(&self->prohibitionLog);
    string_init(&self->worldName);
    vec_init(&self->comboDelays);
    tree_init(&self->abrHp);
    tree_init(&self->abrMaxHp);
    vec_init(&self->maleHairNames);
    vec_init(&self->femaleHairNames);
    vec_init(&self->jobImfNames);
    vec_init(&self->sexSuffixes);
    vec_init(&self->jobSpritePaths);
    vec_init(&self->reqJobNames);
    tree_init(&self->jobNamesSex0);
    tree_init(&self->jobNamesSex99);
    vec_init(&self->sexLetters);
    vec_init(&self->jobIdNames);
    list_init(&self->pingSamples);
    vec_init(&self->jobBodyNames);
    text_array_init(&self->chatMacros);
    for (i = 0; i < 0x19; i++)
        string_init(&self->pet_state_texts[i]);
    xml_element_construct(&self->monsterSkillTable.root, "XMLDocument");
    self->monsterSkillTable.copy = NULL;
    {
        struct List *lists[12];
        session_inventory_lists(self, lists);
        for (i = 0; i < 12; i++)
            list_init(lists[i]);
    }
    vec_init(&self->buyingStoreItems);
    for (i = 0; i < 11; i++)
        list_init(&self->cashShopGoods[i]);
    list_init(&self->partyMembers);
    list_init(&self->friendList);
    for (i = 0; i < 10; i++)
        item_info_construct(&self->equipSlots[i]);
    for (i = 0; i < 10; i++)
        item_info_construct(&self->viewedEquips[i]);
    for (i = 0; i < 10; i++)
        item_info_construct(&self->costumeSlots[i]);
    for (i = 0; i < 10; i++)
        item_info_construct(&self->viewedCostumes[i]);
    for (i = 0; i < 10; i++)
        item_info_construct(&self->exchangeA[i]);
    for (i = 0; i < 10; i++)
        item_info_construct(&self->partnerTradeItems[i]);
    vec_init(&self->talkTypes);
    vec_init(&self->mp3Names);
    vec_init(&self->fogParameters);
    vec_init(&self->viewPoints);
    vec_init(&self->indoorRsws);
    tree_init(&self->shadowFactors);
    self->jobHitSounds = (struct JobHitSounds){0};
    vec_init(&self->weaponHitWaves);
    vec_init(&self->mapObjLights);
    vec_init(&self->jobSpriteNames);
    vec_init(&self->sexSpriteNames);
    vec_init(&self->weaponSpriteNames);
    vec_init(&self->shieldSpriteNames);
    vec_init(&self->exceptionMinimapNames);
    self->talkOptionSettings = (struct TalkOptionSettings){0};
    vec_init(&self->commandListCommands);
    vec_init(&self->windowNames);
    vec_init(&self->altKeyNames);
    tree_init(&self->mapPosRects);
    tree_init(&self->mapPosGroupBoxes);
    self->tableReader = (struct TableReader){0};
    self->skillDelays = (struct SkillCooldownList){0};
    string_init(&self->newPartyName);
    string_init(&self->partyName);
    string_init(&self->shopTitle);
    string_init(&self->serverName);
    string_init(&self->autoMessage);
    string_init(&self->mdName);
    self->channelCopId = 0;
    party_booking_construct(&self->partyBooking);
    vec_init(&self->storeSearchResults);
    tree_init(&self->memberPings);
    vec_init(&self->guildStorageLog);
    vec_init(&self->guildStorageLogPut);
    vec_init(&self->guildStorageLogTaken);
    for (i = 0; i < 5; i++)
        item_info_construct(&self->mailItems[i]);
    tree_init(&self->achievements);
    vec_init(&self->titles);
    vec_init(&self->newTitles);
    vec_init(&self->trackedAchievements);
    vec_init(&self->doramHairNamesMale);
    vec_init(&self->doramHairNamesFemale);
    self->cameraInfoShown = 0;
    for (i = 0; i < 10; i++)
        item_info_construct(self->switchEquips + i);
    for (i = 0; i < 10; i++)
        item_info_construct(self->switchCostumes + i);
    tree_init(&self->transparentItemPulses);
    self->zoomFarLimit = 0;
    self->emblemFrame = 0;
    self->extendDamage = 1;
    tree_init(&self->pendingNameAids);
    self->gameModeScheduleId = 0;
    self->lastPacketTime = 0;
    self->lastSendTime = 0;
    tree_init(&self->instantVars);
    tree_init(&self->statusValues);
    mcstore_init(&self->mcStoreBackup);
    vec_init(&self->pcNames);
    self->hideOtherPlayers = self->userConfigEntry1 = 0;
    vec_init(&self->accountCharNames);

    if (!user_hotkey_mgr_create())
        platform_message_box(NULL, "CUserHotkeyMgr::CreateInstance()", "Error", MB_OK);
    if (!barter_market_create())
        platform_message_box(NULL, "InstanceCBarterMarketItemMgr::CreateInstance()", "Error", MB_OK);
    if (!expanded_barter_create())
        platform_message_box(NULL, "InstanceCBarterMarketItem2Mgr::CreateInstance()", "Error", MB_OK);
    if (!pet_auto_feed_create())
        platform_message_box(NULL, "Pet auto-feeding data initialization", "Error", MB_OK);
    if (!achievement_create())
        platform_message_box(NULL, "CAchievementMgr file init", "Error", MB_OK);
    self->new_layer_table_loaded = 0;
    self->loadingAirplane = 0;
    self->call_allowed = 0;
    self->equipSwapOpen = 0;
    self->doramMakeable = 1;
    self->equipSwapSentAt = 0;
    private_airplane_create();
    check_attendance_create();
    self->showSpiritMarbles = 1;
    self->viewedBody = 0;
    self->inventoryExtension = 0;
    map_info_create();
    self->madogear = 0;
    self->baseLevel4th = 0;
    self->jobLevel4th = 0;
    tree_clear(&self->instantVars, NULL);
    self->saleListSentAt = 0;
    return self;
}

void session_destruct(struct Session *self)
{
    int i, count;
    struct Tree *maps;

    user_hotkey_mgr_release();
    barter_market_destroy();
    expanded_barter_destroy();
    self->prohibitionLog.last = self->prohibitionLog.first;
    if (self->luaState) {
        client_lua_destruct(self->luaState);
        free(self->luaState);
        self->luaState = NULL;
    }

    count = (int)self->mapPosGroupBoxes.size;
    for (i = 0; i < count; i++)
        tree_clear(&self->rectMaps[i], named_map_rect_destroy);
    maps = self->rectMaps;
    if (maps) {
        for (i = self->rectMapCount; i-- > 0;)
            tree_free(&maps[i], named_map_rect_destroy);
        mem_free(maps);
    }

    embedded_fonts_clear();
    item_info_mgr_clear();
    pet_auto_feed_destroy();
    achievement_destroy();
    display_devices_reset();
    tree_clear(&self->transparentItemPulses, NULL);
    memset(&self->baseLevel, 0, 0x7938 - 0x7900);
    private_airplane_destroy();
    check_attendance_destroy();
    self->auras[15] = 0;
    self->auras[16] = 0;

    string_vec_free(&self->accountCharNames);
    vec_free(&self->pcNames);
    mcstore_destroy(&self->mcStoreBackup);
    tree_free(&self->statusValues, NULL);
    tree_free(&self->instantVars, NULL);
    tree_free(&self->pendingNameAids, NULL);
    tree_free(&self->transparentItemPulses, NULL);
    for (i = 10; i-- > 0;)
        item_info_destruct(self->switchCostumes + i);
    for (i = 10; i-- > 0;)
        item_info_destruct(self->switchEquips + i);
    vec_free(&self->doramHairNamesFemale);
    vec_free(&self->doramHairNamesMale);
    tracked_achievements_free(&self->trackedAchievements);
    vec_free(&self->newTitles);
    vec_free(&self->titles);
    tree_free(&self->achievements, NULL);
    for (i = 5; i-- > 0;)
        item_info_destruct(&self->mailItems[i]);
    guild_log_free(&self->guildStorageLogTaken);
    guild_log_free(&self->guildStorageLogPut);
    guild_log_free(&self->guildStorageLog);
    tree_free(&self->memberPings, NULL);
    store_search_items_destroy(self->storeSearchResults.first, self->storeSearchResults.last);
    vec_free(&self->storeSearchResults);
    party_booking_destruct(&self->partyBooking);
    string_free(&self->mdName);
    string_free(&self->autoMessage);
    string_free(&self->serverName);
    string_free(&self->shopTitle);
    string_free(&self->partyName);
    string_free(&self->newPartyName);
    skill_cooldowns_clear(&self->skillDelays);
    table_reader_release(&self->tableReader);
    tree_free(&self->mapPosGroupBoxes, NULL);
    tree_free(&self->mapPosRects, named_map_rect_destroy);
    vec_free(&self->altKeyNames);
    vec_free(&self->windowNames);
    vec_free(&self->commandListCommands);
    talk_option_settings_clear(&self->talkOptionSettings);
    string_vec_free(&self->exceptionMinimapNames);
    vec_free(&self->shieldSpriteNames);
    vec_free(&self->weaponSpriteNames);
    vec_free(&self->sexSpriteNames);
    vec_free(&self->jobSpriteNames);
    vec_free(&self->mapObjLights);
    string_vec_free(&self->weaponHitWaves);
    self->jobHitSounds = (struct JobHitSounds){0};
    tree_free(&self->shadowFactors, NULL);
    vec_free(&self->indoorRsws);
    named_strings_free(&self->viewPoints);
    named_strings_free(&self->fogParameters);
    vec_free(&self->mp3Names);
    vec_free(&self->talkTypes);
    for (i = 10; i-- > 0;)
        item_info_destruct(&self->partnerTradeItems[i]);
    for (i = 10; i-- > 0;)
        item_info_destruct(&self->exchangeA[i]);
    for (i = 10; i-- > 0;)
        item_info_destruct(&self->viewedCostumes[i]);
    for (i = 10; i-- > 0;)
        item_info_destruct(&self->costumeSlots[i]);
    for (i = 10; i-- > 0;)
        item_info_destruct(&self->viewedEquips[i]);
    for (i = 10; i-- > 0;)
        item_info_destruct(&self->equipSlots[i]);
    list_free(&self->friendList, destroy_party_member);
    list_free(&self->partyMembers, destroy_party_member);
    for (i = 11; i-- > 0;)
        list_free(&self->cashShopGoods[i], item_info_destroy);
    vec_free(&self->buyingStoreItems);
    {
        struct List *lists[12];
        session_inventory_lists(self, lists);
        for (i = 12; i-- > 0;)
            list_free(lists[i], item_info_destroy);
    }
    xml_document_release(&self->monsterSkillTable);
    xml_element_destruct(&self->monsterSkillTable.root);
    for (i = 0x19; i-- > 0;)
        string_free(&self->pet_state_texts[i]);
    text_array_clear(&self->chatMacros);
    vec_free(&self->jobBodyNames);
    list_free(&self->pingSamples, NULL);
    vec_free(&self->jobIdNames);
    vec_free(&self->sexLetters);
    tree_free(&self->jobNamesSex99, NULL);
    tree_free(&self->jobNamesSex0, NULL);
    vec_free(&self->reqJobNames);
    vec_free(&self->jobSpritePaths);
    vec_free(&self->sexSuffixes);
    vec_free(&self->jobImfNames);
    vec_free(&self->femaleHairNames);
    vec_free(&self->maleHairNames);
    tree_free(&self->abrMaxHp, NULL);
    tree_free(&self->abrHp, NULL);
    vec_free(&self->comboDelays);
    string_free(&self->worldName);
    vec_free(&self->prohibitionLog);
    chat_tabs_clear(&self->subChatWnds);
    chat_tabs_clear(&self->chatWndTabs);
    list_free(&self->plateRecords, NULL);
    vec_free(&self->merSkillUseLevels);
    vec_free(&self->homunSkillUseLevels);
    vec_free(&self->guildSkillUseLevels);
    tree_free(&self->skillEffectIds, NULL);
    tree_free(&self->skillHitEffectIds, NULL);
    for (i = 0x24; i-- > 0;)
        string_free(&self->shortenSkillNames[i]);
    cooldown_slots_clear(&self->cooldownSlotsQuick);
    cooldown_slots_clear(&self->cooldownSlotsBar);
    list_free(&self->merSkills, skill_info_destroy);
    list_free(&self->homunSkills, skill_info_destroy);
    player_skill_bundle_destruct(&self->skills);
    list_free(&self->guildSkills, skill_info_destroy);
}
