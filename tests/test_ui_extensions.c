#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/fatal.h"
#include "../src/ui/social/guild_wnd.c"
#include "../src/ui/media/connect_sns.c"

static jmp_buf abstract_failure;
static int expect_abstract, switched_to = -1;
__declspec(noreturn) void fatal_error(const char *message)
{
    assert(expect_abstract && !strcmp(message, "abstract method called"));
    longjmp(abstract_failure, 1);
}
static int switch_tab(struct UIWindow *window, struct UIWindow *sender, int message,
                      int tab, int b, int c, int d)
{
    assert(window && !sender && message == MSG_GUILD_SWITCH_TAB && !b && !c && !d);
    switched_to = tab; return 0;
}
static void guild_tab_cases(void)
{
    struct UIGuildWnd window = {0};
    struct GuildWndInfo saved = {0};
    struct UIGuildWndOps ops = {.base = {.send_msg = switch_tab}};
    int tab;
    window.frame.base.ops = &ops.base;
    window.frame.base.x = -20; window.frame.base.y = 30;
    window.frame.base.width = 420; window.frame.base.height = 300;
    window.info = &saved;
    for (tab = GUILD_TAB_INFO; tab <= GUILD_TAB_TOTAL_INFO; ++tab) {
        ops.tab = tab; saved.tab = -99;
        ui_guild_wnd_store_info(&window.frame.base);
        assert(saved.tab == tab && saved.frame.x == -20 && saved.frame.y == 30);
        assert(saved.frame.width == 420 && saved.frame.height == 300);
        switched_to = -1;
        ui_guild_wnd_on_l_btn_down(&window.frame.base, GUILD_TAB_WIDTH * GUILD_TAB_MEMBERS + 1, 0x20);
        assert(switched_to == (tab == GUILD_TAB_MEMBERS ? -1 : GUILD_TAB_MEMBERS));
    }
    ops.tab = -1; window.info = NULL;
    ui_guild_wnd_store_info(&window.frame.base);
    window.info = &saved; expect_abstract = 1;
    if (!setjmp(abstract_failure)) {
        ui_guild_wnd_store_info(&window.frame.base);
        assert(!"abstract guild window must fail before returning a tab");
    }
    expect_abstract = 0;
}
static void image_dimensions(void)
{
    static const struct { int width, height, fitted_width, fitted_height; } cases[] = {
        {0, 0, 0, 0}, {100, 80, 100, 80}, {582, 436, 582, 436},
        {1164, 872, 582, 436}, {1164, 436, 582, 218}, {582, 872, 291, 436},
        {1200, 1200, 436, 436}, {1, 1000, 0, 436}, {-1, 5, -1, 5},
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        int width = cases[i].width, height = cases[i].height;
        ui_connect_twitter_wnd_fit_image( &width, &height);
        assert(width == cases[i].fitted_width && height == cases[i].fitted_height);
        ui_connect_twitter_wnd_min_size( &width, &height);
        assert(width == (cases[i].fitted_width < 300 ? 300 : cases[i].fitted_width));
        assert(height == (cases[i].fitted_height < 226 ? 226 : cases[i].fitted_height));
    }
}
int main(void)
{
    guild_tab_cases(); image_dimensions();
    puts("UI extensions: saved guild tabs, tab selection, abstract-base failure and screenshot fit/minimum dimensions passed.");
    return 0;
}

#include "ui_extensions_unreached.h"
