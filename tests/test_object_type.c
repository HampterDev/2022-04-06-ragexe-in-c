#include "type_query_fixture.h"
#include <windows.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void *__RTDynamicCast(void *object, long delta, const void *source,
                     const void *target, int is_reference);

static int old_class_is(const char *name, const char *expected)
{
    while (*expected && *name == *expected) { ++name; ++expected; }
    return !*expected && (*name == '@' || !*name);
}

int main(void)
{
    SYSTEM_INFO system;
    unsigned char *pages;
    DWORD old_protection;
    void *guarded;
    size_t i, kind;
    unsigned comparisons = 0, successes = 0, named = 0, windows = 0, retired = 0;
    static const char *const policy_names[] = {
        "UIShortCutWnd", "UIQuikSlotWnd", "UIMinimapZoomWnd", "UINewChatWnd",
        "UIChatEditCtrl", "UIBitmapButton", "UIMerchantShopTitle",
        "UIPcGage", "UIPlayerGage", "UIMonsterGage", "UIChatRoomTitle",
        "UIBalloonText", "UINameBalloonText",
        "UICharInfoBalloonText", "UIColorTransBalloonText", "UIAchBalloonText"
    };
    const void *unknown = (const void *)(uintptr_t)1;
    assert(sizeof(void *) == 4);
    assert(offsetof(struct ReferenceVtable, slots) == sizeof(void *));
    GetSystemInfo(&system);
    pages = VirtualAlloc(NULL, system.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages + system.dwPageSize, system.dwPageSize, PAGE_NOACCESS, &old_protection));
    guarded = pages + system.dwPageSize - sizeof(void *);
    for (i = 0; i < REFERENCE_VTABLE_COUNT; ++i) {
        const void *table = reference_vtables[i].slots;
        const uintptr_t *locator = reference_vtables[i].locator;
        const char *previous_name = (const char *)locator[3] + 8 + 4;
        const char *name = object_type_name(table);
        int is_window = !!(reference_valid_sources[i] & (UINT32_C(1) << OBJECT_FRAME_WINDOW));
        int has_name = is_window;
        if (i == 479 || i == 333 || i == 589 || i == 590 || i == 220 || i == 354 || i == 244 || i == 229 || i == 289 || i == 455 || i == 31 || i == 468 || (i >= 294 && i <= 302) || (i >= 356 && i <= 360) || i == 511 || i == 23 || i == 140 || i == 269 || i == 270 || i == 271 || i == 596 || i == 597 || i == 467 || i == 521 || i == 249 || i == 250 || i == 496 || i == 226 || i == 524 || i == 219 || i == 252 || i == 588 || i == 551 || i == 599 || i == 507 || i == 508 || i == 509 || i == 510 || i == 540 || (i >= 303 && i <= 307) || i == 378 || i == 472 || (i >= 535 && i <= 537) || i == 224 || i == 225 || (i >= 402 && i <= 404) || i == 533 || i == 534 || i == 566 || i == 178 || i == 228 || (i >= 230 && i <= 232) || i == 428 || i == 609 || i == 610 || i == 529 ||
                i == 346 || (i >= 384 && i <= 389) || (i >= 318 && i <= 321) || i == 573 || i == 574 || i == 204 ||
                i == 73 || i == 172 || i == 209 || i == 210 || i == 214 || i == 216 || i == 222 || i == 238 ||
                i == 264 || i == 268 || i == 288 || i == 345 || i == 391 || i == 405 || i == 408 || i == 426 ||
                i == 429 || i == 434 || i == 435 || i == 444 || i == 447 || i == 457 || i == 490 || i == 494 ||
                i == 503 || i == 514 || i == 528 || i == 545 || i == 552 || i == 560 || i == 561 || i == 563 ||
                i == 567 || i == 572 || i == 575 || i == 576 || i == 579 || i == 580 || i == 581 || i == 586 ||
                i == 592 || i == 594 || i == 595 || i == 608 || i == 400 || i == 383) {
            static const struct { size_t index; const char *name; } retired_types[] = {
                {479, "UIPetEvolutionWnd@@"},
                {333, "UIGuildTipWnd@@"},
                {589, "UITipOfTheDayWnd@@"},
                {590, "UITipboxWnd@@"},
                {220, "UIBookWnd@@"},
                {354, "UIItemParamChangeDisplayWnd@@"},
                {244, "UICheckAttendanceWnd_Banner@@"},
                {229, "UICashShopWnd_Banner@@"},
                {289, "UIEditctrlWnd@@"},
                {455, "UINoticeConfirmWnd@@"},
                {31, "CChoicePackageItemBoxWnd@@"}, {468, "UIPackageBoxNameList@@"},
                {294, "UIEnchantWnd@@"},
                {295, "UIEnchant_Button@@"},
                {296, "UIEnchant_Group@@"},
                {297, "UIEnchant_Info@@"},
                {298, "UIEnchant_Item@@"},
                {299, "UIEnchant_List@@"},
                {300, "UIEnchant_ScrollBar@@"},
                {301, "UIEnchant_Tab@@"},
                {302, "UIEnchant_TargetList@@"},
                {356, "UIItemReformWnd@@"}, {357, "UIItemReformWnd_DetailBtn@@"},
                {358, "UIItemReformWnd_DetailMsg@@"}, {359, "UIItemReformWnd_Item@@"},
                {360, "UIItemReformWnd_List@@"},
                {511, "UIReputeWnd@@"},
                {23, "CBmpObjWnd@@"}, {140, "CSignBoardWnd@@"},
                {269, "UIConnectSnsWnd@@"}, {270, "UIConnectTwitterWnd@@"}, {271, "UIControlRectWnd@@"},
                {596, "UITwitterEditCtrl3@@"}, {597, "UITwitterScrollBar@@"},
                {467, "UIOpenRoulletteWnd@@"}, {521, "UIRoulletteWnd@@"},
                {249, "UIClanInfoManageWnd@@"}, {250, "UIClanWnd@@"},
                {496, "UIRecommededQuestScrollWnd@@"},
                {226, "UICaptchaRegisterWnd@@"},
                {524, "UIScroll@TagTextViewer@@"}, {219, "UIBmpTextButton2@@"},
                {252, "UIColorButton@@"}, {588, "UITextureButton@@"},
                {551, "UISkipButton@@"}, {599, "UIVideoWnd@@"},
                {507, "UIReplayButton@@"}, {508, "UIReplayControlWnd@@"},
                {509, "UIReplayListBox@@"}, {510, "UIReplayRECControlWnd@@"},
                {540, "UISelectReplayDataWnd@@"},
                {303, "UIEntryQueueHelpWnd@@"}, {304, "UIEntryQueueListBox@@"},
                {305, "UIEntryQueueRequestWnd@@"}, {306, "UIEntryQueueStandByWnd@@"},
                {307, "UIEntryQueueWnd@@"},
                {378, "UIJobListWnd@@"}, {472, "UIPartyBookingHelpWnd@@"},
                {535, "UISeekPartyListWnd@@"}, {536, "UISeekPartyMBWnd@@"}, {537, "UISeekPartyWnd@@"},
                {224, "UICaptchaImage@@"}, {225, "UICaptchaPreviewWnd@@"},
                {402, "UIMacroDetectorWnd@@"}, {403, "UIMacroReporterWnd@@"},
                {404, "UIMacroUserInfoWnd@@"},
                {533, "UISecondPasswordEditWnd@@"}, {534, "UISecondPasswordInputWnd@@"},
                {566, "UIStorePasswordInputWnd@@"},
                {178, "UIAccountLimitedToolWnd@@"}, {228, "UICashShopWnd@@"},
                {230, "UICashShopWnd_Cart@@"}, {231, "UICashShopWnd_CartItem@@"},
                {232, "UICashShopWnd_Item@@"}, {428, "UIMileageWnd@@"},
                {609, "UInCashWnd@@"}, {610, "UInCash_CallWnd@@"},
                {529, "UIScrollBar_Custom@@"},
                {346, "UIIngredientInfoWnd@@"}, {384, "UILapineBoxWnd@@"},
                {385, "UILapineBox_IngredientListWnd@@"}, {386, "UILapineScrollBar@@"},
                {387, "UILapineUpgradeBoxWnd@@"}, {388, "UILapineUpgradeBox_IngredientInfoWnd@@"},
                {389, "UILapineUpgradeBox_IngredientListWnd@@"},
                {318, "UIGradeEnchantWnd@@"}, {319, "UIGradeEnchant_BtnEnhancedDown@@"},
                {320, "UIGradeEnchant_BtnEnhancedUp@@"}, {321, "UIGradeEnchant_BtnStart@@"},
                {573, "UISwapBitmapButton@@"}, {574, "UISwapEquipmentWnd@@"},
                {204, "UIBarGraphPlayerAP@@"},
                {73, "CGrannyPc@@"}, {172, "TableBox@@"}, {209, "UIBasicItemCtrl@@"},
                {210, "UIBasicSkillCtrl@@"}, {214, "UIBitmapButton2@@"}, {216, "UIBitmapTab@@"},
                {222, "UIButtonsOfBasicWnd@@"}, {238, "UIChatHisBox3@@"}, {264, "UIComboBoxV2Wnd@@"},
                {268, "UICompositionWnd@@"}, {288, "UIEditCtrl_OldList@@"}, {345, "UIImgMenu@@"},
                {391, "UIListBox2@@"}, {405, "UIMakeCharWnd@@"}, {408, "UIMakeTargetResultWnd@@"},
                {426, "UIMessageBoxAutoreturn@@"}, {429, "UIMiniBtnWnd@@"}, {434, "UIMixAcceptWnd@@"},
                {435, "UIMonsterDisplayWnd@@"}, {444, "UINavigationWnd@@"}, {447, "UINewComboBoxCtrl@@"},
                {457, "UINoticeWnd@@"}, {490, "UIQuitWnd@@"}, {494, "UIRebirthWnd@@"},
                {503, "UIRenderTargetWnd@@"}, {514, "UIRestartWnd@@"}, {528, "UIScrollBar4@@"},
                {545, "UIShowAndEditTimeCtrl@@"}, {552, "UISlotBitmap@@"}, {560, "UIStaticItem@@"},
                {561, "UIStaticItem_Illust@@"}, {563, "UIStaticText3@@"}, {567, "UIString@@"},
                {572, "UISubChatMiniWnd@@"}, {575, "UITVBitmapButton@@"}, {576, "UITVTextButton@@"},
                {579, "UITableBox@@"}, {580, "UITagButton@@"}, {581, "UITagListBox@@"},
                {586, "UITextViewEX@@"}, {592, "UIToolTipWnd@@"}, {594, "UITransMenuWnd@@"},
                {595, "UITreeView@@"}, {608, "UIZeroQuestScrollBar@@"},
                {400, "UILoginOTPWnd@@"}, {383, "UIKeyStrokeWnd@@"}
            };
            size_t entry;
            for (entry = 0; entry < sizeof retired_types / sizeof retired_types[0]; ++entry)
                if (retired_types[entry].index == i)
                    assert(!strcmp(previous_name, retired_types[entry].name));
            ++retired;
            assert(!strcmp(name, "?"));
            memcpy(guarded, &table, sizeof table);
            for (kind = 0; kind < OBJECT_TYPE_COUNT; ++kind)
                assert(!object_as_type(guarded, (enum ObjectType)kind));
            continue;
        }
        assert(((const void *const *)table)[-1] == reference_vtables[i].locator);
        memcpy(guarded, &table, sizeof table);
        for (kind = 0; kind < OBJECT_TYPE_COUNT; ++kind) {
            const struct ReferenceCast *cast = &reference_casts[kind];
            void *expected = NULL;
            void *actual;
            assert(cast->type == (enum ObjectType)kind);
            if (reference_valid_sources[i] & (UINT32_C(1) << kind)) {
                expected = __RTDynamicCast(guarded, 0, cast->source, cast->target, 0);
                ++comparisons;
            }
            actual = object_as_type(guarded, cast->type);
            assert(actual == expected);
            if (actual) {
                ++successes;
                has_name = 1;
                assert(actual == guarded);
            }
            assert(!memcmp(guarded, &table, sizeof table));
            assert(!object_as_type(NULL, cast->type));
        }
        if (has_name) {
            size_t length = strlen(previous_name);
            assert(length >= 2 && !strcmp(previous_name + length - 2, "@@"));
            assert(strlen(name) == length - 2 && !memcmp(name, previous_name, length - 2));
            ++named;
        } else {
            assert(!strcmp(name, "?"));
        }
        if (is_window) {
            size_t policy;
            ++windows;
            for (policy = 0; policy < sizeof policy_names / sizeof policy_names[0]; ++policy)
                assert(old_class_is(previous_name, policy_names[policy]) ==
                       !strcmp(name, policy_names[policy]));
        }
    }
    assert(named == 327 && windows == 307 && retired == 138);
    assert(!strcmp(object_type_name(NULL), "?"));
    assert(!strcmp(object_type_name(unknown), "?"));
    assert(!strcmp(object_type_name(pages + system.dwPageSize), "?"));
    memcpy(guarded, &unknown, sizeof unknown);
    for (kind = 0; kind < OBJECT_TYPE_COUNT; ++kind)
        assert(!object_as_type(guarded, (enum ObjectType)kind));
    assert(!object_as_type(guarded, (enum ObjectType)-1));
    assert(!object_as_type(guarded, OBJECT_TYPE_COUNT));
    assert(comparisons == 2725 && successes == 206);
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    printf("PASS: 473 concrete classes and 138 retired identities, 17 C type queries, %u prior-runtime comparisons, %u accepted casts, guarded/null/unknown objects\n", comparisons, successes);
    printf("PASS: %u C diagnostic names, %u window names preserve redraw and resize matches\n", named, windows);
    return 0;
}
