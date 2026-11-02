#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/fatal.h"
#include "../src/ui/social/guild_wnd.c"

static jmp_buf abstract_failure;
static int expect_abstract, switched_to = -1;
__declspec(noreturn) void fatal_error(const char *message)
{
    assert(expect_abstract && !strcmp(message, "abstract method called"));
    longjmp(abstract_failure, 1);
}
static intptr_t switch_tab(struct UIWindow *window, struct UIWindow *sender, int message,
                      intptr_t tab, intptr_t b, intptr_t c, intptr_t d)
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

int main(void)
{
    guild_tab_cases();
    puts("UI extensions: saved guild tabs, tab selection, abstract-base failure passed.");
    return 0;
}

#include "ui_extensions_unreached.h"
