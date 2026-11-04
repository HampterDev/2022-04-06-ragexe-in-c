#include "platform/code_page.h"
#include "platform/system.h"
#include "platform/window.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The SDL platform layer's clipboard takes and gives the client's text, UTF-8 in every language
   (core/utf8.h), as SDL does. Invalid bytes become U+FFFD both ways, as Windows' CF_UNICODETEXT
   conversion of the same text gives them. */
unsigned int g_codePage = CODE_PAGE_UTF8;

unsigned platform_user_language(void)
{
    return 0x412;
}

static void round_trip(const char *text)
{
    char *clipboard, *back;
    assert(platform_clipboard_set(NULL, text, strlen(text)));
    clipboard = SDL_GetClipboardText();
    assert(clipboard && !strcmp(clipboard, text));
    SDL_free(clipboard);
    back = platform_clipboard_get(NULL);
    assert(back && !strcmp(back, text));
    free(back);
}

int main(void)
{
    char *clipboard, *back;
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("platform_window: no video (%s), skipped\n", SDL_GetError());
        return 0;
    }
    round_trip("\xed\x95\x9c\xea\xb8\x80 chat"); /* "한글 chat" */
    round_trip("\xe6\x97\xa5\xe6\x9c\xac"); /* "日本" */
    round_trip("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82"); /* "Привет" */
    round_trip("caf\xc3\xa9");
    round_trip("\xf0\x9f\x98\x80"); /* U+1F600, a surrogate pair in UTF-16 */

    /* A byte that is not UTF-8 becomes U+FFFD; the ASCII around it stays. */
    assert(platform_clipboard_set(NULL, "ok \xff!", 5));
    clipboard = SDL_GetClipboardText();
    assert(clipboard && !strcmp(clipboard, "ok \xef\xbf\xbd!"));
    SDL_free(clipboard);
    assert(SDL_SetClipboardText("ok \xc7\xd1"));
    back = platform_clipboard_get(NULL);
    assert(back && !strcmp(back, "ok \xef\xbf\xbd\xef\xbf\xbd"));
    free(back);

    SDL_Quit();
    puts("platform_window: clipboard text in UTF-8 passed");
    return 0;
}
