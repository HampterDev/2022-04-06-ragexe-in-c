#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/chat_tabs.h"

#include "owned_storage_test_allocator.h"

#define malloc checked_malloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/session/chat_tabs.c"
#undef free
#undef malloc

static struct ChatTabSettings settings(void)
{
    const char name[] = "한글\0tab";
    struct ChatTabSettings value = {0};
    int i;
    assert(owned_text_assign(&value.name, text_view(name, sizeof name - 1)));
    for (i = 0; i < CHAT_TAB_FLAG_COUNT; ++i) value.shown[i] = (unsigned char)(i % 3);
    value.x = -40; value.y = 31; value.width = 480; value.height = 102;
    return value;
}

static void check_values(const struct ChatTabSettings *a, const struct ChatTabSettings *b)
{
    assert(a->name.size == b->name.size);
    assert(!memcmp(owned_text_cstr(&a->name), owned_text_cstr(&b->name), a->name.size + 1));
    assert(!memcmp(a->shown, b->shown, sizeof a->shown));
    assert(a->x == b->x && a->y == b->y && a->width == b->width && a->height == b->height);
}

static void check_owner(void)
{
    struct ChatTabs tabs = {0};
    struct ChatTabSettings value = settings(), copy = {0};
    int keys[] = {3, INT_MAX, -5, 0, INT_MIN};
    int sorted[] = {INT_MIN, -5, 0, 3, INT_MAX};
    struct ChatTab before[5], *storage, *entry;
    unsigned i, calls, live;

    assert(sizeof(struct ChatTabs) == 8 && sizeof(struct ChatTabSettings) == 48);
    assert(!chat_tabs_first(&tabs) && !chat_tabs_after(&tabs, INT_MIN));
    for (i = 0; i < 5; ++i) assert(chat_tabs_set(&tabs, keys[i], &value));
    assert(tabs.size == 5);
    for (i = 0; i < 5; ++i) {
        assert(tabs.items[i].key == sorted[i]);
        check_values(&tabs.items[i].value, &value);
        assert(tabs.items[i].value.name.data != value.name.data);
    }
    assert(chat_tabs_after(&tabs, INT_MAX) == NULL);
    assert(chat_tabs_after(&tabs, -4)->key == 0);
    assert(chat_tabs_after(&tabs, 3)->key == INT_MAX);
    calls = allocation_calls;
    assert(chat_tabs_set(&tabs, 3, &chat_tabs_find(&tabs, 3)->value));
    assert(allocation_calls == calls);
    assert(chat_tabs_set(&tabs, 3, &chat_tabs_find(&tabs, -5)->value));
    assert(chat_tabs_find(&tabs, 3)->value.name.data != chat_tabs_find(&tabs, -5)->value.name.data);

    storage = tabs.items;
    memcpy(before, storage, sizeof before);
    live = outstanding;
    for (i = 1; i <= 2; ++i) {
        fail_at = allocation_calls + i;
        assert(!chat_tabs_set(&tabs, 8, &tabs.items[0].value));
        fail_at = 0;
        assert(tabs.items == storage && tabs.size == 5 && outstanding == live);
        assert(!memcmp(before, tabs.items, sizeof before));
    }
    fail_at = allocation_calls + 1;
    assert(!chat_tabs_set(&tabs, 3, &tabs.items[0].value));
    fail_at = 0;
    assert(!memcmp(before, tabs.items, sizeof before) && outstanding == live);
    fail_at = allocation_calls + 1;
    assert(!chat_tabs_get_or_add(&tabs, 8));
    fail_at = 0;
    assert(!memcmp(before, tabs.items, sizeof before) && outstanding == live);

    assert(chat_tab_settings_copy(&copy, &value));
    fail_at = allocation_calls + 1;
    assert(!chat_tab_settings_copy(&copy, &value));
    fail_at = 0;
    check_values(&copy, &value);
    assert(chat_tab_settings_copy(&copy, &copy));
    chat_tab_settings_clear(&copy);
    chat_tab_settings_clear(&copy);

    entry = chat_tabs_find(&tabs, 3);
    for (i = 1; i <= CHAT_TAB_FLAG_COUNT; ++i) {
        assert(chat_tabs_set_option(&tabs, 3, (int)i, 5));
        assert(entry->value.shown[i - 1] == 1);
        assert(chat_tab_shows_kind(&entry->value, (int)i - 1));
        assert(chat_tabs_set_option(&tabs, 3, (int)i, 0));
        assert(entry->value.shown[i - 1] == 0);
        assert(!chat_tab_shows_kind(&entry->value, (int)i - 1));
    }
    memcpy(before, tabs.items, sizeof before);
    assert(!chat_tab_shows_kind(&entry->value, INT_MIN));
    assert(!chat_tab_shows_kind(&entry->value, -1));
    assert(!chat_tab_shows_kind(&entry->value, 22));
    assert(!chat_tab_shows_kind(&entry->value, INT_MAX));
    assert(!chat_tabs_set_option(&tabs, 3, INT_MIN, 1));
    assert(!chat_tabs_set_option(&tabs, 3, 0, 1));
    assert(!chat_tabs_set_option(&tabs, 3, 23, 1));
    assert(!chat_tabs_set_option(&tabs, 3, INT_MAX, 1));
    assert(!chat_tabs_set_option(&tabs, 9, 1, 1));
    assert(!memcmp(before, tabs.items, sizeof before));

    assert(chat_tabs_remove(&tabs, -5));
    assert(!chat_tabs_remove(&tabs, -5));
    assert(chat_tabs_set(&tabs, -9, &value));
    assert(chat_tabs_set(&tabs, -3, &value));
    assert(chat_tabs_after(&tabs, -5)->key == -3);
    calls = allocation_calls;
    chat_tabs_renumber(&tabs);
    assert(allocation_calls == calls);
    for (i = 0; i < tabs.size; ++i) assert(tabs.items[i].key == (int)i);
    chat_tabs_clear(&tabs);
    chat_tabs_clear(&tabs);
    assert(!tabs.items && !tabs.size);
    assert(chat_tabs_get_or_add(&tabs, 17));
    assert(chat_tabs_find(&tabs, 17)->value.name.size == 0);
    assert(chat_tabs_find(&tabs, 17)->value.width == 0);
    assert(chat_tabs_remove(&tabs, 17) && !tabs.items);
    chat_tab_settings_clear(&value);
    assert(outstanding == 0);
}

static void check_edits(void)
{
    struct ChatTabs tabs = {0};
    struct ChatTabSettings value = {0};
    int present[41] = {0}, expected[41] = {0};
    unsigned seed = 17, i;
    for (i = 0; i < 2000; ++i) {
        int index, n = 0, j;
        seed = seed * 1664525u + 1013904223u;
        index = (seed >> 9) % 41;
        if (seed & 1) {
            value.x = (int)i;
            assert(chat_tabs_set(&tabs, index - 20, &value));
            present[index] = 1; expected[index] = (int)i;
        } else {
            assert(chat_tabs_remove(&tabs, index - 20) == present[index]);
            present[index] = 0;
        }
        for (j = 0; j < 41; ++j) {
            struct ChatTab *found = chat_tabs_find(&tabs, j - 20);
            assert(!!found == present[j]);
            if (present[j]) {
                assert(tabs.items[n].key == j - 20 && found->value.x == expected[j]);
                ++n;
            }
        }
        assert(tabs.size == (unsigned)n);
    }
    chat_tabs_clear(&tabs);
    assert(outstanding == 0);
}

int main(void)
{
    check_owner();
    check_edits();
    puts("Chat tabs: owned counted Korean names, signed order, aliasing, atomic allocation failures, option bounds, cursor mutation and 2000 edit sequences passed");
    return 0;
}
