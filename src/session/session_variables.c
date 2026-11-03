#include "session/character_name.h"
#include "session/state.h"
#include "session/csession.h"
#include "session/session_state.h"
#include "session/session_windows.h"

#include <stdlib.h>
#include <string.h>
#include "game/chat_room.h"

void session_init_variables_(struct Session *self)
{
    MEMORYSTATUS memory;
    int value, r;

    self->decoy_sqrt50 = float_sqrt(50.0f);
    r = rand() % 1024;
    value = (r < 0 ? 0 : r) + 1;
    self->zeny = 0;
    self->baseExp = 0;
    self->jobExp = 0;
    self->maxWeight = 0;
    self->weight = 0;
    if (self->maxHp != value) {
        self->maxHp = value;
        session_refresh_basic_info(self);
    }
    if (self->hp != value) {
        self->hp = value;
        session_refresh_basic_info(self);
    }
    self->zeny = value;
    self->charBaseLevel = value;
    self->jobLevel = value;
    session_set_character_name(self, text_view_cstr("\xec\x9e\xa0\xec\x9d\xb4\xeb\x93\xa4\xec\x97\x88\xeb\x8d\x98\xea\xb1\xb0\xec\x95\xbc" /* "잠이들었던거야" */));
    self->statBonus[3] = 0;
    self->statBonus[4] = 0;
    self->statBonus[5] = 0;
    self->statBonus[0] = 0;
    self->statBonus[1] = 0;
    self->statBonus[2] = 0;
    self->statCosts[0] = 2;
    self->statCosts[1] = 2;
    self->statCosts[5] = 2;
    self->statCosts[2] = 2;
    self->statCosts[3] = 2;
    self->statCosts[4] = 2;
    self->statusPoints = 0;
    self->atk = 0;
    self->refineAtk = 0;
    self->matkMax = 0;
    self->matkMin = 0;
    self->def = 0;
    self->plusDef = 0;
    self->mdef = 0;
    self->plusMdef = 0;
    self->hit = 0;
    self->flee = 0;
    self->plusFlee = 0;
    self->critical = 0;
    self->aspd = 0;
    self->plusAspd = 0;
    self->baseExp = 0;
    self->jobExp = 0;
    self->nextBaseExp = 999999;
    self->nextJobExp = 999999;
    self->cartCount = 0;
    self->cartCountMax = 0;
    self->cartWeight = 0;
    self->cartWeightMax = 0;
    self->vendingSlots = 0xd;
    self->buyingStoreSlots = 5;
    self->battlegroundCamp = 0;
    self->battlegroundPoints[0] = -1;
    self->battlegroundPoints[1] = -1;
    self->font = 0;
    self->attackRange = 0x14;
    self->shortcutCharSlot = 0;
    self->wornArrowIndex = 0;
    self->partyExpOption = 0;
    self->partyItemPickupRule = 0;
    self->partyItemDivisionRule = 0;
    self->itemSnap = 0;
    self->monsterSnapInEffect = 1;
    self->monsterSnapOnSkill = 1;
    self->monsterSnapOnNoSkill = 0;
    self->makeMissEffect = 1;
    self->noShift = 0;
    self->quickSpellRightClick = 0;
    self->quickSpellWheel = 0;
    self->battle_mode = 0;
    self->battleChat = 0;
    self->auraFull = 1;
    self->auraOff = 0;
    self->noCtrl = 0;
    self->showSkillFail = 1;
    self->showLoginNotices = 1;
    self->shopOneClick = 0;
    self->hideChat = 0;
    self->hideChatAndGuild = 0;
    self->refuseTrades = 0;
    self->battle_mode_suspended = 0;
    self->battle_extra_flag = 0;
    self->windowSnap = 0;
    g_session1.wheelConsumed = 0;
    self->progress_bar_extra = 0;
    self->showName = 1;
    self->loadingScreenRandom = 1;
    self->married = 0;
    self->ownIllusion = 0;
    self->showTimeFont = 0;
    memset(&memory, 0, sizeof(memory));
    memory.dwLength = sizeof(memory);
    GlobalMemoryStatus(&memory);
    self->lowMemory = memory.dwTotalPhys < 0x9b72ec0;
    memset(self->blockedChars, 0, sizeof self->blockedChars);
    self->cartType = 0;
}
