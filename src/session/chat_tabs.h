#ifndef SESSION_CHAT_TABS_H
#define SESSION_CHAT_TABS_H

#include "core/owned_text.h"

enum { CHAT_TAB_FLAG_COUNT = 22, CHAT_TAB_LIMIT = 10 };

struct ChatTabSettings {
    struct OwnedText name;
    unsigned char shown[CHAT_TAB_FLAG_COUNT];
    int x, y, width, height;
};

struct ChatTab {
    int key;
    struct ChatTabSettings value;
};

struct ChatTabs {
    struct ChatTab *items;
    unsigned size;
};

void chat_tabs_clear(struct ChatTabs *tabs);
struct ChatTab *chat_tabs_find(struct ChatTabs *tabs, int key);
int chat_tabs_set(struct ChatTabs *tabs, int key, const struct ChatTabSettings *value);
struct ChatTabSettings *chat_tabs_get_or_add(struct ChatTabs *tabs, int key);
int chat_tabs_remove(struct ChatTabs *tabs, int key);
void chat_tabs_renumber(struct ChatTabs *tabs);
struct ChatTab *chat_tabs_first(struct ChatTabs *tabs);
struct ChatTab *chat_tabs_after(struct ChatTabs *tabs, int key);

int chat_tab_settings_copy(struct ChatTabSettings *out, const struct ChatTabSettings *source);
void chat_tab_settings_clear(struct ChatTabSettings *settings);
int chat_tab_shows_kind(const struct ChatTabSettings *settings, int kind);
int chat_tabs_set_option(struct ChatTabs *tabs, int key, int option, int enabled);

#endif
