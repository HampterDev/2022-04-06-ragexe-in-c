#include "session/talk_options.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int talk_option_read(const struct TalkOption *option, int *value)
{
    if (option->integer && !option->byte)
        *value = *option->integer;
    else if (option->byte && !option->integer)
        *value = *option->byte;
    else
        return 0;
    return 1;
}

int talk_option_settings_add(struct TalkOptionSettings *settings, struct TextView text,
    struct TalkOption option, int value)
{
    struct TalkOptionSetting entry = {0};
    size_t limit = (size_t)-1 / sizeof entry;

    if (limit > INT_MAX) limit = INT_MAX;
    if (settings->size >= limit || (option.integer != NULL) == (option.byte != NULL))
        return 0;
    if (!owned_text_assign(&entry.text, text))
        return 0;
    if (settings->size == settings->capacity) {
        unsigned capacity = settings->capacity ? settings->capacity + settings->capacity / 2 + 1 : 8;
        struct TalkOptionSetting *items;
        if (capacity > limit) capacity = (unsigned)limit;
        items = malloc(capacity * sizeof *items);
        if (!items) {
            owned_text_clear(&entry.text);
            return 0;
        }
        if (settings->size)
            memcpy(items, settings->items, settings->size * sizeof *items);
        free(settings->items);
        settings->items = items;
        settings->capacity = capacity;
    }
    entry.option = option;
    settings->items[settings->size++] = entry;
    if (option.byte)
        *option.byte = (unsigned char)value;
    else
        *option.integer = value;
    return 1;
}

const struct TalkOptionSetting *talk_option_settings_at(const struct TalkOptionSettings *settings,
    size_t index)
{
    return index < settings->size ? &settings->items[index] : NULL;
}

void talk_option_settings_clear(struct TalkOptionSettings *settings)
{
    unsigned i;
    for (i = 0; i < settings->size; ++i)
        owned_text_clear(&settings->items[i].text);
    free(settings->items);
    *settings = (struct TalkOptionSettings){0};
}
