#ifndef SESSION_CHAT_TABS_LAYOUT_H
#define SESSION_CHAT_TABS_LAYOUT_H

#include "session/chat_tabs.h"

int chat_tabs_save_layout(const struct ChatTabs *main_tabs, const struct ChatTabs *separate_tabs,
                         int editable, const char *path);

#endif
