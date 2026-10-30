#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/talk_options.h"
#include "owned_storage_test_allocator.h"

#define malloc checked_malloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/session/talk_option_storage.c"
#undef free
#undef malloc

int main(void)
{
    struct TalkOptionSettings settings = {0};
    struct { unsigned char before, value, after[3]; } bytes = {0x56, 9, {0x78, 0x9a, 0xbc}};
    int integer = 17, value = 123;
    struct TalkOption byte_option = {0x6a, NULL, &bytes.value};
    struct TalkOption int_option = {0x67, &integer, NULL};
    struct TalkOption unbound = {3, NULL, NULL};
    struct TalkOption invalid = {4, &integer, &bytes.value};
    char name[] = "한글\0command";
    unsigned i, live;
    const struct TalkOptionSetting *entry;

    assert(!talk_option_read(&unbound, &value) && value == 123);
    assert(!talk_option_read(&invalid, &value) && value == 123);
    assert(!talk_option_settings_at(&settings, 0));
    assert(!talk_option_settings_add(&settings, text_view_cstr("bad"), invalid, 0));
    assert(!settings.size && integer == 17 && bytes.value == 9);
    assert(talk_option_settings_add(&settings, text_view(name, sizeof name - 1), byte_option, -1));
    assert(bytes.value == 255 && bytes.before == 0x56 && bytes.after[0] == 0x78 && bytes.after[1] == 0x9a && bytes.after[2] == 0xbc);
    entry = talk_option_settings_at(&settings, 0);
    assert(entry->text.size == sizeof name - 1 && !memcmp(entry->text.data, name, sizeof name));
    name[0] = 'x';
    assert(entry->text.data[0] != 'x');
    assert(talk_option_read(&entry->option, &value) && value == 255);
    bytes.value = 0;
    assert(talk_option_read(&entry->option, &value) && value == 0);
    while (settings.size < settings.capacity)
        assert(talk_option_settings_add(&settings, text_view_cstr("repeat"), int_option, INT_MIN));
    assert(integer == INT_MIN);
    live = outstanding;
    for (i = 1; i <= 2; ++i) {
        struct TalkOptionSetting *old = settings.items;
        unsigned size = settings.size, capacity = settings.capacity;
        fail_at = allocation_calls + i;
        assert(!talk_option_settings_add(&settings, owned_text_view(&old[0].text), byte_option, 43));
        fail_at = 0;
        assert(settings.items == old && settings.size == size && settings.capacity == capacity);
        assert(outstanding == live && bytes.value == 0 && integer == INT_MIN);
    }
    assert(talk_option_settings_add(&settings, owned_text_view(&settings.items[0].text), settings.items[0].option, 257));
    assert(bytes.value == 1);
    entry = talk_option_settings_at(&settings, settings.size - 1);
    assert(entry->text.data != settings.items[0].text.data);
    assert(entry->text.size == settings.items[0].text.size);
    assert(!memcmp(entry->text.data, settings.items[0].text.data, entry->text.size + 1));
    for (i = 0; i < 1000; ++i) {
        int next = (int)i - 500;
        unsigned index = settings.size;
        assert(talk_option_settings_add(&settings, text_view_cstr("repeat"), int_option, next));
        assert(settings.size == index + 1 && integer == next);
        entry = talk_option_settings_at(&settings, index);
        assert(entry->option.type == 0x67 && !strcmp(owned_text_cstr(&entry->text), "repeat"));
    }
    assert(!talk_option_settings_at(&settings, settings.size));
    assert(!talk_option_settings_at(&settings, (size_t)-1));
    {
        struct TalkOptionSettings oversized = {NULL, UINT_MAX, UINT_MAX};
        unsigned calls = allocation_calls;
        assert(!talk_option_settings_add(&oversized, text_view_cstr("overflow"), int_option, 2));
        assert(integer == 499 && allocation_calls == calls);
    }
    talk_option_settings_clear(&settings);
    talk_option_settings_clear(&settings);
    assert(!settings.items && !settings.size && !settings.capacity && !outstanding);
    puts("Talk option storage: typed live values, byte neighbors, owned counted text, duplicates, aliasing, growth, failure rollback and bounds passed");
    return 0;
}
