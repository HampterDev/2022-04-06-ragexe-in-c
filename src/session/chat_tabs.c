#include "session/chat_tabs.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static unsigned lower_bound(const struct ChatTabs *tabs, int key)
{
    unsigned first = 0, last = tabs->size;
    while (first < last) {
        unsigned middle = first + (last - first) / 2;
        if (tabs->items[middle].key < key)
            first = middle + 1;
        else
            last = middle;
    }
    return first;
}

void chat_tab_settings_clear(struct ChatTabSettings *settings)
{
    owned_text_clear(&settings->name);
    memset(settings, 0, sizeof *settings);
}

int chat_tab_settings_copy(struct ChatTabSettings *out, const struct ChatTabSettings *source)
{
    struct ChatTabSettings copy;
    if (out == source)
        return 1;
    copy = *source;
    owned_text_init(&copy.name);
    if (!owned_text_assign(&copy.name, owned_text_view(&source->name)))
        return 0;
    owned_text_clear(&out->name);
    *out = copy;
    return 1;
}

void chat_tabs_clear(struct ChatTabs *tabs)
{
    unsigned i;
    for (i = 0; i < tabs->size; ++i)
        owned_text_clear(&tabs->items[i].value.name);
    free(tabs->items);
    tabs->items = NULL;
    tabs->size = 0;
}

struct ChatTab *chat_tabs_find(struct ChatTabs *tabs, int key)
{
    unsigned at = lower_bound(tabs, key);
    return at < tabs->size && tabs->items[at].key == key ? &tabs->items[at] : NULL;
}

int chat_tabs_set(struct ChatTabs *tabs, int key, const struct ChatTabSettings *value)
{
    unsigned at = lower_bound(tabs, key);
    struct ChatTabSettings copy = {0};
    struct ChatTab *items;

    if (at < tabs->size && tabs->items[at].key == key)
        return chat_tab_settings_copy(&tabs->items[at].value, value);
    if (tabs->size >= INT_MAX || (size_t)tabs->size + 1 > (size_t)-1 / sizeof *items)
        return 0;
    if (!chat_tab_settings_copy(&copy, value))
        return 0;
    items = malloc(((size_t)tabs->size + 1) * sizeof *items);
    if (!items) {
        chat_tab_settings_clear(&copy);
        return 0;
    }
    if (at)
        memcpy(items, tabs->items, at * sizeof *items);
    if (at < tabs->size)
        memcpy(items + at + 1, tabs->items + at, (tabs->size - at) * sizeof *items);
    items[at].key = key;
    items[at].value = copy;
    free(tabs->items);
    tabs->items = items;
    ++tabs->size;
    return 1;
}

struct ChatTabSettings *chat_tabs_get_or_add(struct ChatTabs *tabs, int key)
{
    struct ChatTab *tab = chat_tabs_find(tabs, key);
    struct ChatTabSettings empty = {0};
    if (tab)
        return &tab->value;
    if (!chat_tabs_set(tabs, key, &empty))
        return NULL;
    return &chat_tabs_find(tabs, key)->value;
}

int chat_tabs_remove(struct ChatTabs *tabs, int key)
{
    unsigned at = lower_bound(tabs, key);
    if (at == tabs->size || tabs->items[at].key != key)
        return 0;
    owned_text_clear(&tabs->items[at].value.name);
    if (at + 1 < tabs->size)
        memmove(tabs->items + at, tabs->items + at + 1, (tabs->size - at - 1) * sizeof *tabs->items);
    --tabs->size;
    if (!tabs->size) {
        free(tabs->items);
        tabs->items = NULL;
    }
    return 1;
}

void chat_tabs_renumber(struct ChatTabs *tabs)
{
    unsigned i;
    for (i = 0; i < tabs->size; ++i)
        tabs->items[i].key = (int)i;
}

struct ChatTab *chat_tabs_first(struct ChatTabs *tabs)
{
    return tabs->size ? tabs->items : NULL;
}

struct ChatTab *chat_tabs_after(struct ChatTabs *tabs, int key)
{
    unsigned at = lower_bound(tabs, key);
    if (at < tabs->size && tabs->items[at].key == key)
        ++at;
    return at < tabs->size ? &tabs->items[at] : NULL;
}

int chat_tab_shows_kind(const struct ChatTabSettings *settings, int kind)
{
    return (unsigned)kind < CHAT_TAB_FLAG_COUNT && settings->shown[kind] != 0;
}

int chat_tabs_set_option(struct ChatTabs *tabs, int key, int option, int enabled)
{
    struct ChatTab *tab;
    if (option < 1 || option > CHAT_TAB_FLAG_COUNT)
        return 0;
    tab = chat_tabs_find(tabs, key);
    if (!tab)
        return 0;
    tab->value.shown[option - 1] = enabled != 0;
    return 1;
}
