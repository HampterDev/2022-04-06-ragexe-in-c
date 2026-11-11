#include "type_query_fixture.h"
#include "core/object_type_registry.h"
#include <windows.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *__RTDynamicCast(void *object, long delta, const void *source,
                     const void *target, int is_reference);

__declspec(noreturn) void fatal_error(const char *message)
{
    printf("fatal_error: %s\n", message);
    exit(1);
}

static int old_class_is(const char *name, const char *expected)
{
    while (*expected && *name == *expected) { ++name; ++expected; }
    return !*expected && (*name == '@' || !*name);
}

/* The fixture keeps the original's RTTI as the 32-bit runtime reads it: absolute pointers in a
   complete object locator of signature 0. The 64-bit runtime's __RTDynamicCast reads signature 1,
   whose pointers are 32-bit offsets from the image base (the locator's own offset says where that
   base is), so the locators, hierarchies and base classes are copied into that form, inside this
   image, before they are compared. The type descriptors are the fixture's own. */
struct Rtti64Locator { DWORD signature, offset, cd_offset; int type, hierarchy, self; };
struct Rtti64Hierarchy { DWORD signature, attributes, count; int bases; };
struct Rtti64Base { int type; DWORD contained; int mdisp, pdisp, vdisp; DWORD attributes; int hierarchy; };

extern IMAGE_DOS_HEADER __ImageBase;
static unsigned long long rtti_arena[1 << 15];
static size_t rtti_used;
static struct { const void *from; void *to; } rtti_copies[2048];
static size_t rtti_copy_count;

static int rtti_rva(const void *p)
{
    ptrdiff_t rva = (const char *)p - (const char *)&__ImageBase;
    assert(rva > 0 && rva <= INT_MAX);
    return (int)rva;
}

static void *rtti_take(size_t bytes)
{
    void *p = (unsigned char *)rtti_arena + rtti_used;
    rtti_used += (bytes + 7) & ~(size_t)7;
    assert(rtti_used <= sizeof rtti_arena);
    return p;
}

static void *rtti_copy_of(const void *from)
{
    size_t i;
    for (i = 0; i < rtti_copy_count; ++i)
        if (rtti_copies[i].from == from) return rtti_copies[i].to;
    return NULL;
}

static void rtti_note(const void *from, void *to)
{
    assert(rtti_copy_count < sizeof rtti_copies / sizeof rtti_copies[0]);
    rtti_copies[rtti_copy_count].from = from;
    rtti_copies[rtti_copy_count++].to = to;
}

static const struct Rtti64Base *rtti_base(const uintptr_t *base);

static const struct Rtti64Hierarchy *rtti_hierarchy(const uintptr_t *hierarchy)
{
    struct Rtti64Hierarchy *copy = rtti_copy_of(hierarchy);
    const uintptr_t *bases = (const uintptr_t *)hierarchy[3];
    int *array;
    size_t i;
    if (copy) return copy;
    copy = rtti_take(sizeof *copy);
    rtti_note(hierarchy, copy);
    copy->signature = (DWORD)hierarchy[0];
    copy->attributes = (DWORD)hierarchy[1];
    copy->count = (DWORD)hierarchy[2];
    array = rtti_take(copy->count * sizeof *array);
    copy->bases = rtti_rva(array);
    for (i = 0; i < copy->count; ++i)
        array[i] = rtti_rva(rtti_base((const uintptr_t *)bases[i]));
    return copy;
}

static const struct Rtti64Base *rtti_base(const uintptr_t *base)
{
    struct Rtti64Base *copy = rtti_copy_of(base);
    if (copy) return copy;
    copy = rtti_take(sizeof *copy);
    rtti_note(base, copy);
    copy->type = rtti_rva((const void *)base[0]);
    copy->contained = (DWORD)base[1];
    copy->mdisp = (int)base[2];
    copy->pdisp = (int)base[3];
    copy->vdisp = (int)base[4];
    copy->attributes = (DWORD)base[5];
    copy->hierarchy = rtti_rva(rtti_hierarchy((const uintptr_t *)base[6]));
    return copy;
}

static const struct Rtti64Locator *rtti_locator(const uintptr_t *locator)
{
    struct Rtti64Locator *copy = rtti_take(sizeof *copy);
    assert(!locator[0]);
    copy->signature = 1;
    copy->offset = (DWORD)locator[1];
    copy->cd_offset = (DWORD)locator[2];
    copy->type = rtti_rva((const void *)locator[3]);
    copy->hierarchy = rtti_rva(rtti_hierarchy((const uintptr_t *)locator[4]));
    copy->self = rtti_rva(copy);
    return copy;
}

/* What the original's dynamic_cast answers for an object whose vtable is entry `index`. */
static int runtime_cast_succeeds(size_t index, const struct ReferenceCast *cast)
{
    static const void *tables[REFERENCE_VTABLE_COUNT][2];
    static const void *object;
    void *result;
    if (!tables[index][0])
        tables[index][0] = rtti_locator(reference_vtables[index].locator);
    object = &tables[index][1];
    result = __RTDynamicCast((void *)&object, 0, cast->source, cast->target, 0);
    assert(!result || result == (void *)&object);
    return result != NULL;
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
    object_types_install(&g_objectTypes);
    assert(offsetof(struct ReferenceVtable, slots) == sizeof(void *));
    GetSystemInfo(&system);
    pages = VirtualAlloc(NULL, system.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages + system.dwPageSize, system.dwPageSize, PAGE_NOACCESS, &old_protection));
    guarded = pages + system.dwPageSize - sizeof(void *);
    for (i = 0; i < REFERENCE_VTABLE_COUNT; ++i) {
        const void *table = reference_vtables[i].slots;
        const uintptr_t *locator = reference_vtables[i].locator;
        const char *previous_name = (const char *)locator[3] + 2 * sizeof(void *) + 4;
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
                i == 592 || i == 594 || i == 595 || i == 608 || i == 400 || i == 383 || i == 223) {
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
                {400, "UILoginOTPWnd@@"}, {383, "UIKeyStrokeWnd@@"},
                {223, "UICandidateWnd@@"}
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
                expected = runtime_cast_succeeds(i, cast) ? guarded : NULL;
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
    assert(named == 326 && windows == 306 && retired == 139);
    assert(!strcmp(object_type_name(NULL), "?"));
    assert(!strcmp(object_type_name(unknown), "?"));
    assert(!strcmp(object_type_name(pages + system.dwPageSize), "?"));
    memcpy(guarded, &unknown, sizeof unknown);
    for (kind = 0; kind < OBJECT_TYPE_COUNT; ++kind)
        assert(!object_as_type(guarded, (enum ObjectType)kind));
    assert(!object_as_type(guarded, (enum ObjectType)-1));
    assert(!object_as_type(guarded, OBJECT_TYPE_COUNT));
    assert(comparisons == 2717 && successes == 205);
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    printf("PASS: 472 concrete classes and 139 retired identities, 17 C type queries, %u prior-runtime comparisons, %u accepted casts, guarded/null/unknown objects\n", comparisons, successes);
    printf("PASS: %u C diagnostic names, %u window names preserve redraw and resize matches\n", named, windows);
    return 0;
}
