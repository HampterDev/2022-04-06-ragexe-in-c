#include "platform/code_page.h"
#include "platform/system.h"
#include "platform/window.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The SDL platform layer's clipboard takes and gives text in the client's code page (the
   language's, which clientinfo sets in g_codePage), whatever language that is. */
unsigned int g_codePage;

unsigned platform_user_language(void)
{
    return 0x412;
}

static void round_trip(unsigned int code_page, const char *text, const char *utf8)
{
    char *clipboard, *back;
    g_codePage = code_page;
    assert(platform_clipboard_set(NULL, text, strlen(text)));
    clipboard = SDL_GetClipboardText();
    assert(clipboard && !strcmp(clipboard, utf8));
    SDL_free(clipboard);
    back = platform_clipboard_get(NULL);
    assert(back && !strcmp(back, text));
    free(back);
}

int main(void)
{
    char *back;
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("platform_window: no video (%s), skipped\n", SDL_GetError());
        return 0;
    }
    round_trip(949, "\xc7\xd1\xb1\xdb chat", "\xed\x95\x9c\xea\xb8\x80 chat");
    round_trip(932, "\x93\xfa\x96\x7b", "\xe6\x97\xa5\xe6\x9c\xac");
    round_trip(1251, "\xcf\xf0\xe8\xe2\xe5\xf2", "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82");
    round_trip(1252, "caf\xe9", "caf\xc3\xa9");

    /* Text that is not the code page's keeps its ASCII. */
    g_codePage = 949;
    assert(SDL_SetClipboardText("ok \xff"));
    back = platform_clipboard_get(NULL);
    assert(back && !strncmp(back, "ok ", 3));
    free(back);

    SDL_Quit();
    puts("platform_window: clipboard text in the client's code page passed");
    return 0;
}
