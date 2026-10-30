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
        "CBmpObjWnd", "UIBalloonText", "UINameBalloonText",
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
        if (i == 524 || i == 219 || i == 252 || i == 588) {
            static const struct { size_t index; const char *name; } retired_types[] = {
                {524, "UIScroll@TagTextViewer@@"}, {219, "UIBmpTextButton2@@"},
                {252, "UIColorButton@@"}, {588, "UITextureButton@@"}
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
    assert(named == 461 && windows == 440 && retired == 4);
    assert(!strcmp(object_type_name(NULL), "?"));
    assert(!strcmp(object_type_name(unknown), "?"));
    assert(!strcmp(object_type_name(pages + system.dwPageSize), "?"));
    memcpy(guarded, &unknown, sizeof unknown);
    for (kind = 0; kind < OBJECT_TYPE_COUNT; ++kind)
        assert(!object_as_type(guarded, (enum ObjectType)kind));
    assert(!object_as_type(guarded, (enum ObjectType)-1));
    assert(!object_as_type(guarded, OBJECT_TYPE_COUNT));
    assert(comparisons > 2000 && successes > 250);
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    printf("PASS: 607 concrete classes and four retired identities, 19 C type queries, %u prior-runtime comparisons, %u accepted casts, guarded/null/unknown objects\n", comparisons, successes);
    printf("PASS: %u C diagnostic names, %u window names preserve redraw and resize matches\n", named, windows);
    return 0;
}
