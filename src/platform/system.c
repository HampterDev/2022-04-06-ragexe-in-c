#include "platform/system.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void platform_debug_output(const char *text)
{
    OutputDebugStringA(text);
}

unsigned platform_user_language(void)
{
    return GetUserDefaultLangID();
}

void platform_set_background_priority(int background)
{
    SetPriorityClass(GetCurrentProcess(), background ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
}
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct {
    char code[3];
    unsigned id;
} user_languages[] = {
    {"de", 0x07},
    {"it", 0x10},
    {"pt", 0x16},
    {"tr", 0x1f}
};

void platform_debug_output(const char *text)
{
    fputs(text, stderr);
}

unsigned platform_user_language(void)
{
    static const char *const variables[] = {"LC_ALL", "LC_MESSAGES", "LANG"};
    const char *locale = NULL;
    size_t i;

    for (i = 0; i < sizeof variables / sizeof *variables && (!locale || !*locale); ++i)
        locale = getenv(variables[i]);
    if (!locale)
        return 0;
    for (i = 0; i < sizeof user_languages / sizeof *user_languages; ++i)
        if (!strncmp(locale, user_languages[i].code, 2) && (locale[2] < 'a' || locale[2] > 'z'))
            return user_languages[i].id;
    return 0;
}

void platform_set_background_priority(int background)
{
    (void)background;
}
#endif
