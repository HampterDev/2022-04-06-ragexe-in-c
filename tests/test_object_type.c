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
        if (i == 479 || i == 333 || i == 589 || i == 590 || i == 220 || i == 354 || i == 244 || i == 229 || i == 289 || i == 455 || i == 31 || i == 468 || (i >= 294 && i <= 302) || (i >= 356 && i <= 360) || i == 511 || i == 23 || i == 140 || i == 269 || i == 270 || i == 271 || i == 596 || i == 597 || i == 467 || i == 521 || i == 249 || i == 250 || i == 496 || i == 226 || i == 524 || i == 219 || i == 252 || i == 588 || i == 551 || i == 599 || i == 507 || i == 508 || i == 509 || i == 510 || i == 540 || (i >= 303 && i <= 307)) {
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
                {307, "UIEntryQueueWnd@@"}
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
    assert(named == 409 && windows == 388 && retired == 56);
    assert(!strcmp(object_type_name(NULL), "?"));
    assert(!strcmp(object_type_name(unknown), "?"));
    assert(!strcmp(object_type_name(pages + system.dwPageSize), "?"));
    memcpy(guarded, &unknown, sizeof unknown);
    for (kind = 0; kind < OBJECT_TYPE_COUNT; ++kind)
        assert(!object_as_type(guarded, (enum ObjectType)kind));
    assert(!object_as_type(guarded, (enum ObjectType)-1));
    assert(!object_as_type(guarded, OBJECT_TYPE_COUNT));
    assert(comparisons == 3375 && successes == 250);
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    printf("PASS: 555 concrete classes and fifty-six retired identities, 17 C type queries, %u prior-runtime comparisons, %u accepted casts, guarded/null/unknown objects\n", comparisons, successes);
    printf("PASS: %u C diagnostic names, %u window names preserve redraw and resize matches\n", named, windows);
    return 0;
}
