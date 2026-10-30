#ifndef SESSION_TALK_OPTIONS_H
#define SESSION_TALK_OPTIONS_H

#include "core/owned_text.h"

struct TalkOption {
    int type;
    int *integer;
    unsigned char *byte;
};

struct TalkOptionSetting {
    struct OwnedText text;
    struct TalkOption option;
};

struct TalkOptionSettings {
    struct TalkOptionSetting *items;
    unsigned size, capacity;
};

int talk_option_read(const struct TalkOption *option, int *value);
int talk_option_settings_add(struct TalkOptionSettings *settings, struct TextView text,
    struct TalkOption option, int value);
const struct TalkOptionSetting *talk_option_settings_at(const struct TalkOptionSettings *settings,
    size_t index);
void talk_option_settings_clear(struct TalkOptionSettings *settings);

struct TalkOption talk_option_find(const char *text);
int talk_option_set(const char *text, int value);

#endif
